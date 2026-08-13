/*
 * bridge.cpp - Unified bridge with CUDA support
 * Works both with and without USE_CUDA flag
 */

#include "GUI.h"
#include "model/simulation.h"
#include "layers.h"
#include "voxel.h"
#include "tomography.h"
#include "render_pipeline.h"
#include "sinc_overlay.h"

#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <cstring>
#include <cmath>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#if defined(USE_CUDA) && !defined(CUDA_BRIDGE_CU)
#include "cuda_sim_optimized.h"
extern "C" void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX);
static bool useCuda = false;
#endif

void updateBufferSimple();

namespace
{
    constexpr double PI = 3.14159265358979323846;

    // Convert polarisation angle atan2(v,u) to an RGB colour.
    inline unsigned int polarisationColor(int u, int v)
    {
        double theta = std::atan2((double)v, (double)u);
        double hue = (theta + PI) * 360.0 / (2.0 * PI);
        if (hue >= 360.0) hue -= 360.0;
        if (hue < 0.0)   hue += 360.0;

        double c = 1.0;
        double x = c * (1.0 - std::fabs(std::fmod(hue / 60.0, 2.0) - 1.0));
        double rp = 0.0, gp = 0.0, bp = 0.0;

        if      (hue < 60.0)   { rp = c;  gp = x;  bp = 0; }
        else if (hue < 120.0)  { rp = x;  gp = c;  bp = 0; }
        else if (hue < 180.0)  { rp = 0;  gp = c;  bp = x; }
        else if (hue < 240.0)  { rp = 0;  gp = x;  bp = c; }
        else if (hue < 300.0)  { rp = x;  gp = 0;  bp = c; }
        else                   { rp = c;  gp = 0;  bp = x; }

        unsigned char r = (unsigned char)(rp * 255.0 + 0.5);
        unsigned char g = (unsigned char)(gp * 255.0 + 0.5);
        unsigned char b = (unsigned char)(bp * 255.0 + 0.5);
        return makeColor(r, g, b, 255);
    }
}

namespace automaton
{
    extern unsigned EL;
    extern unsigned W_USED;
    extern unsigned FRAME;
    extern unsigned RMAX;
    extern unsigned CONVOL;
    extern unsigned SLOT1, SLOT2, SLOT3, SLOT4, SLOT5;
    extern unsigned SLOT6, SLOT7, SLOT8;
    extern unsigned DIFFUSION;
    extern unsigned RELOC;
    extern unsigned REISSUE;
    extern unsigned FLOOD;

    extern std::vector<Cell> lattice_curr;
    extern unsigned int pulse_tick;
    extern std::vector<std::array<unsigned, 3>> lcenters;
}

// ============================================================
// Forward declarations
// ============================================================

void updateBufferCPU();

#ifdef CUDA_BRIDGE_CU
extern unsigned g_cuda_selectedLayer;
#endif

// ============================================================
// Helper for bridge_cuda.cu
// ============================================================

void updateLCenter(unsigned w, unsigned x, unsigned y, unsigned z)
{
    automaton::lcenters[w][0] = x;
    automaton::lcenters[w][1] = y;
    automaton::lcenters[w][2] = z;
}

// ============================================================
// Tomography visibility helper
// ============================================================

bool isVisibleInTomogram(unsigned x, unsigned y, unsigned z)
{
    using namespace tomography;

    // ========================================================
    // Pipeline override
    // ========================================================

    switch (gPipelineState)
    {
        case RenderPipelineState::FULL_VOLUME:
            return true;

        case RenderPipelineState::TOMOGRAPHY_XY:
            return z == tomo_z;

        case RenderPipelineState::TOMOGRAPHY_YZ:
            return x == tomo_x;

        case RenderPipelineState::TOMOGRAPHY_ZX:
            return y == tomo_y;

        default:
            break;
    }

    // ========================================================
    // Legacy fallback
    // ========================================================

    if (!tomoEnable)
        return true;

    if (!tomoEnable->getState())
        return true;

    if (tomoDirs.size() < 3)
        return true;

    if (tomoDirs[0].isSelected())
        return z == tomo_z;

    if (tomoDirs[1].isSelected())
        return x == tomo_x;

    if (tomoDirs[2].isSelected())
        return y == tomo_y;

    return true;
}

// ============================================================
// Sinc / r·sin(r) overlay profile
// ============================================================

#include <array>

namespace
{
    // Self-contained integer-only 3-D sinc wave CA taken from sine2/mytry.c.
    // It runs independently on the host, computes its own integer r/r2 from
    // the lattice centre, and feeds the 2-D overlay with:
    //   - green  : radial sinc(r) displacement profile
    //   - cyan   : trigger rate (profile / u_peak) after convergence
    //   - yellow : peak-history time series
    //   - red    : and_count[r] geometric product (triggered && active)
    struct SincOutput
    {
        std::vector<float> profile;
        std::vector<float> triggerRate;
        std::vector<float> peakHistory;
        std::vector<float> andMask;
        unsigned currentRadius = 0;
        unsigned graphSize = 0;
    };

    class SincWave
    {
    public:
        unsigned getEL() const { return m_el; }

        void reset(unsigned EL)
        {
            m_el = EL;
            m_tick = 0;
            m_curSweepR = 0;
            m_curPulseR2 = 0;
            m_sincConverged = false;
            m_stableFrames = 0;
            m_uPeak = 0;
            m_peakIdx = 0;
            m_sincQ = 1;

            m_centerX = (int)automaton::CENTER;
            m_centerY = m_centerX;
            m_centerZ = m_centerX;

            m_radius = (m_el > 4) ? (int)(m_el / 2 - 2) : 1;
            if (m_radius < 1) m_radius = 1;
            m_shellR   = (int)(m_el * 12 / 100);
            m_shellW   = ((int)m_el / 10) > 0 ? ((int)m_el / 10) : 1;
            m_absorbW  = ((m_radius / 27) > 2) ? (m_radius / 27) : 2;
            m_pulseStep = ((int)m_el + 15) / 30;
            if (m_pulseStep < 1) m_pulseStep = 1;
            m_pulseTol = ((m_el * m_el + 150u) / 300u) > 0u
                            ? ((m_el * m_el + 150u) / 300u)
                            : 1u;
            m_graphSize = (unsigned)m_radius;

            m_diffDivShift = 2;
            if (m_radius >= 384)       m_diffDivShift = 6;
            else if (m_radius >= 192)  m_diffDivShift = 5;
            else if (m_radius >= 96)   m_diffDivShift = 4;
            else if (m_radius >= 40)   m_diffDivShift = 3;

            m_velDampShift = m_diffDivShift + 3;

            m_ttlDecayMask = 7;
            if (m_radius >= 384)       m_ttlDecayMask = 127;
            else if (m_radius >= 192) m_ttlDecayMask = 63;
            else if (m_radius >= 96)  m_ttlDecayMask = 31;
            else if (m_radius >= 40)  m_ttlDecayMask = 15;

            size_t n = (size_t)m_el * m_el * m_el;
            m_u.assign(n, 0);
            m_v.assign(n, 0);
            m_uNext.assign(n, 0);
            m_vNext.assign(n, 0);
            m_sincP.assign(n, 0);
            m_acc.assign(n, 0);
            m_ttl.assign(n, 0);

            m_andCount.assign(m_graphSize, 0);
            m_profileRaw.assign(m_graphSize, 0);
            m_profilePrev.assign(m_graphSize, 0);
            m_profile.assign(m_graphSize, 0.0f);
            m_triggerRate.assign(m_graphSize, 0.0f);
            m_andMask.assign(m_graphSize, 0.0f);
            m_peakHistory.assign(PEAK_HIST_W, 0);
            m_peakHistoryBuf.assign(PEAK_HIST_W, 0.0f);

            size_t cx = (size_t)m_centerX;
            size_t cy = (size_t)m_centerY;
            size_t cz = (size_t)m_centerZ;
            if (cx < m_el && cy < m_el && cz < m_el)
                m_u[((cx * m_el + cy) * m_el + cz)] = SHELL_TARGET / 8;
        }

        void stepAndUpdate()
        {
            if (m_el == 0)
                return;

            stepOnce();
            updateProfile();
        }

        unsigned int pulseFromTime(unsigned int t) const
        {
            const unsigned int min_r2 = 0;
            int maxR = m_radius - 1;
            if (maxR < 1) maxR = 1;
            const unsigned int max_r2 = (unsigned int)maxR * (unsigned int)maxR;
            const unsigned int step = (unsigned int)m_pulseStep;
            unsigned int span = max_r2 - min_r2;
            if (span == 0) return min_r2;
            unsigned int period = 2 * span;
            unsigned int phase = (t * step) % period;
            if (phase < span)
                return min_r2 + phase;
            else
                return max_r2 - (phase - span);
        }

        void publish(SincOutput& out) const
        {
            out.profile     = m_profile;
            out.triggerRate = m_triggerRate;
            out.peakHistory = m_peakHistoryBuf;
            out.andMask     = m_andMask;
            out.currentRadius = static_cast<unsigned>(m_curSweepR);
            out.graphSize   = m_graphSize;
        }

        const std::vector<float>& profile()      const { return m_profile; }
        const std::vector<float>& triggerRate()  const { return m_triggerRate; }
        const std::vector<float>& peakHistory()  const { return m_peakHistoryBuf; }
        const std::vector<float>& andMask()      const { return m_andMask; }
        unsigned currentRadius() const { return static_cast<unsigned>(m_curSweepR); }
        unsigned graphSize()     const { return m_graphSize; }

    private:
        static constexpr int DIFF_SHIFT         = 4;
        static constexpr int SHELL_TARGET         = 16384;
        static constexpr int STABILITY_THRESHOLD  = 35;
        static constexpr int STABILITY_FRAMES   = 180;
        static constexpr int PROFILE_PEAK_REF   = SHELL_TARGET * 3;
        static constexpr int PEAK_HIST_W        = 600;

        unsigned m_el = 0;
        unsigned m_tick = 0;
        int m_curSweepR = 0;
        unsigned int m_curPulseR2 = 0;

        int m_centerX = 0, m_centerY = 0, m_centerZ = 0;
        int m_radius = 0;
        int m_shellR = 0, m_shellW = 0, m_absorbW = 0, m_pulseStep = 0;
        int m_diffDivShift = 2, m_velDampShift = 5, m_ttlDecayMask = 7;
        int m_sincQ = 1;
        unsigned int m_pulseTol = 1;
        unsigned int m_graphSize = 0;

        std::vector<int>           m_u, m_v, m_uNext, m_vNext;
        std::vector<int>           m_sincP, m_acc;
        std::vector<unsigned char> m_ttl;
        std::vector<int64_t>       m_andCount;
        std::vector<int64_t>       m_profileRaw;
        std::vector<int64_t>       m_profilePrev;
        std::vector<float>         m_profile;
        std::vector<float>         m_triggerRate;
        std::vector<float>         m_andMask;
        std::vector<int>           m_peakHistory;
        std::vector<float>         m_peakHistoryBuf;
        size_t m_peakIdx = 0;
        bool   m_sincConverged = false;
        int    m_stableFrames = 0;
        int64_t m_uPeak = 0;

        void stepOnce()
        {
            const unsigned EL = m_el;
            if (EL < 3)
                return;

            const int R = m_radius;
            const int shellR = m_shellR;
            const int shellW = m_shellW;
            const int absorbW = m_absorbW;
            const int diffDivShift = m_diffDivShift;
            const int velDampShift = m_velDampShift;
            const int ttlDecayMask = m_ttlDecayMask;
            const int sincQ = m_sincQ;
            const unsigned int pulseTol = m_pulseTol;

            m_curPulseR2 = pulseFromTime(m_tick);
            m_curSweepR = isqrt((int)m_curPulseR2);

            for (unsigned x = 1; x < EL - 1; ++x)
            {
                size_t rowBaseXp = ((size_t)(x + 1) * EL) * EL;
                size_t rowBaseX  = ((size_t)x       * EL) * EL;
                size_t rowBaseXm = ((size_t)(x - 1) * EL) * EL;

                for (unsigned y = 1; y < EL - 1; ++y)
                {
                    size_t baseXp = rowBaseXp + (size_t)y * EL;
                    size_t baseX  = rowBaseX  + (size_t)y * EL;
                    size_t baseXm = rowBaseXm + (size_t)y * EL;
                    size_t baseYp = rowBaseX  + (size_t)(y + 1) * EL;
                    size_t baseYm = rowBaseX  + (size_t)(y - 1) * EL;

                    for (unsigned z = 1; z < EL - 1; ++z)
                    {
                        int dx_ = (int)x - m_centerX;
                        int dy_ = (int)y - m_centerY;
                        int dz_ = (int)z - m_centerZ;
                        unsigned int ax = (unsigned int)(dx_ < 0 ? -dx_ : dx_);
                        unsigned int ay = (unsigned int)(dy_ < 0 ? -dy_ : dy_);
                        unsigned int az = (unsigned int)(dz_ < 0 ? -dz_ : dz_);
                        unsigned int r2 = ax * ax + ay * ay + az * az;
                        int r = isqrt((int)r2);

                        if (r < 0 || r >= R)
                        {
                            m_uNext[baseX + z] = 0;
                            m_vNext[baseX + z] = 0;
                            m_acc[baseX + z] = 0;
                            m_ttl[baseX + z] = 0;
                            continue;
                        }

                        size_t idx = baseX + z;
                        int u = m_u[idx];
                        int v = m_v[idx];

                        int neighbors =
                            m_u[baseXp + z] + m_u[baseXm + z]
                          + m_u[baseYp + z] + m_u[baseYm + z]
                          + m_u[baseX + z + 1]
                          + m_u[baseX + z - 1];

                        int lap = neighbors - (u << 2) - (u << 1);

                        int diffShift = DIFF_SHIFT + 1 - (r >> diffDivShift);
                        if (diffShift < DIFF_SHIFT - 1)
                            diffShift = DIFF_SHIFT - 1;

                        int v_new = v + (lap >> diffShift);
                        int u_new = u + v_new;
                        v_new -= (v_new >> velDampShift);

                        int dr = r - shellR;
                        if (dr < 0) dr = -dr;
                        if (dr <= shellW)
                        {
                            if (u > SHELL_TARGET)
                            {
                                int excess = u - SHELL_TARGET;
                                v_new -= (excess >> 4);
                            }
                            else if ((m_tick & 3) == 0)
                            {
                                int deficit = SHELL_TARGET - u;
                                v_new += (deficit >> 10) + 1;
                            }
                        }

                        if (r > R - absorbW)
                        {
                            int dist = r - (R - absorbW);
                            if (dist >= absorbW)
                                u_new = 0;
                            else
                                u_new >>= dist;
                        }

                        if (u_new < 0)
                            u_new = 0;

                        // Bresenham trigger
                        int acc = m_acc[idx] + m_sincP[idx];
                        int triggered = 0;
                        if (sincQ > 0 && acc >= sincQ)
                        {
                            acc -= sincQ;
                            triggered = 1;
                        }

                        // TTL persistence
                        unsigned char ttl = m_ttl[idx];
                        if (((int)m_tick & ttlDecayMask) == 0 && ttl > 0)
                            ttl--;

                        // Geometric product with pulsating wavefront
                        unsigned int d = (r2 > m_curPulseR2)
                                            ? (r2 - m_curPulseR2)
                                            : (m_curPulseR2 - r2);
                        bool active = (d <= pulseTol);

                        if (triggered && active)
                        {
                            if (sincQ > 0)
                                ttl = (unsigned char)(32 + (223 * m_sincP[idx]) / sincQ);

                            if (r >= 0 && (unsigned)r < m_graphSize && r == m_curSweepR)
                                m_andCount[r]++;
                        }

                        m_uNext[idx] = u_new;
                        m_vNext[idx] = v_new;
                        m_acc[idx]   = acc;
                        m_ttl[idx]   = ttl;
                    }
                }
            }

            // Copy back with global damping.
            size_t n = m_u.size();
            for (size_t i = 0; i < n; ++i)
            {
                int u_new = m_uNext[i];
                int v_new = m_vNext[i];
                m_u[i] = u_new - (u_new >> 12);
                m_v[i] = v_new - (v_new >> 12);
            }

            ++m_tick;
        }

        void updateProfile()
        {
            const unsigned EL = m_el;
            const unsigned size = m_graphSize;
            m_profileRaw.assign(size, 0);
            std::vector<int> counts(size, 0);

            for (unsigned x = 0; x < EL; ++x)
            {
                size_t rowBase = ((size_t)x * EL) * EL;
                for (unsigned y = 0; y < EL; ++y)
                {
                    size_t base = rowBase + (size_t)y * EL;
                    for (unsigned z = 0; z < EL; ++z)
                    {
                        int dx_ = (int)x - m_centerX;
                        int dy_ = (int)y - m_centerY;
                        int dz_ = (int)z - m_centerZ;
                        unsigned int ax = (unsigned int)(dx_ < 0 ? -dx_ : dx_);
                        unsigned int ay = (unsigned int)(dy_ < 0 ? -dy_ : dy_);
                        unsigned int az = (unsigned int)(dz_ < 0 ? -dz_ : dz_);
                        unsigned int r2 = ax * ax + ay * ay + az * az;
                        int r = isqrt((int)r2);

                        if (r < 0 || (unsigned)r >= size)
                            continue;

                        size_t idx = base + z;
                        m_profileRaw[r] += m_u[idx];
                        counts[r]++;
                    }
                }
            }

            for (unsigned i = 0; i < size; ++i)
            {
                if (counts[i] > 0)
                    m_profileRaw[i] /= counts[i];
            }

            int64_t maxChange = 0;
            for (unsigned i = 0; i < size; ++i)
            {
                int64_t d = m_profileRaw[i] - m_profilePrev[i];
                if (d < 0) d = -d;
                if (d > maxChange) maxChange = d;
            }

            if (!m_sincConverged)
            {
                if (maxChange < STABILITY_THRESHOLD)
                    m_stableFrames++;
                else
                    m_stableFrames = 0;

                if (m_stableFrames >= STABILITY_FRAMES)
                {
                    m_sincConverged = true;
                    m_uPeak = 0;
                    for (unsigned i = 0; i < size; ++i)
                    {
                        if (m_profileRaw[i] > m_uPeak)
                            m_uPeak = m_profileRaw[i];
                    }

                    if (m_uPeak > 0)
                    {
                        m_sincQ = (int)m_uPeak;
                        size_t n = m_u.size();
                        for (size_t i = 0; i < n; ++i)
                        {
                            m_sincP[i] = m_u[i];
                            m_acc[i] = 0;
                        }
                    }
                }
            }

            m_profilePrev = m_profileRaw;

            int64_t peak = 1;
            for (unsigned i = 0; i < size; ++i)
            {
                if (counts[i] > 0 && m_profileRaw[i] > peak)
                    peak = m_profileRaw[i];
            }
            m_peakHistory[m_peakIdx] = (int)(peak > INT32_MAX ? INT32_MAX : peak);
            m_peakIdx = (m_peakIdx + 1) % PEAK_HIST_W;

            for (unsigned i = 0; i < size; ++i)
            {
                m_profile[i] = (float)m_profileRaw[i] / (float)PROFILE_PEAK_REF;
                m_triggerRate[i] = (m_sincConverged && m_uPeak > 0)
                                        ? (float)m_profileRaw[i] / (float)m_uPeak
                                        : 0.0f;
            }

            int64_t maxCount = 1;
            for (unsigned i = 0; i < size; ++i)
            {
                if (m_andCount[i] > maxCount)
                    maxCount = m_andCount[i];
            }
            if (maxCount < 1) maxCount = 1;

            for (unsigned i = 0; i < size; ++i)
                m_andMask[i] = (float)m_andCount[i] / (float)maxCount;

            int maxHist = 1;
            for (int i = 0; i < PEAK_HIST_W; ++i)
            {
                if (m_peakHistory[i] > maxHist)
                    maxHist = m_peakHistory[i];
            }
            int denom = maxHist + (maxHist >> 3);
            if (denom < 1) denom = 1;

            for (int i = 0; i < PEAK_HIST_W; ++i)
            {
                int val = m_peakHistory[(m_peakIdx + i) % PEAK_HIST_W];
                m_peakHistoryBuf[i] = (float)val / (float)denom;
            }
        }
    };

    SincWave g_sincWave;

    std::mutex g_sincOutputMutex;
    std::array<SincOutput, 2> g_sincOutputs;
    std::atomic<int> g_sincOutputReadIdx{0};

    std::atomic<bool> g_overlayStop{false};
    std::thread g_overlayThread;

    void overlayThreadFunc()
    {
        while (!g_overlayStop.load(std::memory_order_acquire))
        {
            unsigned el = g_sincWave.getEL();
            if (el == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            g_sincWave.stepAndUpdate();

            int readIdx = g_sincOutputReadIdx.load(std::memory_order_acquire);
            int writeIdx = 1 - readIdx;
            SincOutput& out = g_sincOutputs[writeIdx];
            g_sincWave.publish(out);

            std::lock_guard<std::mutex> lock(g_sincOutputMutex);
            g_sincOutputReadIdx.store(writeIdx, std::memory_order_release);
        }
    }

    void stopOverlayThread()
    {
        if (g_overlayThread.joinable())
        {
            g_overlayStop.store(true, std::memory_order_release);
            g_overlayThread.join();
            g_overlayStop.store(false, std::memory_order_release);
        }
    }

    void startOverlayThread()
    {
        stopOverlayThread();
        g_overlayThread = std::thread(overlayThreadFunc);
    }

    struct OverlayLifetime
    {
        ~OverlayLifetime() { stopOverlayThread(); }
    };
    OverlayLifetime g_overlayLifetime;
}

namespace sinc_overlay
{
    // Double buffers for the HUD overlay.
    static std::array<std::vector<float>, 2> profileBufs;
    static std::array<std::vector<float>, 2> triggerRateBufs;
    static std::array<std::vector<float>, 2> peakHistoryBufs;
    static std::array<std::vector<float>, 2> andMaskBufs;

    static std::atomic<int>      frontIdx{0};
    static std::atomic<unsigned> pulseRadius{0};
    static std::atomic<unsigned> gGraphSize{0};
    static std::atomic<bool>     readyFlag{false};

    const std::vector<float>& profile()     { return profileBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& triggerRate() { return triggerRateBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& peakHistory() { return peakHistoryBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& andMask()     { return andMaskBufs[frontIdx.load(std::memory_order_acquire)]; }
    unsigned currentRadius() { return pulseRadius.load(std::memory_order_acquire); }
    unsigned graphSize()     { return gGraphSize.load(std::memory_order_acquire); }
    bool ready()             { return readyFlag.load(std::memory_order_acquire); }

    void update(unsigned selectedW)
    {
        (void)selectedW; // overlay uses its own self-contained CA centred on layer 0

        if (automaton::EL == 0 || automaton::lattice_curr.empty())
            return;

        if (g_sincWave.getEL() != automaton::EL)
        {
            stopOverlayThread();
            g_sincWave.reset(automaton::EL);
            startOverlayThread();
        }

        SincOutput snapshot;
        {
            std::lock_guard<std::mutex> lock(g_sincOutputMutex);
            snapshot = g_sincOutputs[g_sincOutputReadIdx.load(std::memory_order_acquire)];
        }

        int backIdx = 1 - frontIdx.load(std::memory_order_relaxed);
        profileBufs[backIdx]     = snapshot.profile;
        triggerRateBufs[backIdx]  = snapshot.triggerRate;
        peakHistoryBufs[backIdx]  = snapshot.peakHistory;
        andMaskBufs[backIdx]      = snapshot.andMask;

        pulseRadius.store(snapshot.currentRadius, std::memory_order_release);
        gGraphSize.store(snapshot.graphSize, std::memory_order_release);
        frontIdx.store(backIdx, std::memory_order_release);
        readyFlag.store(true, std::memory_order_release);
    }
} // namespace sinc_overlay

#if defined(USE_CUDA) && !defined(CUDA_BRIDGE_CU)

// ============================================================
// Cell conversion helpers
// ============================================================

static void convertCellToCellDevice(
    const automaton::Cell& src,
    ::CellDevice& dst)
{
    dst.ch  = static_cast<uint8_t>(src.ch);
    dst.pB  = src.pB ? 1 : 0;
    dst.sB  = src.sB ? 1 : 0;
    dst.a   = static_cast<uint32_t>(src.a);

    for (int i = 0; i < 4; ++i)
        dst.x[i] = static_cast<uint32_t>(src.x[i]);

    dst.r2    = static_cast<uint32_t>(src.r2);
    dst.r     = static_cast<int32_t>(src.r);
    dst.u     = static_cast<int32_t>(src.u);
    dst.v     = static_cast<int32_t>(src.v);
    dst.active= src.active ? 1u : 0u;
    dst.phiB  = src.phiB ? 1 : 0;
    dst.t     = static_cast<uint32_t>(src.t);
    dst.f     = static_cast<uint32_t>(src.f);

    for (int i = 0; i < 3; ++i)
        dst.c[i] = static_cast<uint32_t>(src.c[i]);

    dst.k   = static_cast<uint32_t>(src.k);
    dst.s2B = src.s2B ? 1 : 0;

    dst.kB  = src.kB ? 1 : 0;
    dst.bB  = src.bB ? 1 : 0;
    dst.hB  = src.hB ? 1 : 0;
    dst.cB  = src.cB ? 1 : 0;

    dst.kind       = static_cast<uint8_t>(src.kind);
    dst.parent     = src.parent;
    dst.spin_target= static_cast<int32_t>(src.spin_target);
    dst.pair_idx   = src.pair_idx;
    for (int i = 0; i < 3; ++i)
        dst.m[i]   = static_cast<int32_t>(src.m[i]);
}

static void convertCellDeviceToCell(
    const ::CellDevice& src,
    automaton::Cell& dst)
{
    dst.ch  = static_cast<unsigned char>(src.ch);
    dst.pB  = (src.pB != 0);
    dst.sB  = (src.sB != 0);
    dst.a   = static_cast<unsigned>(src.a);

    for (int i = 0; i < 4; ++i)
        dst.x[i] = static_cast<unsigned>(src.x[i]);

    dst.r2    = static_cast<unsigned>(src.r2);
    dst.r     = static_cast<int>(src.r);
    dst.u     = static_cast<int>(src.u);
    dst.v     = static_cast<int>(src.v);
    dst.active= (src.active != 0);
    dst.phiB  = (src.phiB != 0);
    dst.t     = static_cast<unsigned>(src.t);
    dst.f     = static_cast<unsigned>(src.f);

    for (int i = 0; i < 3; ++i)
        dst.c[i] = static_cast<unsigned>(src.c[i]);

    dst.k   = static_cast<unsigned>(src.k);
    dst.s2B = (src.s2B != 0);

    dst.kB  = (src.kB != 0);
    dst.bB  = (src.bB != 0);
    dst.hB  = (src.hB != 0);
    dst.cB  = (src.cB != 0);

    dst.kind       = static_cast<automaton::SourceKind>(src.kind);
    dst.parent     = src.parent;
    dst.spin_target= static_cast<int8_t>(src.spin_target);
    dst.pair_idx   = src.pair_idx;
    for (int i = 0; i < 3; ++i)
        dst.m[i]   = static_cast<int>(src.m[i]);
}

// ============================================================
// CUDA initialization
// ============================================================

bool initializeCudaSimulation()
{
    if (!isCudaAvailable())
    {
        fprintf(stderr, "CUDA not available\n");
        return false;
    }

    if (!initCudaSimulation(
            automaton::EL,
            automaton::W_USED))
    {
        fprintf(stderr, "Failed to initialize CUDA simulation\n");
        return false;
    }

    setCudaConstants(
        automaton::EL,
        automaton::W_USED,
        automaton::RMAX);

    size_t totalCells =
        static_cast<size_t>(automaton::EL) *
        automaton::EL *
        automaton::EL *
        automaton::W_USED;

    std::vector<::CellDevice> deviceCells(totalCells);

    for (size_t i = 0; i < totalCells; ++i)
    {
        convertCellToCellDevice(
            automaton::lattice_curr[i],
            deviceCells[i]);
    }

    if (!uploadLatticeToCuda(deviceCells.data(), totalCells))
    {
        fprintf(stderr, "Failed to upload lattice\n");
        cudaCleanup();
        return false;
    }

    printf("CUDA simulation initialized\n");
    return true;
}

// ============================================================
// CUDA simulation step
// ============================================================

void cudaSimulationStepWrapper()
{
    cudaSimulationStep(
        automaton::CONVOL,
        automaton::SLOT1,
        automaton::SLOT2,
        automaton::SLOT3,
        automaton::SLOT4,
        automaton::DIFFUSION,
        automaton::SLOT5,
        automaton::SLOT6,
        automaton::SLOT7,
        automaton::SLOT8,
        automaton::RELOC,
        automaton::REISSUE,
        automaton::FLOOD,
        automaton::FRAME,
        automaton::RMAX,
        scenario
    );

    size_t totalCells =
        static_cast<size_t>(automaton::EL) *
        automaton::EL *
        automaton::EL *
        automaton::W_USED;

    std::vector<::CellDevice> deviceCells(totalCells);

    if (downloadLatticeFromCuda(deviceCells.data(), totalCells))
    {
        for (size_t i = 0; i < totalCells; ++i)
        {
            convertCellDeviceToCell(
                deviceCells[i],
                automaton::lattice_curr[i]);
        }
    }
}

// ============================================================
// CUDA voxel update
// ============================================================

void updateBufferCuda()
{
    unsigned selectedW =
        (framework::layerList && framework::layerList.get())
        ? framework::layerList->getSelected()
        : 0u;

    cudaUpdateVoxelsLayer(selectedW);

    uint32_t* gpuVoxels = getMappedVoxels();

    if (!gpuVoxels)
    {
        updateBufferCPU();
        return;
    }

    size_t idx = 0;

    for (unsigned x = 0; x < automaton::EL; ++x)
    for (unsigned y = 0; y < automaton::EL; ++y)
    for (unsigned z = 0; z < automaton::EL; ++z)
    {
        if (!isVisibleInTomogram(x, y, z))
        {
            voxels[idx++] = 0x00000000u;
        }
        else
        {
            voxels[idx++] =
                gpuVoxels[
                    (x * automaton::EL + y) *
                    automaton::EL + z];
        }
    }

    sinc_overlay::update(selectedW);
}

#endif // USE_CUDA && !CUDA_BRIDGE_CU

// ============================================================
// CPU voxel update
// ============================================================


void updateBufferCPU()
{
    unsigned selectedW =
        (framework::layerList && framework::layerList.get())
        ? framework::layerList->getSelected()
        : 0u;

#ifdef CUDA_BRIDGE_CU
    g_cuda_selectedLayer = selectedW;
#endif

    size_t idx = 0;

    // Marker follows the pulse radius of the centre cell for the selected layer.
    const automaton::Cell& centreCell =
        automaton::getCell(
            automaton::lattice_curr,
            automaton::CENTER, automaton::CENTER, automaton::CENTER,
            selectedW);
    unsigned int pulse_r = automaton::effective_t(centreCell.t);

    for (unsigned x = 0; x < automaton::EL; ++x)
    for (unsigned y = 0; y < automaton::EL; ++y)
    for (unsigned z = 0; z < automaton::EL; ++z)
    {
        if (!isVisibleInTomogram(x, y, z))
        {
            voxels[idx++] = 0x00000000u;
            continue;
        }

        const automaton::Cell& cell =
            automaton::getCell(
                automaton::lattice_curr,
                x, y, z,
                selectedW);

        uint32_t color = 0x00000000u;

        if (gConfig.data3D[4] && cell.r2 != INF_R2 && (cell.u != 0 || cell.v != 0))
        {
            // Colour cells inside the bubble by their polarisation angle.
            color = polarisationColor(cell.u, cell.v);
        }

        if (cell.active)
        {
            // Highlight the current pulse wavefront in white.
            color = makeColor(255, 255, 255, 255);
        }

        if (cell.r2 == 0)
        {
            // Center cell
            color = makeColor(80, 255, 80, 255);   // Green
        }

        // Current radius marker on X axis (red dot)
        unsigned int markerX = automaton::CENTER + pulse_r;
        if (x == markerX &&
            y == automaton::CENTER &&
            z == automaton::CENTER)
        {
            color = makeColor(255, 80, 80, 255);   // Red
        }

        voxels[idx++] = color;
    }

    sinc_overlay::update(selectedW);
}

// ============================================================
// Public API
// ============================================================

#if !defined(CUDA_BRIDGE_CU)

#ifndef USE_CUDA

void automaton::updateBuffer()
{
    updateBufferCPU();
    //updateBufferSimple();
}

#else

void automaton::updateBuffer()
{
    if (useCuda)
        updateBufferCuda();
    else
        updateBufferCPU();
}

#endif

#ifdef USE_CUDA

namespace automaton
{
    bool tryEnableCuda()
    {
        if (useCuda)
            return true;

        bool ok = initializeCudaSimulation();

        if (ok)
        {
            useCuda = true;
            printf("CUDA ENABLED\n");
        }

        return ok;
    }

    void disableCuda()
    {
        if (!useCuda)
            return;

        size_t totalCells =
            static_cast<size_t>(EL) *
            EL * EL * W_USED;

        std::vector<::CellDevice> deviceCells(totalCells);

        if (downloadLatticeFromCuda(
                deviceCells.data(),
                totalCells))
        {
            for (size_t i = 0; i < totalCells; ++i)
            {
                convertCellDeviceToCell(
                    deviceCells[i],
                    lattice_curr[i]);
            }
        }

        cudaCleanup();

        useCuda = false;

        printf("CUDA DISABLED\n");
    }

    bool isCudaEnabled()
    {
        return useCuda;
    }
}

#else

namespace automaton
{
    bool tryEnableCuda()
    {
        std::cerr
            << "CUDA support not compiled."
            << std::endl;

        return false;
    }

    void disableCuda()
    {
    }

    bool isCudaEnabled()
    {
        return false;
    }
}

#endif // USE_CUDA

#endif // !CUDA_BRIDGE_CU