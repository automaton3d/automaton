/*
 * automaton_compute_cuda.cu
 */
//#define USE_CUDA // DEBUG

#ifdef USE_CUDA
#include "automaton_compute.h"
#include "automaton_constants.h"
#include <cuda_runtime.h>
#include <iostream>
#include <cmath>
#include <vector>
#include "voxel.h"

// Host copies of grid dimensions set by initGPU/setSimulationParameters
unsigned h_EL = 0;
unsigned h_W_USED = 0;

// Forward declaration for initialization function (defined in automaton_init.cu)
extern "C" void initGPULatticeState_HostImpl(
    automaton::Cell* d_lattice_curr_ptr,
    automaton::Cell* d_lattice_draft_ptr,
    automaton::Cell* d_lattice_mirror_ptr,
    unsigned EL_in,
    unsigned W_USED_in);

#define CUDA_CHECK(call)                                                      \
{                                                                             \
    const cudaError_t error = call;                                           \
    if (error != cudaSuccess)                                                 \
    {                                                                         \
        fprintf(stderr, "CUDA Error: %s at %s:%d\n",                          \
                cudaGetErrorString(error), __FILE__, __LINE__);               \
        exit(EXIT_FAILURE);                                                   \
    }                                                                         \
}

// ===================================================================
// HOST GLOBAL VARIABLES (Pointers to Device Memory and Host Copies)
// ===================================================================

static automaton::Cell* d_lattice_curr = nullptr;
static automaton::Cell* d_lattice_draft = nullptr;
static automaton::Cell* d_lattice_mirror = nullptr;

static unsigned h_BLOCK = 0;   // Host copy of c_BLOCK
static unsigned h_L3 = 0;      // Host copy of c_L3
static unsigned h_UPDATE = 0;  // Host copy of c_UPDATE (RMAX)
static unsigned h_CONVOL = 0;  // Host copy of c_CONVOL

static Voxel* d_voxels = nullptr;
static bool gpu_initialized = false;
static unsigned g_host_k = 0; // Host-side global tick counter

// ===================================================================
// CA CONSTANTS (Passed to __constant__ memory)
// ===================================================================
__constant__ unsigned c_EL;
__constant__ unsigned c_W_USED;
__constant__ unsigned c_L2;       // EL*EL
__constant__ unsigned c_L3;       // EL*EL*EL (Block size for 3D grid)
__constant__ unsigned c_BLOCK;    // L3 * W_USED (Total cells)

__constant__ unsigned c_UPDATE;   // Max tick count (RMAX)
__constant__ unsigned c_CONVOL;
__constant__ unsigned c_SLOT1, c_SLOT2, c_SLOT3, c_SLOT4;
__constant__ unsigned c_SLOT5, c_SLOT6, c_SLOT7, c_SLOT8;
__constant__ unsigned c_DIFFUSION;
__constant__ unsigned c_RELOC;
__constant__ unsigned c_REISSUE;
__constant__ unsigned c_FLOOD;
__constant__ unsigned c_FRAME;
__constant__ unsigned c_RMAX;
__constant__ int c_scenario;
__constant__ unsigned c_DIAG;
__constant__ unsigned c_CONTRACT;

// ===================================================================
// NAMESPACE COMPATIBILITY LAYER
// ===================================================================
namespace automaton
{
    extern unsigned EL;
    extern unsigned W_USED;
    extern unsigned CENTER;
    
    extern bool convol_delay;
    extern bool diffuse_delay;
    extern bool reloc_delay;
}

// ===================================================================
// DEVICE HELPER FUNCTIONS (Indexing, Neighbors, Utils)
// ===================================================================

__device__ inline int wrapCoord(int coord, int dim) {
    if (coord < 0) return coord + dim;
    if (coord >= dim) return coord - dim;
    return coord;
}

__device__ inline size_t getIndex(int x, int y, int z, int w) {
    x = wrapCoord(x, c_EL);
    y = wrapCoord(y, c_EL);
    z = wrapCoord(z, c_EL);
    w = wrapCoord(w, c_W_USED);

    return (size_t)w * c_L3 + (size_t)z * c_L2 + (size_t)y * c_EL + (size_t)x;
}

__device__ inline automaton::Cell& getCell(automaton::Cell* lattice, int x, int y, int z, int w) {
    return lattice[getIndex(x, y, z, w)];
}

__device__ inline automaton::Cell& getNeighbor(
    automaton::Cell* lattice, 
    int curr_x, int curr_y, int curr_z, int curr_w, 
    int direction
) {
    static const int disp[8][4] = {
      { 0, -1,  0,  0}, // NORTH (0)
      {+1,  0,  0,  0}, // EAST (1)
      { 0, +1,  0,  0}, // SOUTH (2)
      {-1,  0,  0,  0}, // WEST (3)
      { 0,  0, +1,  0}, // UP (4)
      { 0,  0, -1,  0}, // DOWN (5)
      { 0,  0,  0, +1}, // FORWARD (6)
      { 0,  0,  0, -1}  // BACKWARD (7)
    };
    
    int dx = disp[direction][0];
    int dy = disp[direction][1];
    int dz = disp[direction][2];
    int dw = disp[direction][3];

    return getCell(lattice, curr_x + dx, curr_y + dy, curr_z + dz, curr_w + dw);
}

__device__ inline bool isColorNeutral(unsigned char c1, unsigned char c2) {
    unsigned res = (c1 ^ c2) & COLOR_MASK; 
    return ((res == 0 || res == COLOR_MASK) && c1 && c2 && c1 != COLOR_MASK && c2 != COLOR_MASK);
}

__device__ inline bool neutralColor(automaton::Cell &a, automaton::Cell &b) {
    return isColorNeutral(a.COLOR(), b.COLOR());
}

__device__ inline bool neutralWeak(automaton::Cell &a, automaton::Cell &b) {
    unsigned res = (a.ch ^ b.ch) & WEAK_MASK;
    return (res == 0 || res == WEAK_MASK);
}

// ===================================================================
// CA RULE IMPLEMENTATIONS (__device__ functions)
// ===================================================================

// Device-only implementations (not in automaton namespace to avoid conflicts)
__device__ bool device_convolute0(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &mirror) {
    return false;
}

__device__ bool ctrl = true;

__device__ bool device_convolute1(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &mirror) 
{
    if (curr.t == curr.d && curr.t == c_RMAX / 2 && curr.x[3] == 0 && ctrl) {
        draft.c[0] = (unsigned)(clock64() % c_EL);
        draft.c[1] = (unsigned)(clock64() % c_EL);
        draft.c[2] = (unsigned)(clock64() % c_EL);
        ctrl = false;
    }
    return false;
}

__device__ bool device_convolute(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &mirror) {
    switch(c_scenario) {
      case 0: return device_convolute0(curr, draft, mirror);
      case 1: return device_convolute1(curr, draft, mirror);
    }
    return false;
}

__device__ void device_diffuse(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &forward, 
                        automaton::Cell &north, automaton::Cell &west, automaton::Cell &down, 
                        automaton::Cell &south, automaton::Cell &east, automaton::Cell &up) 
{
    if (curr.k < c_SLOT1) {
        if ((north.a == c_W_USED && curr.d >= north.d) ||
            (west.a  == c_W_USED && curr.d >= west.d)  ||
            (down.a  == c_W_USED && curr.d >= down.d)  ||
            (south.a == c_W_USED && curr.d >= south.d) ||
            (east.a  == c_W_USED && curr.d >= east.d)  ||
            (up.a    == c_W_USED && curr.d >= up.d)) 
        {
            draft.a = c_W_USED;
        }
    }
    if (curr.k < c_SLOT2) {
        if ((north.a == c_W_USED && curr.d >= north.d) ||
            (west.a  == c_W_USED && curr.d >= west.d)  ||
            (down.a  == c_W_USED && curr.d >= down.d)  ||
            (south.a == c_W_USED && curr.d >= south.d) ||
            (east.a  == c_W_USED && curr.d >= east.d)  ||
            (up.a    == c_W_USED && curr.d >= up.d)) 
        {
            draft.a = c_W_USED;
        }

        if (curr.d == curr.t) {
            if (north.hB) { draft.c[0] = (north.c[0] + 1) % c_EL; curr.sB = !draft.hB; }
            else if (west.hB)  { draft.c[1] = (west.c[1] + 1) % c_EL; curr.sB = !draft.hB; }
            else if (down.hB)  { draft.c[2] = (down.c[2] + 1) % c_EL; curr.sB = !draft.hB; }
            else if (south.hB) { draft.c[0] = (south.c[0] + 1) % c_EL; curr.sB = !draft.hB; }
            else if (east.hB)  { draft.c[1] = (east.c[1] + 1) % c_EL; curr.sB = !draft.hB; }
            else if (up.hB)    { draft.c[2] = (up.c[2] + 1) % c_EL; curr.sB = !draft.hB; }
        }
    }
    else if (curr.k < c_SLOT3) {
        if (!ZERO(north.c)) {
            draft.c[0] = north.c[0];
            draft.c[1] = north.c[1];
            draft.c[2] = north.c[2];
            if (north.kB) draft.kB = north.kB;
        }
        if (!ZERO(west.c)) {
            draft.c[0] = west.c[0];
            draft.c[1] = west.c[1];
            draft.c[2] = west.c[2];
            if (west.kB) draft.kB = west.kB;
        }
        if (!ZERO(down.c)) {
            draft.c[0] = down.c[0];
            draft.c[1] = down.c[1];
            draft.c[2] = down.c[2];
            if (down.kB) draft.kB = down.kB;
        }

        unsigned char mf = down.f;
        if (west.f  > mf) mf = west.f;
        if (north.f > mf) mf = north.f;
        if (south.f > mf) mf = south.f;
        if (east.f  > mf) mf = east.f;
        if (up.f    > mf) mf = up.f;
        draft.f = mf;

        if (!curr.cB) {
            if (north.cB && north.d > curr.d) {
                draft.cB = true;
                if (north.a != c_W_USED) draft.a = north.a;
            } else if (south.cB && south.d > curr.d) {
                draft.cB = true;
                if (south.a != c_W_USED) draft.a = south.a;
            } else if (east.cB && east.d > curr.d) {
                draft.cB = true;
                if (east.a != c_W_USED) draft.a = east.a;
            } else if (west.cB && west.d > curr.d) {
                draft.cB = true;
                if (west.a != c_W_USED) draft.a = west.a;
            } else if (down.cB && down.d > curr.d) {
                draft.cB = true;
                if (down.a != c_W_USED) draft.a = down.a;
            } else if (up.cB && up.d > curr.d) {
                draft.cB = true;
                if (up.a != c_W_USED) draft.a = up.a;
            }
        }
    }
    else if (curr.k < c_SLOT4) {
        if (forward.kB && forward.a == curr.a) {
            int delta_x = (int(curr.x[0]) - int(forward.x[0]) + int(c_EL)) % int(c_EL);
            int delta_y = (int(curr.x[1]) - int(forward.x[1]) + int(c_EL)) % int(c_EL);
            int delta_z = (int(curr.x[2]) - int(forward.x[2]) + int(c_EL)) % int(c_EL);

            draft.c[0] = (forward.c[0] + unsigned(delta_x)) % c_EL;
            draft.c[1] = (forward.c[1] + unsigned(delta_y)) % c_EL;
            draft.c[2] = (forward.c[2] + unsigned(delta_z)) % c_EL;
            draft.kB = forward.kB;
            draft.cB = forward.cB;
        }

        draft.f = (forward.f > curr.f) ? forward.f : curr.f;
    }
    else if (curr.k < c_SLOT5) {
        if (curr.a == c_W_USED) {
            if (curr.d < curr.t) {
                draft.a = curr.x[3];
            }
        }
    }
}

__device__ void device_relocate(automaton::Cell& curr, automaton::Cell &draft, 
                         automaton::Cell &north, automaton::Cell &west, automaton::Cell &down) 
{
    unsigned x = curr.x[0];
    unsigned y = curr.x[1];
    unsigned z = curr.x[2];

    if (curr.k < c_SLOT6) {
        if (north.c[0] > 0) {
            draft = north;
            draft.c[0]--;
        }
    }
    else if (curr.k < c_SLOT7) {
        if (west.c[1] > 0) {
            draft = west;
            draft.c[1]--;
        }
    }
    else if (curr.k < c_SLOT8) {
        if (down.c[2] > 0) {
            draft = down;
            draft.c[2]--;
        }
    }

    draft.x[0] = x;
    draft.x[1] = y;
    draft.x[2] = z;
}

__device__ void device_reissue(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &forward, 
                        automaton::Cell &north, automaton::Cell &west, automaton::Cell &down, 
                        automaton::Cell &south, automaton::Cell &east, automaton::Cell &up) 
{
    draft.kB = false;
    draft.hB = false;
    draft.bB = false;

    if (curr.t == curr.d) {
        if (north.d == curr.d + 1) draft.a = north.a;
        if (south.d == curr.d + 1) draft.a = south.a;
        if (east.d == curr.d + 1) draft.a = east.a;
        if (west.d == curr.d + 1) draft.a = west.a;
        if (up.d == curr.d + 1) draft.a = up.a;
        if (down.d == curr.d + 1) draft.a = down.a;
    }

    if (curr.cB) {
        draft.cB = false;
        if (curr.a != c_W_USED && curr.d < 2) {
            draft.t = 0;
        }
    }
}

__device__ void device_flood(automaton::Cell& curr, automaton::Cell &draft, automaton::Cell &forward, 
                      automaton::Cell &north, automaton::Cell &west, automaton::Cell &down, 
                      automaton::Cell &south, automaton::Cell &east, automaton::Cell &up) 
{
    if (curr.a != c_W_USED) {
        unsigned char min_t = north.t;
        if (south.t < min_t)  min_t = south.t;
        if (east.t  < min_t)  min_t = east.t;
        if (west.t  < min_t)  min_t = west.t;
        if (down.t  < min_t)  min_t = down.t;
        if (up.t    < min_t)  min_t = up.t;

        draft.t = min_t;
    }
}

// ===================================================================
// CUDA KERNELS
// ===================================================================

__global__ void updateLatticeKernel(
    automaton::Cell* d_lattice_curr, 
    automaton::Cell* d_lattice_draft, 
    automaton::Cell* d_lattice_mirror
) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= c_BLOCK) return;

    automaton::Cell &curr = d_lattice_curr[index];
    automaton::Cell &draft = d_lattice_draft[index];
    automaton::Cell &mirror = d_lattice_mirror[index];

    draft = curr; 
    unsigned current_k = curr.k; 

    int x, y, z, w;
    w = index / c_L3;
    int remainder = index % c_L3;
    z = remainder / c_L2;
    remainder = remainder % c_L2;
    y = remainder / c_EL;
    x = remainder % c_EL;

    automaton::Cell &forward = getNeighbor(d_lattice_curr, x, y, z, w, FORWARD);
    automaton::Cell &north   = getNeighbor(d_lattice_curr, x, y, z, w, NORTH);
    automaton::Cell &west    = getNeighbor(d_lattice_curr, x, y, z, w, WEST);
    automaton::Cell &down    = getNeighbor(d_lattice_curr, x, y, z, w, DOWN);
    automaton::Cell &south   = getNeighbor(d_lattice_curr, x, y, z, w, SOUTH);
    automaton::Cell &east    = getNeighbor(d_lattice_curr, x, y, z, w, EAST);
    automaton::Cell &up      = getNeighbor(d_lattice_curr, x, y, z, w, UP);

    if (current_k < c_CONVOL) {
        device_convolute(curr, draft, mirror);
    }
    else if (curr.k < c_DIFFUSION)
    {
        device_diffuse(curr, draft, forward, north, west, down, south, east, up);
    }
    else if (curr.k < c_RELOC)
    {
        device_relocate(curr, draft, north, west, down);
    }
    else if (curr.k < c_REISSUE)
    {
        device_reissue(curr, draft, forward, north, west, down, south, east, up);
    }
    else if (curr.k < c_FLOOD)
    {
        device_flood(curr, draft, forward, north, west, down, south, east, up);
    }
    
    draft.k = (current_k + 1) % c_UPDATE; 
}

__global__ void shiftMirrorKernel(automaton::Cell* d_lattice_curr, automaton::Cell* d_lattice_mirror) 
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= c_BLOCK) return;
    
    d_lattice_mirror[index] = d_lattice_curr[index];
    d_lattice_mirror[index].f = d_lattice_mirror[index].t;
}

// ===================================================================
// HOST FUNCTIONS
// ===================================================================

void initGPU(unsigned EL, unsigned W_USED) {
    h_EL = EL;
    h_W_USED = W_USED;
    
    // Calculate derived values
    unsigned long long L3_val = (unsigned long long)EL * EL * EL;
    unsigned long long BLOCK_val = L3_val * W_USED;
    
    h_L3 = (unsigned)L3_val;
    h_BLOCK = (unsigned)BLOCK_val;
    
    size_t lattice_size = (size_t)BLOCK_val * sizeof(automaton::Cell);
    
    printf("InitGPU: Allocating %zu bytes for %llu cells\n", lattice_size, BLOCK_val);
    
    // Allocate device memory
    CUDA_CHECK(cudaMalloc((void**)&d_lattice_curr, lattice_size));
    CUDA_CHECK(cudaMalloc((void**)&d_lattice_draft, lattice_size));
    CUDA_CHECK(cudaMalloc((void**)&d_lattice_mirror, lattice_size));

    printf("DEBUG POS-MALLOC: d_lattice_curr = %p\n", (void*)d_lattice_curr);

    if (d_lattice_curr == nullptr) {
        fprintf(stderr, "ERRO: Ponteiro nulo apos cudaMalloc!\n");
        return;
    }

    // Copy grid dimensions to constant memory
    unsigned L2_val = EL * EL;
    CUDA_CHECK(cudaMemcpyToSymbol(c_EL, &EL, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_W_USED, &W_USED, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_L2, &L2_val, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_L3, &h_L3, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_BLOCK, &h_BLOCK, sizeof(unsigned)));

    gpu_initialized = true;
    
    std::cout << "GPU initialization completed successfully.\n";
}

void setSimulationParameters(unsigned EL, unsigned W_USED, int scenario)
{
    unsigned CONVOL;
    unsigned SLOT1, SLOT2, SLOT3, SLOT4;
    unsigned SLOT5, SLOT6, SLOT7, SLOT8;
    unsigned DIFFUSION, RELOC; 
    unsigned REISSUE, FLOOD, FRAME, RMAX;
    
    unsigned CENTER = ((EL - 1) / 2);
    unsigned DIAG   = (unsigned) (EL * sqrt(3.0));
    RMAX      = DIAG / 2;
    int CONTRACT  = static_cast<int>(floor(sqrt(3.0) * CENTER));
    
    CONVOL    = W_USED;
    SLOT1     = CONVOL + RMAX;
    SLOT2     = SLOT1 + 3 * (EL - 1);
    SLOT3     = SLOT2 + 3 * (EL - 1);
    SLOT4     = SLOT3 + 2 * W_USED;
    SLOT5     = SLOT4 + 3 * (EL - 1);
    DIFFUSION = SLOT5;
    SLOT6     = DIFFUSION + (EL - 1);
    SLOT7     = SLOT6 + (EL - 1);
    SLOT8     = SLOT7 + (EL - 1);
    RELOC     = SLOT8;
    REISSUE   = RELOC + 1;
    FLOOD     = REISSUE + 3 * (EL - 1);
    FRAME     = FLOOD;

    unsigned update = RMAX;

    h_UPDATE = RMAX;
    h_CONVOL = CONVOL;
 
    CUDA_CHECK(cudaMemcpyToSymbol(c_CONVOL, &CONVOL, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT1, &SLOT1, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT2, &SLOT2, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT3, &SLOT3, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT4, &SLOT4, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT5, &SLOT5, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT6, &SLOT6, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT7, &SLOT7, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_SLOT8, &SLOT8, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_DIFFUSION, &DIFFUSION, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_RELOC, &RELOC, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_REISSUE, &REISSUE, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_FLOOD, &FLOOD, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_FRAME, &FRAME, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_RMAX, &RMAX, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_scenario, &scenario, sizeof(int)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_UPDATE, &update, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_DIAG, &DIAG, sizeof(unsigned)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_CONTRACT, &CONTRACT, sizeof(unsigned)));
    
    g_host_k = 0;
    
    std::cout << "Simulation parameters set successfully (scenario=" << scenario << ").\n";
}

void initGPULatticeState() {
    if (!gpu_initialized) {
        std::cerr << "ERROR: GPU not initialized before initGPULatticeState!\n";
        return;
    }
    
    std::cout << "Calling unified initialization path...\n";
    
    // Call the unified initialization that reuses initSim.cpp functions
    initGPULatticeState_HostImpl(d_lattice_curr, d_lattice_draft, d_lattice_mirror, h_EL, h_W_USED);
    
    cudaDeviceSynchronize();
    
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "CUDA error in initGPULatticeState: " << cudaGetErrorString(err) << "\n";
    } else {
        std::cout << "GPU lattice state initialized successfully.\n";
    }
}

void runSimulationSteps(int numSteps) {
    if (!gpu_initialized) return;
/*
    dim3 block(256);
    dim3 grid((h_BLOCK + block.x - 1) / block.x); 
    
    for (int i = 0; i < numSteps; i++) {
        updateLatticeKernel<<<grid, block>>>(
            d_lattice_curr, d_lattice_draft, d_lattice_mirror
        );
        
        if (g_host_k < h_CONVOL) {
             shiftMirrorKernel<<<grid, block>>>(d_lattice_curr, d_lattice_mirror);
        }
        
        automaton::Cell* temp = d_lattice_curr;
        d_lattice_curr = d_lattice_draft;
        d_lattice_draft = temp;
        
        g_host_k = (g_host_k + 1) % h_UPDATE;
    }
    
    cudaDeviceSynchronize();
    */
}

namespace automaton
{
void getLayerData(unsigned selectedW, std::vector<Cell>& host_layer_data) {
    if (!gpu_initialized || !d_lattice_curr) return;
    
    size_t layer_size_cells = h_L3; 
    size_t layer_size_bytes = layer_size_cells * sizeof(Cell);
    host_layer_data.resize(layer_size_cells);

    size_t offset_bytes = (size_t)selectedW * h_L3 * sizeof(Cell);

    CUDA_CHECK(cudaMemcpy(
        host_layer_data.data(),
        d_lattice_curr + (offset_bytes / sizeof(Cell)),
        layer_size_bytes,
        cudaMemcpyDeviceToHost));
    
    cudaDeviceSynchronize(); 
}
}

void cleanupGPU() {
    if (gpu_initialized) {
        cudaFree(d_lattice_curr);
        cudaFree(d_lattice_draft);
        cudaFree(d_lattice_mirror);
        cudaFree(d_voxels);
        
        d_lattice_curr = nullptr;
        d_lattice_draft = nullptr;
        d_lattice_mirror = nullptr;
        d_voxels = nullptr;
        
        gpu_initialized = false;
        
        std::cout << "GPU resources cleaned up successfully.\n";
    }
}

#endif // USE_CUDA