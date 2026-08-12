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
        using automaton::Cell;
        using automaton::EL;
        using automaton::W_USED;
        using automaton::lattice_curr;
        using automaton::getCell;
        using automaton::CENTER;
        using automaton::pulse_from_time;

        if (EL == 0 || selectedW >= W_USED || lattice_curr.empty())
            return;

        const unsigned size = EL + 1;

        int backIdx = 1 - frontIdx.load(std::memory_order_relaxed);
        std::vector<float>& profileBack = profileBufs[backIdx];
        std::vector<float>& andMaskBack = andMaskBufs[backIdx];

        profileBack.assign(size, 0.0f);
        andMaskBack.assign(size, 0.0f);
        std::vector<int> counts(size, 0);

        const Cell& centre = getCell(lattice_curr, CENTER, CENTER, CENTER, selectedW);
        unsigned int pulse_r2 = pulse_from_time(centre.t);
        unsigned int pulse_r  = (unsigned int)std::sqrt((double)pulse_r2);

        for (unsigned x = 0; x < EL; ++x)
        for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
        {
            const Cell& c = getCell(lattice_curr, x, y, z, selectedW);

            if (c.r2 == INF_R2)
                continue;

            int r = c.r;
            if (r < 0 || (unsigned)r >= size)
                continue;

            profileBack[r] += static_cast<float>(c.u);
            if (c.sB && c.active)
                andMaskBack[r] += 1.0f;
            counts[r]++;
        }

        float maxU = 1.0f;
        float maxA = 1.0f;

        for (unsigned i = 0; i < size; ++i)
        {
            if (counts[i] > 0)
                profileBack[i] /= static_cast<float>(counts[i]);

            float au = std::fabs(profileBack[i]);
            if (au > maxU) maxU = au;
            if (andMaskBack[i] > maxA) maxA = andMaskBack[i];
        }

        if (maxU < 1.0f) maxU = 1.0f;
        if (maxA < 1.0f) maxA = 1.0f;

        for (unsigned i = 0; i < size; ++i)
        {
            profileBack[i] /= maxU;       // now in [-1, 1]
            andMaskBack[i] /= maxA;       // now in [ 0, 1]
        }

        pulseRadius.store(pulse_r, std::memory_order_release);
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