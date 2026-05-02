/*
 * bridge_cuda.cu
 */

#include "cuda_runtime.h"
#include "cuda_sim_optimized.h"
#include "cuda/cuda_common.h"
#include <cstdio>

/*
#include "GUI.h"
#include "model/simulation.h"
#include "layers.h"
*/
#include "voxel.h"
#include <vector>
#include <cstdint>
#include <memory>
#include <cstring>

#ifdef USE_CUDA
#include "cuda_sim_optimized.h"
static bool useCuda = false;
#endif

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

// ===================================================================
// Tomografia: verifica se o voxel deve ser desenhado
// ===================================================================
static bool isVisibleInTomogram(unsigned x, unsigned y, unsigned z)
{
  if (!tomoEnable || !tomoEnable->getState())
    return true;

  if (tomoDirs.empty())
    return true;

  if (tomoDirs[0].isSelected()) return z == tomo_z; // XY plane
  if (tomoDirs[1].isSelected()) return x == tomo_x; // YZ plane
  if (tomoDirs[2].isSelected()) return y == tomo_y; // ZX plane

  return true;
}

#ifdef USE_CUDA
// ===================================================================
// CUDA Helper: Convert CPU Cell to CellDevice
// ===================================================================
static void convertCellToCellDevice(const automaton::Cell& src, CellDevice& dst)
{
    dst.ch = src.ch;
    dst.pB = src.pB ? 1 : 0;
    dst.sB = src.sB ? 1 : 0;
    dst.a = src.a;
    
    // Copy x[3] - note: Cell.x is [4] but CellDevice.x is [3]
    for (int i = 0; i < 3; ++i) {
        dst.x[i] = src.x[i];
    }
    
    dst.d = src.d;
    dst.phiB = src.phiB ? 1 : 0;
    dst.t = src.t;
    dst.f = src.f;
    
    // Copy c[3] to c[4] - CellDevice has c[4]
    for (int i = 0; i < 3; ++i) {
        dst.c[i] = src.c[i];
    }
    dst.c[3] = 0; // Padding
    
    dst.k = src.k;
    dst.s2B = src.s2B ? 1 : 0;
    dst.kB = src.kB ? 1 : 0;
    dst.bB = src.bB ? 1 : 0;
    dst.hB = src.hB ? 1 : 0;
    dst.cB = src.cB ? 1 : 0;
}

// ===================================================================
// CUDA Helper: Convert CellDevice to CPU Cell
// ===================================================================
static void convertCellDeviceToCell(const CellDevice& src, automaton::Cell& dst)
{
    dst.ch = src.ch;
    dst.pB = src.pB != 0;
    dst.sB = src.sB != 0;
    dst.a = src.a;
    
    // Copy x[3] to x[4]
    for (int i = 0; i < 3; ++i) {
        dst.x[i] = src.x[i];
    }
    dst.x[3] = 0; // Cell has x[4], initialize 4th element
    
    dst.d = src.d;
    dst.phiB = src.phiB != 0;
    dst.t = src.t;
    dst.f = src.f;
    
    // Copy c[4] to c[3]
    for (int i = 0; i < 3; ++i) {
        dst.c[i] = src.c[i];
    }
    
    dst.k = src.k;
    dst.s2B = src.s2B != 0;
    dst.kB = src.kB != 0;
    dst.bB = src.bB != 0;
    dst.hB = src.hB != 0;
    dst.cB = src.cB != 0;
}

// ===================================================================
// CUDA initialization
// ===================================================================
bool initializeCudaSimulation()
{
    if (!isCudaAvailable()) {
        fprintf(stderr, "CUDA not available - falling back to CPU\n");
        return false;
    }
    
    // Initialize CUDA simulation with lattice dimensions
    if (!initCudaSimulation(automaton::EL, automaton::W_USED)) {
        fprintf(stderr, "Failed to initialize CUDA simulation\n");
        return false;
    }

    setCudaConstants(automaton::EL, automaton::W_USED, automaton::RMAX);
    
    // Convert and upload initial lattice
    size_t totalCells = (size_t)automaton::EL * automaton::EL * 
                        automaton::EL * automaton::W_USED;
    std::vector<CellDevice> deviceCells(totalCells);
    
    printf("Converting %zu cells to device format...\n", totalCells);
    for (size_t i = 0; i < totalCells; i++) {
        convertCellToCellDevice(automaton::lattice_curr[i], deviceCells[i]);
    }
    
    printf("Uploading lattice to CUDA...\n");
    if (!uploadLatticeToCuda(deviceCells.data(), totalCells)) {
        fprintf(stderr, "Failed to upload lattice to CUDA\n");
        cudaCleanup();
        return false;
    }
    
    printf("CUDA simulation initialized successfully\n");
    return true;
}

// ===================================================================
// CUDA simulation step - wrapper that calls the GPU kernel
// ===================================================================
void cudaSimulationStepWrapper()
{
    // This calls the CUDA kernel with all the simulation parameters
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
        automaton::RMAX
    );
    
    size_t totalCells = (size_t)automaton::EL * automaton::EL *
                    automaton::EL * automaton::W_USED;

    dim3 blockSize(256);
    dim3 gridSize((totalCells + blockSize.x - 1) / blockSize.x);

    updateMirrorKernel<<<gridSize, blockSize>>>(d_lattice_curr,
                                            d_lattice_mirror,
                                            totalCells);

    cudaDeviceSynchronize();

    std::vector<CellDevice> deviceCells(totalCells);
    
    if (downloadLatticeFromCuda(deviceCells.data(), totalCells)) {
        for (size_t i = 0; i < totalCells; i++) {
            convertCellDeviceToCell(deviceCells[i], automaton::lattice_curr[i]);
        }
    } else {
        fprintf(stderr, "Warning: Failed to download lattice from CUDA\n");
    }
}

// ===================================================================
// updateBuffer() - CUDA version
// ===================================================================
void updateBufferCuda()
{
    unsigned selectedW = (framework::layerList && framework::layerList.get())
                         ? framework::layerList->getSelected()
                         : 0u;
    
    // Generate voxels on GPU
    cudaUpdateVoxelsLayer(selectedW);
    
    // Get mapped voxels (zero-copy)
    uint32_t* gpuVoxels = getMappedVoxels();
    
    if (gpuVoxels != nullptr) {
        size_t idx = 0;
        
        // Apply tomography filter (CPU side)
        for (unsigned x = 0; x < automaton::EL; ++x)
        for (unsigned y = 0; y < automaton::EL; ++y)
        for (unsigned z = 0; z < automaton::EL; ++z)
        {
            if (!isVisibleInTomogram(x, y, z)) {
                voxels[idx++] = 0x00000000u;
            } else {
                voxels[idx++] = gpuVoxels[(x * automaton::EL + y) * automaton::EL + z];
            }
        }
    } else {
        // Fallback to CPU rendering if GPU voxels not available
        updateBufferCPU();
    }
}
#endif

// ===================================================================
// updateBuffer() - CPU version (original)
// ===================================================================
void updateBufferCPU()
{
    unsigned selectedW = (framework::layerList && framework::layerList.get())
                         ? framework::layerList->getSelected()
                         : 0u;
    size_t idx = 0;

    for (unsigned x = 0; x < automaton::EL; ++x)
    for (unsigned y = 0; y < automaton::EL; ++y)
    for (unsigned z = 0; z < automaton::EL; ++z)
    {
        if (!isVisibleInTomogram(x, y, z))
        {
            voxels[idx++] = 0x00000000u;
            continue;
        }

        const automaton::Cell& cell = automaton::getCell(
            automaton::lattice_curr, x, y, z, selectedW);

        uint32_t color = 0x00000000u;

        if (cell.t == cell.d)
        {
            if (cell.a == automaton::W_USED)
                color = makeColor(255, 80, 80, 255);
            else if (cell.t == 0)
                color = makeColor(80, 255, 80, 255);
            else
                color = makeColor(200, 200, 255, 255);
        }

        voxels[idx++] = color;
    }
}

// ===================================================================
// Public API - dispatch to CPU or CUDA
// ===================================================================
#ifndef USE_CUDA
// CPU-only version
void automaton::updateBuffer()
{
    updateBufferCPU();
}
#else
// CUDA-enabled version
void automaton::updateBuffer()
{
    if (useCuda) {
        updateBufferCuda();
    } else {
        updateBufferCPU();
    }
}

// ===================================================================
// Enable/disable CUDA at runtime
// ===================================================================
namespace automaton
{
    bool tryEnableCuda()
    {
        if (useCuda) {
            printf("CUDA already enabled\n");
            return true;
        }
        
        bool success = initializeCudaSimulation();
        if (success) {
            useCuda = true;
            printf("✓ CUDA acceleration ENABLED\n");
        } else {
            printf("✗ CUDA acceleration not available - using CPU\n");
        }
        return success;
    }
    
    void disableCuda()
    {
        if (!useCuda) {
            printf("CUDA already disabled\n");
            return;
        }
        
        // Download final state back to CPU
        size_t totalCells = (size_t)EL * EL * EL * W_USED;
        std::vector<CellDevice> deviceCells(totalCells);
        
        printf("Downloading final state from GPU...\n");
        if (downloadLatticeFromCuda(deviceCells.data(), totalCells)) {
            for (size_t i = 0; i < totalCells; i++) {
                convertCellDeviceToCell(deviceCells[i], lattice_curr[i]);
            }
            printf("State downloaded successfully\n");
        } else {
            fprintf(stderr, "Warning: Failed to download final state\n");
        }
        
        cudaCleanup();
        useCuda = false;
        printf("✓ CUDA acceleration DISABLED\n");
    }
    
    bool isCudaEnabled()
    {
        return useCuda;
    }
}
#endif // USE_CUDA