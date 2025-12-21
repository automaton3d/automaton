#include "simulation.h"
#include "automaton_compute.h"
#include "automaton_constants.h"
#include <cuda_runtime.h>
#include <vector>
#include <cstdio>

// DEBUG


// Forward declarations from initSim.cpp
namespace automaton {
    extern std::vector<Cell> lattice_curr;
    extern std::vector<Cell> lattice_draft;
    extern std::vector<Cell> lattice_mirror;
    
    void initGeneral();
    void initMomentum();
    void initSpirals();
    void initSine2();
    void replicate();
    void relocateAllWRandom();
}

/**
 * initGPULatticeState_HostImpl - Initialize GPU lattices using CPU initialization
 * 
 * This function reuses ALL the initialization logic from initSim.cpp to ensure
 * perfect consistency between CPU and GPU modes.
 * 
 * Strategy:
 * 1. Call the existing CPU initialization functions (initGeneral, initMomentum, etc.)
 * 2. These functions populate automaton::lattice_curr in host memory
 * 3. Copy the initialized data to GPU
 */
extern "C" void initGPULatticeState_HostImpl(
    automaton::Cell* d_lattice_curr_ptr,
    automaton::Cell* d_lattice_draft_ptr,
    automaton::Cell* d_lattice_mirror_ptr,
    unsigned EL,
    unsigned W_USED)
{
    printf("=== GPU Lattice Initialization (Unified Path) ===\n");
    printf("EL=%u, W_USED=%u\n", EL, W_USED);
    
    // 1. Calculate sizes
    unsigned long long L3 = (unsigned long long)EL * EL * EL;
    unsigned long long BLOCK = L3 * W_USED;
    size_t total_bytes = (size_t)BLOCK * sizeof(automaton::Cell);
    
    printf("Total cells: %llu (%zu MB)\n", BLOCK, total_bytes / (1024*1024));
    
    // 2. Verify GPU pointers
    if (!d_lattice_curr_ptr || !d_lattice_draft_ptr || !d_lattice_mirror_ptr) {
        fprintf(stderr, "ERROR: GPU pointers are null! Aborting.\n");
        return;
    }
    
    // 3. Ensure host lattices are properly sized
    if (automaton::lattice_curr.size() != BLOCK) {
        printf("Resizing host lattices from %zu to %llu cells\n", 
               automaton::lattice_curr.size(), BLOCK);
        automaton::lattice_curr.resize(BLOCK);
        automaton::lattice_draft.resize(BLOCK);
        automaton::lattice_mirror.resize(BLOCK);
    }
    
    // 4. Run the SAME initialization sequence as CPU mode
    printf("Step 1/5: initGeneral...\n");
    automaton::initGeneral();
    
    printf("Step 2/5: initMomentum...\n");
    automaton::initMomentum();
    
    printf("Step 3/5: initSpirals...\n");
    automaton::initSpirals();
    
    printf("Step 4/5: initSine2...\n");
    automaton::initSine2();
    
    printf("Step 5/5: replicate...\n");
    automaton::replicate();
    
    automaton::relocateAllWRandom();

    // Also copy to mirror (as done in initSimulation step 5)
    std::copy(automaton::lattice_curr.begin(),
              automaton::lattice_curr.begin() + BLOCK,
              automaton::lattice_mirror.begin());
    
    printf("Host initialization completed. Copying to GPU...\n");
    
    // 5. Copy initialized data to GPU
    cudaError_t err;
    
    err = cudaMemcpy(d_lattice_curr_ptr, automaton::lattice_curr.data(), 
                     total_bytes, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA Memcpy ERROR (curr): %s\n", cudaGetErrorString(err));
        return;
    }
    
    err = cudaMemcpy(d_lattice_draft_ptr, automaton::lattice_draft.data(), 
                     total_bytes, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA Memcpy ERROR (draft): %s\n", cudaGetErrorString(err));
        return;
    }
    
    err = cudaMemcpy(d_lattice_mirror_ptr, automaton::lattice_mirror.data(), 
                     total_bytes, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA Memcpy ERROR (mirror): %s\n", cudaGetErrorString(err));
        return;
    }
    
    cudaDeviceSynchronize();
    
    printf("=== GPU Lattice Initialization Complete ===\n");
    printf("Data successfully copied to device.\n");
}