// cuda_sim.cu
#include "cuda_sim.h"
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>

// device arrays (one dimension)
static CellDevice* d_curr = nullptr;
static CellDevice* d_draft = nullptr;
static CellDevice* d_mirror = nullptr;

// mapped host voxels pointer
static uint32_t* h_mappedVoxels = nullptr;
static size_t voxels_count = 0;

// grid dims (copied from host)
static unsigned gEL = 0;
static unsigned gW = 0;
static size_t BLOCK = 0;

// error macro
#define CUDA_CHECK(call) do { \
  cudaError_t err = (call); \
  if (err != cudaSuccess) { \
    fprintf(stderr,"CUDA error %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
    return false; } } while(0)

// index helpers
__device__ inline size_t idx4(unsigned x, unsigned y, unsigned z, unsigned w, unsigned EL, unsigned W) {
    return (((size_t)x * EL + y) * EL + z) * (size_t)W + w;
}

// neighbor index with wrap (same convention as CPU)
__device__ inline void neighbor_coords(unsigned &nx, unsigned &ny, unsigned &nz, unsigned &nw, 
                                       unsigned x, unsigned y, unsigned z, unsigned w, int dx, int dy, int dz, int dw,
                                       unsigned EL, unsigned W) {
    int tx = int(x) + dx;
    int ty = int(y) + dy;
    int tz = int(z) + dz;
    int tw = int(w) + dw;
    tx = (tx % int(EL) + int(EL)) % int(EL);
    ty = (ty % int(EL) + int(EL)) % int(EL);
    tz = (tz % int(EL) + int(EL)) % int(EL);
    tw = (tw % int(W)  + int(W))  % int(W);
    nx = (unsigned)tx; ny = (unsigned)ty; nz = (unsigned)tz; nw = (unsigned)tw;
}

// Example: simple update kernel that applies k++ and updates t as in CPU (you must expand with your full step logic)
__global__ void kernel_simulation_step(CellDevice* curr, CellDevice* draft, unsigned EL, unsigned W, unsigned FRAME, size_t totalCells) {
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;
    if (gid >= totalCells) return;
    // decode coordinates
    unsigned cellsPerW = EL * EL * EL;
    unsigned w = gid / cellsPerW;
    unsigned rem = gid % cellsPerW;
    unsigned x = rem / (EL * EL);
    unsigned y = (rem / EL) % EL;
    unsigned z = rem % EL;

    size_t id = idx4(x,y,z,w,EL,W);
    CellDevice my = curr[id];
    CellDevice out = my; // by default copy

    // === HERE: put your per-cell logic ===
    // For example update k:
    out.k = (my.k + 1); // modulo FRAME handled by CPU wrapper or add % FRAME
    // update t only when k wraps:
    if (out.k == 0) {
        // mimic original: if (curr.a == W_USED && curr.t <= RMAX) draft.t++ else draft.t = (curr.t + 1) % RMAX;
        // But we don't have RMAX/FRAME here. For demo:
        out.t = (my.t + 1);
    }
    // ======================================
    draft[id] = out;
}

// kernel to generate voxels based on device lattice (single layer w)
__global__ void kernel_generate_voxels(
    const CellDevice* lattice, uint32_t* voxels, unsigned EL, unsigned W, unsigned layerW)
{
    size_t linearXYZ = blockIdx.x * blockDim.x + threadIdx.x;
    size_t total = (size_t)EL * EL * EL;
    if (linearXYZ >= total) return;
    unsigned x = (unsigned)(linearXYZ / (EL * EL));
    unsigned y = (unsigned)((linearXYZ / EL) % EL);
    unsigned z = (unsigned)(linearXYZ % EL);

    size_t id = idx4(x,y,z, layerW, EL, W);
    CellDevice c = lattice[id];

    // Example coloring logic copied from CPU:
    uint32_t color = 0x000000; // black default
    if (c.t == c.d) {
        if (c.a == W) { // original checks: if (cell.a == W_USED) -> orphan red.
            color = 0x00FF0000; // 0x00BBGGRR (we produce 0x00RRGGBB? Windows COLORREF uses 0x00BBGGRR)
            // We'll produce 0x00BBGGRR directly below
            // Set red: 0x000000FF would be RR but COLORREF is 0x00BBGGRR so red = 0x000000FF
            color = (0x000000FF);
        } else if (c.t == 0) {
            // green
            color = (0x0000FF00);
        } else {
            // white: R=G=B=255 => COLORREF 0x00FFFFFF
            color = 0x00FFFFFF;
        }
    } else {
        color = 0x00000000;
    }
    // store
    voxels[linearXYZ] = color;
}

extern "C" bool initCudaSimulation(unsigned EL, unsigned W_USED) {
    gEL = EL;
    gW = W_USED;
    BLOCK = (size_t)EL * EL * EL * (size_t)W_USED;
    voxels_count = (size_t)EL * EL * EL;

    // allocate device buffers
    size_t bytes = BLOCK * sizeof(CellDevice);
    cudaError_t err;

    err = cudaMalloc(&d_curr, bytes);
    if (err != cudaSuccess) { fprintf(stderr,"cudaMalloc d_curr failed: %s\n", cudaGetErrorString(err)); return false; }
    err = cudaMalloc(&d_draft, bytes);
    if (err != cudaSuccess) { fprintf(stderr,"cudaMalloc d_draft failed: %s\n", cudaGetErrorString(err)); return false; }
    err = cudaMalloc(&d_mirror, bytes);
    if (err != cudaSuccess) { fprintf(stderr,"cudaMalloc d_mirror failed: %s\n", cudaGetErrorString(err)); return false; }

    // allocate mapped host voxels (zero-copy)
    size_t voxBytes = voxels_count * sizeof(uint32_t);
    err = cudaHostAlloc(&h_mappedVoxels, voxBytes, cudaHostAllocMapped);
    if (err != cudaSuccess) { fprintf(stderr,"cudaHostAlloc mapped voxels failed: %s\n", cudaGetErrorString(err)); return false; }
    // Optionally memset
    memset(h_mappedVoxels, 0, voxBytes);

    return true;
}

extern "C" void cudaSimulationStep() {
    size_t total = BLOCK;
    if (total == 0) return;
    // launch kernel: 1 thread per cell
    const int threads = 256;
    const size_t blocks = (total + threads - 1) / threads;
    // you'll want to pass FRAME and other constants; for demo pass 256 as FRAME
    kernel_simulation_step<<<blocks, threads>>>(d_curr, d_draft, gEL, gW, 256u, total);
    cudaDeviceSynchronize();

    // swap pointers on device
    CellDevice* tmp = d_curr; d_curr = d_draft; d_draft = tmp;
}

extern "C" void cudaUpdateVoxelsLayer(unsigned layer) {
    // generate voxels into mapped host memory
    size_t total = (size_t)gEL * gEL * gEL;
    const int threads = 256;
    const size_t blocks = (total + threads - 1) / threads;
    kernel_generate_voxels<<<blocks, threads>>>(d_curr, h_mappedVoxels, gEL, gW, layer);
    cudaDeviceSynchronize();
}

extern "C" uint32_t* getMappedVoxels() { return h_mappedVoxels; }
extern "C" size_t getVoxelsSize() { return voxels_count; }

extern "C" void cudaCleanup() {
    if (d_curr) cudaFree(d_curr);
    if (d_draft) cudaFree(d_draft);
    if (d_mirror) cudaFree(d_mirror);
    if (h_mappedVoxels) cudaFreeHost(h_mappedVoxels);
    d_curr = d_draft = d_mirror = nullptr;
    h_mappedVoxels = nullptr;
}
