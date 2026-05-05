#pragma once

#include <cstdint>
#include <vector>

// Forward declare the automaton namespace
namespace automaton {
    class Cell;
}

// ================================================
// CellDevice já está definida em cuda_common.h
// Não redefina aqui para evitar erro de redefinição
// ================================================

// Inclua o header que contém a definição real
#include "cuda_common.h"

// CUDA Host API Functions
// Estas funções são implementadas em cuda_automaton.cu

// Check if CUDA is available on this system
bool isCudaAvailable();

// Initialize CUDA simulation with given lattice dimensions
bool initCudaSimulation(unsigned EL, unsigned W_USED);

// Upload lattice data from CPU to GPU
bool uploadLatticeToCuda(::CellDevice* hostCells, size_t totalCells);

// Execute one simulation step on GPU
void cudaSimulationStep(
    unsigned CONVOL, unsigned GSLOT_Z,
    unsigned SLOT1, unsigned SLOT2, unsigned SLOT3, 
    unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6, 
    unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE, 
    unsigned FLOOD, unsigned FRAME, unsigned RMAX, int scenario
);

// Update voxel rendering for a specific layer
void cudaUpdateVoxelsLayer(unsigned selectedW);

// Get pointer to mapped voxel memory (for zero-copy access)
uint32_t* getMappedVoxels();

// Download lattice data from GPU to CPU
bool downloadLatticeFromCuda(::CellDevice* hostCells, size_t totalCells);

// Clean up CUDA resources
void cudaCleanup();

// Internal functions (used by cuda_automaton.cu)
void free_cuda_memory();
bool swap_lattices_gpu();

// Note: convertToDevice and convertToHost are implemented in cuda_automaton.cu
// in the automaton namespace but are only used internally by bridge.cpp