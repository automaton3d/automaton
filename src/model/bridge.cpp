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
#include <algorithm>
#include <cstdio>
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
//
// The 2-D HUD overlay is a pure read-out of automaton::lattice_curr.
// The self-contained SincWave simulation and background thread were removed;
// only the main CA dynamics drives the display.
// ============================================================

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

    // Persistent histograms for the red r·sin(r) accumulation.
    static std::vector<int64_t> g_andAcc;
    static std::vector<int64_t> g_activeCount;

    // Running peak of the signed |u| profile, used as the per-frame
    // display reference so the overlay stays visible at every grid size.
    static int64_t g_uPeak = 1;

    // Sliding window for the yellow peak-history curve.
    static std::vector<float> g_peakHistory;
    constexpr size_t PEAK_HIST_SIZE = 128;

    const std::vector<float>& profile()     { return profileBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& triggerRate() { return triggerRateBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& peakHistory() { return peakHistoryBufs[frontIdx.load(std::memory_order_acquire)]; }
    const std::vector<float>& andMask()     { return andMaskBufs[frontIdx.load(std::memory_order_acquire)]; }
    unsigned currentRadius() { return pulseRadius.load(std::memory_order_acquire); }
    unsigned graphSize()     { return gGraphSize.load(std::memory_order_acquire); }
    bool ready()             { return readyFlag.load(std::memory_order_acquire); }

    void update(unsigned selectedW)
    {
        using automaton::Cell;
        using automaton::getCell;

        if (automaton::EL == 0 || automaton::lattice_curr.empty())
            return;

        unsigned graphSize = automaton::RMAX + 1;
        if (graphSize < 2)
            return;

        if (g_andAcc.size() != graphSize)
        {
            g_andAcc.assign(graphSize, 0);
            g_activeCount.assign(graphSize, 0);
            g_uPeak = 1;
            g_peakHistory.assign(PEAK_HIST_SIZE, 0.0f);
        }

        unsigned cx = automaton::CENTER;
        unsigned cy = automaton::CENTER;
        unsigned cz = automaton::CENTER;
        if (selectedW < (unsigned)automaton::lcenters.size())
        {
            cx = automaton::lcenters[selectedW][0];
            cy = automaton::lcenters[selectedW][1];
            cz = automaton::lcenters[selectedW][2];
        }

        std::vector<int64_t> sumU(graphSize, 0);
        std::vector<int64_t> cntU(graphSize, 0);

        for (unsigned x = 0; x < automaton::EL; ++x)
        for (unsigned y = 0; y < automaton::EL; ++y)
        for (unsigned z = 0; z < automaton::EL; ++z)
        {
            const Cell& c = getCell(automaton::lattice_curr, x, y, z, selectedW);
            if (c.r < 0 || c.r > (int)automaton::RMAX)
                continue;
            unsigned r = (unsigned)c.r;
            if (r >= graphSize)
                continue;

            sumU[r] += (int64_t)c.u;
            cntU[r]++;

            if (c.active)
            {
                g_activeCount[r]++;
                if (c.sB)
                    g_andAcc[r]++;
            }
        }

        // Per-shell average of u(r) (signed) and running peak.
        std::vector<int64_t> avgU(graphSize, 0);
        for (unsigned r = 0; r < graphSize; ++r)
        {
            if (cntU[r] > 0)
            {
                avgU[r] = sumU[r] / cntU[r];
                int64_t a = avgU[r];
                if (a < 0) a = -a;
                if (a > g_uPeak) g_uPeak = a;
            }
        }
        if (g_uPeak < 1) g_uPeak = 1;

        // Use the running peak as a per-frame reference so the overlay stays
        // visible even when the absolute |u| is far below SHELL_TARGET*3.
        float peakRef = (float)std::max<int64_t>(1, g_uPeak);

        // Green: envelope |u(r)| / (running peak with headroom) so it sits
        // below the cyan trigger-rate curve and both remain visible.
        // Cyan: signed profile / running peak (trigger rate).
        float greenPeakRef = peakRef * 1.25f;
        if (greenPeakRef < 1.0f) greenPeakRef = 1.0f;

        std::vector<float> profile(graphSize, 0.0f);
        std::vector<float> triggerRate(graphSize, 0.0f);
        for (unsigned r = 0; r < graphSize; ++r)
        {
            int64_t au = avgU[r];
            if (au < 0) au = -au;
            profile[r]     = (float)au / greenPeakRef;
            triggerRate[r] = (float)avgU[r] / peakRef;
        }

        // Yellow: raw running-peak history, auto-normalized like mytry.c.
        float peakLevelRaw = (float)g_uPeak;
        for (size_t i = 0; i + 1 < g_peakHistory.size(); ++i)
            g_peakHistory[i] = g_peakHistory[i + 1];
        if (!g_peakHistory.empty())
            g_peakHistory.back() = peakLevelRaw;

        // Build a normalized copy for display without overwriting the raw window.
        std::vector<float> peakHistoryNorm = g_peakHistory;
        float maxRaw = 1.0f;
        for (float v : peakHistoryNorm)
            if (v > maxRaw) maxRaw = v;
        maxRaw += maxRaw / 8.0f;
        for (float& v : peakHistoryNorm)
            v /= maxRaw;

        // Red: accumulated AND counts per shell (r·sin(r) mask), absolute counts.
        std::vector<float> andMask(graphSize, 0.0f);
        int64_t maxAnd = 1;
        for (unsigned r = 0; r < graphSize; ++r)
        {
            if (g_andAcc[r] > maxAnd) maxAnd = g_andAcc[r];
        }
        if (maxAnd < 1) maxAnd = 1;
        for (unsigned r = 0; r < graphSize; ++r)
            andMask[r] = (float)g_andAcc[r] / (float)maxAnd;

        unsigned pulseR = (unsigned)::isqrt((int)automaton::pulse_from_time(automaton::pulse_tick));

        int backIdx = 1 - frontIdx.load(std::memory_order_relaxed);
        profileBufs[backIdx]     = std::move(profile);
        andMaskBufs[backIdx]     = std::move(andMask);
        triggerRateBufs[backIdx] = std::move(triggerRate);
        peakHistoryBufs[backIdx] = std::move(peakHistoryNorm);

        pulseRadius.store(pulseR, std::memory_order_release);
        gGraphSize.store(graphSize, std::memory_order_release);
        frontIdx.store(backIdx, std::memory_order_release);
        readyFlag.store(true, std::memory_order_release);

        // DEBUG: throttle a snapshot of the overlay read-out.
        static int overlayReport = 0;
        if (++overlayReport % 60 == 0) {
            const Cell& c = automaton::getCell(automaton::lattice_curr, automaton::CENTER, automaton::CENTER, automaton::CENTER, selectedW);
            const auto& prBuf = profileBufs[backIdx];
            float pr0   = prBuf.empty() ? -1.0f : prBuf[0];
            float prMid = prBuf.empty() ? -1.0f : prBuf[graphSize / 2];
            printf("DEBUG overlay #%d selectedW=%u g_uPeak=%lld center u=%d v=%d active=%u profile[0]=%.3f profile[mid]=%.3f pulseR=%u\n",
                   overlayReport, selectedW, g_uPeak, c.u, c.v, c.active,
                   pr0, prMid, pulseR);
        }
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
    dst.pB  = src.pB ? 1 : 0;    dst.sB  = src.sB ? 1 : 0;
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
    automaton::pulse_tick++;
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
        scenario,
        automaton::pulse_tick
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