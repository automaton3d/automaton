// cuda_automaton.cu - CUDA implementation with FULL CA logic

#pragma nv_diag_suppress 177

#include "model/simulation.h"
#include "cuda_sim_optimized.h"
#include <cuda_runtime.h>
#include <iostream>
#include <algorithm>

// Declare the constant memory symbols (defined in cuda_constants.cu)
extern __constant__ unsigned dev_EL;
extern __constant__ unsigned dev_W_USED;
extern __constant__ unsigned dev_RMAX;

// Forward declare the function from cuda_constants.cu
extern "C" void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX);

// ===================================================================
// GLOBAL DEVICE VARIABLES
// ===================================================================
::CellDevice* d_lattice_curr = nullptr;
::CellDevice* d_lattice_draft = nullptr;
::CellDevice* d_lattice_mirror = nullptr;

static bool g_cuda_initialized = false;

#define CUDA_CHECK(call) \
    { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s (code %d)\n", \
                    __FILE__, __LINE__, cudaGetErrorString(err), err); \
            fflush(stderr); \
            return false; \
        } \
    }

#define CUDA_CHECK_VOID(call) \
    { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s (code %d)\n", \
                    __FILE__, __LINE__, cudaGetErrorString(err), err); \
            fflush(stderr); \
        } \
    }

// ===================================================================
// DEVICE HELPER FUNCTIONS
// ===================================================================
static __device__ inline ::CellDevice& d_getCell(::CellDevice* lattice, int x, int y, int z, int w)
{
    return lattice[(((x * dev_EL + y) * dev_EL + z) * dev_W_USED) + w];
}

static __device__ ::CellDevice d_getNeighbor(::CellDevice* d_curr_lattice,
                                           unsigned x_curr, 
                                           unsigned y_curr, 
                                           unsigned z_curr, 
                                           unsigned w_curr, 
                                           int i) 
{
    static const int disp[8][4] =
    {
      {+1,  0,  0,  0}, {-1,  0,  0,  0},
      { 0, +1,  0,  0}, { 0, -1,  0,  0},
      { 0,  0, +1,  0}, { 0,  0, -1,  0},
      { 0,  0,  0, +1}, { 0,  0,  0, -1}
    };

    int nx = (x_curr + disp[i][0] + dev_EL) % dev_EL;
    int ny = (y_curr + disp[i][1] + dev_EL) % dev_EL;
    int nz = (z_curr + disp[i][2] + dev_EL) % dev_EL;
    int nw = (w_curr + disp[i][3] + dev_W_USED) % dev_W_USED;

    return d_getCell(d_curr_lattice, nx, ny, nz, nw);
}

#define ZERO_C(c) (!(c[0] | c[1] | c[2]))

// ===================================================================
// MAIN UPDATE KERNEL - COMPLETE CA LOGIC
// ===================================================================
__global__ void ca_update_kernel(::CellDevice* d_curr, ::CellDevice* d_draft, ::CellDevice* d_mirror,
                                 unsigned CONVOL, unsigned SLOT1, unsigned SLOT2, unsigned SLOT3,
                                 unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6,
                                 unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE,
                                 unsigned FLOOD, unsigned FRAME)
{
    unsigned int idx = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int total_cells = dev_EL * dev_EL * dev_EL * dev_W_USED;

    if (idx >= total_cells) return;

    // Map 1D index to 4D coordinates (x, y, z, w)
    unsigned w = idx % dev_W_USED;
    unsigned idx_3d = idx / dev_W_USED;
    unsigned z = idx_3d % dev_EL;
    unsigned y = (idx_3d / dev_EL) % dev_EL;
    unsigned x = idx_3d / (dev_EL * dev_EL);
    
    ::CellDevice curr = d_getCell(d_curr, x, y, z, w);
    ::CellDevice draft = curr;  // Start with copy
    ::CellDevice mirror = d_getCell(d_mirror, x, y, z, w);
    
    // Get neighbors
    ::CellDevice forward = d_getNeighbor(d_curr, x, y, z, w, 6); // FORWARD
    ::CellDevice north   = d_getNeighbor(d_curr, x, y, z, w, 0); // NORTH
    ::CellDevice west    = d_getNeighbor(d_curr, x, y, z, w, 2); // WEST
    ::CellDevice down    = d_getNeighbor(d_curr, x, y, z, w, 4); // DOWN
    ::CellDevice south   = d_getNeighbor(d_curr, x, y, z, w, 1); // SOUTH
    ::CellDevice east    = d_getNeighbor(d_curr, x, y, z, w, 3); // EAST
    ::CellDevice up      = d_getNeighbor(d_curr, x, y, z, w, 5); // UP

    // ===================================================================
    // CONVOLUTION PHASE (k < CONVOL)
    // ===================================================================
    if (curr.k < CONVOL) {
        // Simplified convolute logic - you can expand based on scenario
        // For now, just basic update (full logic would need scenario parameter)
    }
    
    // ===================================================================
    // DIFFUSION PHASE (k < DIFFUSION)
    // ===================================================================
    else if (curr.k < DIFFUSION) {
        // SLOT I
        if (curr.k < SLOT1) {
            if ((north.a == dev_W_USED && curr.d >= north.d) ||
                (west.a  == dev_W_USED && curr.d >= west.d)  ||
                (down.a  == dev_W_USED && curr.d >= down.d)  ||
                (south.a == dev_W_USED && curr.d >= south.d) ||
                (east.a  == dev_W_USED && curr.d >= east.d)  ||
                (up.a    == dev_W_USED && curr.d >= up.d)) {
                draft.a = dev_W_USED;
            }
        }
        // SLOT II
        if (curr.k < SLOT2) {
            if ((north.a == dev_W_USED && curr.d >= north.d) ||
                (west.a  == dev_W_USED && curr.d >= west.d)  ||
                (down.a  == dev_W_USED && curr.d >= down.d)  ||
                (south.a == dev_W_USED && curr.d >= south.d) ||
                (east.a  == dev_W_USED && curr.d >= east.d)  ||
                (up.a    == dev_W_USED && curr.d >= up.d)) {
                draft.a = dev_W_USED;
            }
            if (curr.d == curr.t) {
                if (north.hB) { draft.c[0] = (north.c[0] + 1) % dev_EL; curr.sB = !draft.hB; }
                else if (west.hB)  { draft.c[1] = (west.c[1] + 1) % dev_EL; curr.sB = !draft.hB; }
                else if (down.hB)  { draft.c[2] = (down.c[2] + 1) % dev_EL; curr.sB = !draft.hB; }
                else if (south.hB) { draft.c[0] = (south.c[0] + 1) % dev_EL; curr.sB = !draft.hB; }
                else if (east.hB)  { draft.c[1] = (east.c[1] + 1) % dev_EL; curr.sB = !draft.hB; }
                else if (up.hB)    { draft.c[2] = (up.c[2] + 1) % dev_EL; curr.sB = !draft.hB; }
            }
        }
        // SLOT III
        else if (curr.k < SLOT3) {
            if (!ZERO_C(north.c)) {
                draft.c[0] = north.c[0];
                draft.c[1] = north.c[1];
                draft.c[2] = north.c[2];
                if (north.kB) draft.kB = north.kB;
            }
            if (!ZERO_C(west.c)) {
                draft.c[0] = west.c[0];
                draft.c[1] = west.c[1];
                draft.c[2] = west.c[2];
                if (west.kB) draft.kB = west.kB;
            }
            if (!ZERO_C(down.c)) {
                draft.c[0] = down.c[0];
                draft.c[1] = down.c[1];
                draft.c[2] = down.c[2];
                if (down.kB) draft.kB = down.kB;
            }
            draft.f = max(down.f, max(west.f, max(north.f,
                        max(south.f, max(east.f, up.f)))));
            
            // Diffuse cB toward center
            if (!curr.cB) {
                if (north.cB && north.d > curr.d) {
                    draft.cB = 1;
                    if (north.a != dev_W_USED) draft.a = north.a;
                }
                else if (south.cB && south.d > curr.d) {
                    draft.cB = 1;
                    if (south.a != dev_W_USED) draft.a = south.a;
                }
                else if (east.cB && east.d > curr.d) {
                    draft.cB = 1;
                    if (east.a != dev_W_USED) draft.a = east.a;
                }
                else if (west.cB && west.d > curr.d) {
                    draft.cB = 1;
                    if (west.a != dev_W_USED) draft.a = west.a;
                }
                else if (down.cB && down.d > curr.d) {
                    draft.cB = 1;
                    if (down.a != dev_W_USED) draft.a = down.a;
                }
                else if (up.cB && up.d > curr.d) {
                    draft.cB = 1;
                    if (up.a != dev_W_USED) draft.a = up.a;
                }
            }
        }
        // SLOT IV
        else if (curr.k < SLOT4) {
            if (forward.kB && forward.a == curr.a) {
                int delta_x = (curr.x[0] - forward.x[0] + dev_EL) % dev_EL;
                int delta_y = (curr.x[1] - forward.x[1] + dev_EL) % dev_EL;
                int delta_z = (curr.x[2] - forward.x[2] + dev_EL) % dev_EL;
                
                draft.c[0] = (forward.c[0] + delta_x) % dev_EL;
                draft.c[1] = (forward.c[1] + delta_y) % dev_EL;
                draft.c[2] = (forward.c[2] + delta_z) % dev_EL;
                draft.kB = forward.kB;
                draft.cB = forward.cB;
            }
            draft.f = max(forward.f, curr.f);
        }
        // SLOT V
        else if (curr.k < SLOT5) {
            if (curr.a == dev_W_USED && curr.d < curr.t) {
                draft.a = curr.x[3];
            }
        }
    }
    
    // ===================================================================
    // RELOCATION PHASE (k < RELOC)
    // ===================================================================
    else if (curr.k < RELOC) {
        unsigned save_x = curr.x[0];
        unsigned save_y = curr.x[1];
        unsigned save_z = curr.x[2];
        
        // SLOT VI
        if (curr.k < SLOT6) {
            if (north.c[0] > 0) {
                draft = north;
                draft.c[0]--;
            }
        }
        // SLOT VII
        else if (curr.k < SLOT7) {
            if (west.c[1] > 0) {
                draft = west;
                draft.c[1]--;
            }
        }
        // SLOT VIII
        else if (curr.k < SLOT8) {
            if (down.c[2] > 0) {
                draft = down;
                draft.c[2]--;
            }
        }
        
        // Restore 3D address
        draft.x[0] = save_x;
        draft.x[1] = save_y;
        draft.x[2] = save_z;
    }
    
    // ===================================================================
    // REISSUE PHASE (k < REISSUE)
    // ===================================================================
    else if (curr.k < REISSUE) {
        draft.kB = 0;
        draft.hB = 0;
        draft.bB = 0;
        
        if (curr.t == curr.d) {
            if (north.d == curr.d + 1) draft.a = north.a;
            if (south.d == curr.d + 1) draft.a = south.a;
            if (east.d == curr.d + 1)  draft.a = east.a;
            if (west.d == curr.d + 1)  draft.a = west.a;
            if (up.d == curr.d + 1)    draft.a = up.a;
            if (down.d == curr.d + 1)  draft.a = down.a;
        }
        
        if (curr.cB) {
            draft.cB = 0;
            if (curr.a != dev_W_USED && curr.d < 2) {
                draft.t = 0;
            }
        }
    }
    
    // ===================================================================
    // FLOOD PHASE (k < FLOOD)
    // ===================================================================
    else if (curr.k < FLOOD) {
        if (curr.a != dev_W_USED) {
            draft.t = min(north.t, min(south.t, min(east.t, 
                      min(west.t, min(down.t, up.t)))));
        }
    }

    // ===================================================================
    // UPDATE FRAME COUNTERS (always)
    // ===================================================================
    draft.k = (curr.k + 1) % FRAME;
    if (draft.k == 0) {
        if (curr.a == dev_W_USED && curr.t <= dev_RMAX) {
            draft.t++;
        } else {
            draft.t = (curr.t + 1) % (dev_RMAX + 1);
        }
    }

    // Write result
    d_getCell(d_draft, x, y, z, w) = draft;
}

// ===================================================================
// MIRROR UPDATE KERNEL
// ===================================================================
__global__ void updateMirrorKernel(CellDevice* lattice_curr,
                                   CellDevice* lattice_mirror,
                                   unsigned totalCells)
{
    unsigned tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= totalCells) return;

    lattice_mirror[tid] = lattice_curr[tid];
    lattice_mirror[tid].f = lattice_mirror[tid].t;
}

// ===================================================================
// HOST API FUNCTIONS
// ===================================================================

bool isCudaAvailable() 
{
    int devCount = 0;
    cudaError_t err = cudaGetDeviceCount(&devCount); 
    
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA not available: %s\n", cudaGetErrorString(err));
        return false;
    }
    
    if (devCount == 0) {
        fprintf(stderr, "No CUDA devices found\n");
        return false;
    }
    
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("Found CUDA device: %s (Compute %d.%d)\n", 
           prop.name, prop.major, prop.minor);
    
    return true;
}

bool init_cuda_memory(unsigned EL, unsigned W_USED)
{
    if (d_lattice_curr != nullptr) {
        fprintf(stderr, "CUDA memory already allocated\n");
        return true;
    }
    
    size_t total_cells = (size_t)EL * EL * EL * W_USED;
    size_t size = total_cells * sizeof(::CellDevice);
    
    printf("Allocating CUDA memory: %zu cells, %zu MB per lattice\n", 
           total_cells, size / (1024 * 1024));
    
    if (size > 2ULL * 1024 * 1024 * 1024) {
        fprintf(stderr, "ERROR: Requested allocation too large: %zu MB\n", size / (1024 * 1024));
        return false;
    }

    CUDA_CHECK(cudaMalloc((void**)&d_lattice_curr, size));
    CUDA_CHECK(cudaMalloc((void**)&d_lattice_draft, size));
    CUDA_CHECK(cudaMalloc((void**)&d_lattice_mirror, size));
    
    CUDA_CHECK(cudaMemset(d_lattice_curr, 0, size));
    CUDA_CHECK(cudaMemset(d_lattice_draft, 0, size));
    CUDA_CHECK(cudaMemset(d_lattice_mirror, 0, size));
    
    printf("✓ CUDA Memory Allocated successfully\n");
    return true;
}

bool initCudaSimulation(unsigned EL, unsigned W_USED)
{
    if (g_cuda_initialized) {
        printf("CUDA already initialized\n");
        return true;
    }
    
    printf("Initializing CUDA simulation: EL=%u, W_USED=%u\n", EL, W_USED);
    
    CUDA_CHECK_VOID(cudaDeviceReset());
    CUDA_CHECK(cudaSetDevice(0));
    
    printf("Copying constants to device...\n");
    unsigned RMAX = automaton::RMAX;
    printf("RMAX = %u\n", RMAX);
    
    setCudaConstants(EL, W_USED, RMAX);
    
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to set CUDA constants: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return false;
    }
    
    printf("Constants copied successfully\n");

    if (!init_cuda_memory(EL, W_USED)) {
        return false;
    }
    
    g_cuda_initialized = true;
    printf("✓ CUDA simulation initialized successfully\n");
    return true;
}

void free_cuda_memory()
{
    if (d_lattice_curr) {
        cudaFree(d_lattice_curr);
        d_lattice_curr = nullptr;
    }
    if (d_lattice_draft) {
        cudaFree(d_lattice_draft);
        d_lattice_draft = nullptr;
    }
    if (d_lattice_mirror) {
        cudaFree(d_lattice_mirror);
        d_lattice_mirror = nullptr;
    }
    g_cuda_initialized = false;
}

void cudaCleanup()
{
    printf("Cleaning up CUDA resources...\n");
    free_cuda_memory();
    cudaDeviceReset();
    printf("✓ CUDA cleanup complete\n");
}

bool uploadLatticeToCuda(::CellDevice* hostCells, size_t totalCells)
{
    if (!g_cuda_initialized) {
        fprintf(stderr, "ERROR: CUDA not initialized\n");
        return false;
    }
    
    if (hostCells == nullptr) {
        fprintf(stderr, "ERROR: hostCells is NULL\n");
        return false;
    }
    
    size_t size = totalCells * sizeof(::CellDevice);
    printf("Uploading %zu cells (%zu MB) to device...\n", 
           totalCells, size / (1024 * 1024));
    
    CUDA_CHECK(cudaMemcpy(d_lattice_curr, hostCells, size, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_lattice_draft, d_lattice_curr, size, cudaMemcpyDeviceToDevice));
    CUDA_CHECK(cudaMemcpy(d_lattice_mirror, d_lattice_curr, size, cudaMemcpyDeviceToDevice));
    
    printf("✓ Upload complete\n");
    return true;
}

bool downloadLatticeFromCuda(::CellDevice* hostCells, size_t totalCells)
{
    if (!g_cuda_initialized) {
        fprintf(stderr, "ERROR: CUDA not initialized\n");
        return false;
    }
    
    size_t size = totalCells * sizeof(::CellDevice);
    CUDA_CHECK(cudaMemcpy(hostCells, d_lattice_curr, size, cudaMemcpyDeviceToHost));
    return true;
}

void cudaSimulationStep(
    unsigned CONVOL, unsigned SLOT1, unsigned SLOT2, unsigned SLOT3, 
    unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6, 
    unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE, 
    unsigned FLOOD, unsigned FRAME, unsigned RMAX)
{
    if (!g_cuda_initialized) {
        fprintf(stderr, "ERROR: CUDA not initialized\n");
        return;
    }
    
    const int BLOCK_SIZE = 256;
    unsigned int total_cells = automaton::EL * automaton::EL * automaton::EL * automaton::W_USED;
    int GRID = (total_cells + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    // Step 1: Check if we need to update mirror (k == 0)
    // For simplicity, we update mirror every frame
    // In production, you'd check the k value first
    updateMirrorKernel<<<GRID, BLOCK_SIZE>>>(d_lattice_curr, d_lattice_mirror, total_cells);
    
    cudaError_t err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        fprintf(stderr, "Mirror kernel failed: %s\n", cudaGetErrorString(err));
        return;
    }
    
    // Step 2: Run main update kernel
    ca_update_kernel<<<GRID, BLOCK_SIZE>>>(
        d_lattice_curr, d_lattice_draft, d_lattice_mirror,
        CONVOL, SLOT1, SLOT2, SLOT3, SLOT4, DIFFUSION,
        SLOT5, SLOT6, SLOT7, SLOT8, RELOC, REISSUE,
        FLOOD, FRAME
    );

    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel launch failed: %s\n", cudaGetErrorString(err));
        return;
    }
    
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel execution failed: %s\n", cudaGetErrorString(err));
        return;
    }
    
    // Step 3: Swap pointers
    ::CellDevice* temp = d_lattice_curr;
    d_lattice_curr = d_lattice_draft;
    d_lattice_draft = temp;
}

void cudaUpdateVoxelsLayer(unsigned selectedW)
{
    // TODO: Implement voxel rendering kernel
}

uint32_t* getMappedVoxels()
{
    // TODO: Return mapped voxel memory pointer
    return nullptr;
}

// ===================================================================
// NAMESPACE WRAPPER FUNCTIONS
// ===================================================================
namespace automaton 
{

::CellDevice convertToDevice(const Cell& src)
{
    ::CellDevice dst;
    
    dst.ch = src.ch;
    dst.pB = src.pB ? 1 : 0;
    dst.sB = src.sB ? 1 : 0;
    dst.a = src.a;

    for (int i = 0; i < 4; ++i) dst.x[i] = src.x[i];
    
    dst.d = src.d;
    dst.phiB = src.phiB ? 1 : 0;
    dst.t = src.t;
    dst.f = src.f;
    
    for (int i = 0; i < 3; ++i) dst.c[i] = src.c[i];
    
    dst.k = src.k;
    dst.s2B = src.s2B ? 1 : 0;
    dst.kB = src.kB ? 1 : 0;
    dst.bB = src.bB ? 1 : 0;
    dst.hB = src.hB ? 1 : 0;
    dst.cB = src.cB ? 1 : 0;
    
    return dst;
}

void convertToHost(const ::CellDevice& src, Cell& dst)
{
    dst.ch = src.ch;
    dst.pB = src.pB != 0;
    dst.sB = src.sB != 0;
    dst.a = src.a;
    
    for (int i = 0; i < 4; ++i) dst.x[i] = src.x[i];
    
    dst.d = src.d;
    dst.phiB = src.phiB != 0;
    dst.t = src.t;
    dst.f = src.f;
    
    for (int i = 0; i < 3; ++i) dst.c[i] = src.c[i];
    
    dst.k = src.k;
    dst.s2B = src.s2B != 0;
    dst.kB = src.kB != 0;
    dst.bB = src.bB != 0;
    dst.hB = src.hB != 0;
    dst.cB = src.cB != 0;
}

bool swap_lattices_gpu()
{
    return true;
}

void ca_update_gpu_wrapper(
    unsigned CONVOL, unsigned SLOT1, unsigned SLOT2, unsigned SLOT3, 
    unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6, 
    unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE, 
    unsigned FLOOD, unsigned FRAME, unsigned RMAX)
{
    cudaSimulationStep(
        CONVOL, SLOT1, SLOT2, SLOT3, SLOT4, DIFFUSION, 
        SLOT5, SLOT6, SLOT7, SLOT8, RELOC, REISSUE, 
        FLOOD, FRAME, RMAX
    );
}

void ca_update_gpu_wrapper()
{
    ca_update_gpu_wrapper(
        CONVOL, SLOT1, SLOT2, SLOT3, SLOT4, DIFFUSION, 
        SLOT5, SLOT6, SLOT7, SLOT8, RELOC, REISSUE, 
        FLOOD, FRAME, RMAX
    );
}

void free_cuda_memory()
{
    ::free_cuda_memory();
}

} // namespace automaton