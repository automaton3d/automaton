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
#include <cstdint>
#include <memory>
#include <cstring>
#include <cmath>
#include <iostream>

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
    // Integer-only 3-D sinc wave CA taken from sine2/mytry.c.
    // It runs independently on the host, reads the BFS radius r/r2 from
    // layer w = 0 of lattice_curr, and feeds the 2-D overlay with a
    // radial sinc(r) displacement profile and a red (v > 0 && active)
    // geometric-product mask.
    class SincWave
    {
    public:
        void run()
        {
            if (automaton::EL == 0 || automaton::lattice_curr.empty() ||
                automaton::lcenters.empty())
                return;

            if (m_el != automaton::EL || automaton::pulse_tick < m_lastPulseTick)
                reset();

            if (m_tick == 0)
            {
                // Produce an initial profile immediately so the overlay is
                // populated even before the first simulation tick.
                stepOnce();
                m_lastPulseTick = automaton::pulse_tick;
            }
            else
            {
                unsigned delta = automaton::pulse_tick - m_lastPulseTick;
                for (unsigned i = 0; i < delta; ++i)
                    stepOnce();
                m_lastPulseTick = automaton::pulse_tick;
            }

            updateProfile();
        }

        const std::vector<float>& profile() const { return m_profile; }
        const std::vector<float>& andMask() const { return m_andMask; }
        unsigned currentRadius() const { return static_cast<unsigned>(m_curSweepR); }

    private:
        static constexpr int DIFF_SHIFT   = 4;
        static constexpr int SHELL_TARGET = 16384;

        unsigned m_el = 0;
        unsigned m_tick = 0;
        unsigned m_lastPulseTick = 0xFFFFFFFFu;
        int m_curSweepR = 0;
        unsigned int m_curPulseR2 = 0;

        std::vector<int>   m_u, m_v, m_uNext, m_vNext;
        std::vector<float> m_profile, m_andMask;

        void reset()
        {
            m_el = automaton::EL;
            m_tick = 0;
            m_lastPulseTick = automaton::pulse_tick;
            m_curSweepR = 0;
            m_curPulseR2 = 0;

            size_t n = (size_t)m_el * m_el * m_el;
            m_u.assign(n, 0);
            m_v.assign(n, 0);
            m_uNext.assign(n, 0);
            m_vNext.assign(n, 0);
            m_profile.assign(m_el + 1, 0.0f);
            m_andMask.assign(m_el + 1, 0.0f);

            unsigned cx = automaton::lcenters[0][0];
            unsigned cy = automaton::lcenters[0][1];
            unsigned cz = automaton::lcenters[0][2];
            if (cx < m_el && cy < m_el && cz < m_el)
                m_u[((size_t)cx * m_el + cy) * m_el + cz] = SHELL_TARGET / 8;
        }

        void stepOnce()
        {
            const unsigned EL = m_el;
            if (EL < 3)
                return;

            const int R = (EL > 4) ? (int)(EL / 2 - 2) : 1;

            const int shellR   = (int)(EL * 12 / 100);
            const int shellW   = (EL / 10) > 0 ? (EL / 10) : 1;
            const int absorbW  = (R / 27) > 2 ? (R / 27) : 2;
            const int pulseStep = ((EL + 15) / 30) > 0 ? ((EL + 15) / 30) : 1;

            int diffDivShift = 2;
            if (R >= 384) diffDivShift = 6;
            else if (R >= 192) diffDivShift = 5;
            else if (R >= 96) diffDivShift = 4;
            else if (R >= 40) diffDivShift = 3;

            int velDampShift = 2;
            if (R >= 384) velDampShift = 9;
            else if (R >= 192) velDampShift = 8;
            else if (R >= 96) velDampShift = 7;
            else if (R >= 40) velDampShift = 6;

            m_curPulseR2 = automaton::pulse_from_time(m_tick * (unsigned)pulseStep);
            m_curSweepR  = isqrt((int)m_curPulseR2);

            const automaton::Cell* lattice = automaton::lattice_curr.data();

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

                    const automaton::Cell* rowC = lattice + baseX;

                    for (unsigned z = 1; z < EL - 1; ++z)
                    {
                        const automaton::Cell& c = rowC[z];
                        int r = c.r;
                        if (r < 0 || r >= R)
                        {
                            m_uNext[baseX + z] = 0;
                            m_vNext[baseX + z] = 0;
                            continue;
                        }

                        size_t idx = baseX + z;
                        int u = m_u[idx];
                        int v = m_v[idx];

                        int neighbors = m_u[baseXp + z] + m_u[baseXm + z]
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

                        // Shell forcing around SHELL_R.
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

                        // Boundary absorption.
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

                        m_uNext[idx] = u_new;
                        m_vNext[idx] = v_new;
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
            const unsigned size = EL + 1;
            m_profile.assign(size, 0.0f);
            m_andMask.assign(size, 0.0f);

            std::vector<int64_t> profSum(size, 0);
            std::vector<int> counts(size, 0);
            std::vector<int> andCounts(size, 0);

            const automaton::Cell* lattice = automaton::lattice_curr.data();
            const unsigned int pulseTol = ((EL * EL + 150) / 300) > 0
                                            ? ((EL * EL + 150) / 300)
                                            : 1;
            const unsigned int pulseR2 = m_curPulseR2;

            for (unsigned x = 0; x < EL; ++x)
            {
                size_t rowBase = ((size_t)x * EL) * EL;
                for (unsigned y = 0; y < EL; ++y)
                {
                    size_t base = rowBase + (size_t)y * EL;
                    const automaton::Cell* row = lattice + base;
                    for (unsigned z = 0; z < EL; ++z)
                    {
                        const automaton::Cell& c = row[z];
                        int r = c.r;
                        if (r < 0 || (unsigned)r >= size)
                            continue;

                        size_t idx = base + z;
                        profSum[r] += m_u[idx];
                        counts[r]++;

                        unsigned int d = (c.r2 > pulseR2) ? (c.r2 - pulseR2)
                                                          : (pulseR2 - c.r2);
                        bool active = (d <= pulseTol);
                        if (m_v[idx] > 0 && active)
                            andCounts[r]++;
                    }
                }
            }

            float maxU = 1.0f;
            float maxA = 1.0f;
            for (unsigned i = 0; i < size; ++i)
            {
                float p = (counts[i] > 0)
                            ? static_cast<float>(profSum[i]) / static_cast<float>(counts[i])
                            : 0.0f;
                m_profile[i] = p;
                m_andMask[i] = static_cast<float>(andCounts[i]);

                if (p > maxU) maxU = p;
                if (andCounts[i] > maxA) maxA = static_cast<float>(andCounts[i]);
            }

            if (maxU < 1.0f) maxU = 1.0f;
            if (maxA < 1.0f) maxA = 1.0f;

            for (unsigned i = 0; i < size; ++i)
            {
                m_profile[i] /= maxU;
                m_andMask[i] /= maxA;
            }
        }
    };

    SincWave g_sincWave;
}

namespace sinc_overlay
{
    // Double buffers for the HUD overlay.
    static std::array<std::vector<float>, 2> profileBufs;
    static std::array<std::vector<float>, 2> andMaskBufs;

    static std::atomic<int>     frontIdx{0};
    static std::atomic<unsigned> pulseRadius{0};
    static std::atomic<bool>    readyFlag{false};

    const std::vector<float>& profile()
    {
        return profileBufs[frontIdx.load(std::memory_order_acquire)];
    }

    const std::vector<float>& andMask()
    {
        return andMaskBufs[frontIdx.load(std::memory_order_acquire)];
    }

    unsigned currentRadius() { return pulseRadius.load(std::memory_order_acquire); }
    bool ready()             { return readyFlag.load(std::memory_order_acquire); }

    void update(unsigned selectedW)
    {
        (void)selectedW; // overlay always uses the sinc wave on layer w = 0

        using automaton::EL;
        using automaton::lattice_curr;

        if (EL == 0 || lattice_curr.empty())
            return;

        g_sincWave.run();

        int backIdx = 1 - frontIdx.load(std::memory_order_relaxed);
        profileBufs[backIdx] = g_sincWave.profile();
        andMaskBufs[backIdx] = g_sincWave.andMask();

        pulseRadius.store(g_sincWave.currentRadius(), std::memory_order_release);
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
    unsigned int pulse_r2 = automaton::pulse_from_time(centreCell.t);

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
        unsigned int pulse_r = (unsigned int)sqrt((double)pulse_r2);
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