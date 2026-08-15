// cuda_automaton.cu - CUDA implementation with FULL CA logic
// Unified version: all constants and host functions are defined here.

#pragma nv_diag_suppress 177

#include "model/simulation.h"
#include "cuda_sim_optimized.h"
#include <cuda_runtime.h>
#include "cuda_constants.h" 
#include <iostream>
#include <algorithm>
#include "config.h"

// ===================================================================
// Constant memory — defined HERE so they are in the same compilation
// unit as the kernels (required without -dc separate compilation).
// ===================================================================
__constant__ unsigned dev_EL;
__constant__ unsigned dev_W_USED;
__constant__ unsigned dev_RMAX;
__constant__ unsigned dev_CENTER;
__constant__ unsigned dev_lcenters[32][3]; // per-W source centers (host copies each frame)
__device__   int      dev_ctrl;

// Global device pointers (defined here, used by bridge)
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

// Per-source center lookup (read from constant-memory copy of host lcenters).
static __device__ inline void dev_source_center(unsigned w, int& cx, int& cy, int& cz)
{
    cx = (int)dev_lcenters[w][0];
    cy = (int)dev_lcenters[w][1];
    cz = (int)dev_lcenters[w][2];
}

static __device__ inline int dev_shortest_delta(int a, int b, int mod)
{
    int d = b - a;
    int half = mod / 2;
    if (d > half) d -= mod;
    else if (d < -half) d += mod;
    return d;
}

static __device__ inline int dev_sign(int v)
{
    return (v > 0) - (v < 0);
}

static __device__ inline unsigned dev_wrap(int v, int mod)
{
    int r = v % mod;
    if (r < 0) r += mod;
    return (unsigned)r;
}

// Spherical antipodal wrap for spatial coordinates — matches CPU's get_sphere_cell()
static __device__ void dev_spherical_wrap(int& x, int& y, int& z)
{
    // Toroidal wrap into [0, dev_EL)
    if (x < 0) x += (int)dev_EL;
    if (x >= (int)dev_EL) x -= (int)dev_EL;
    if (y < 0) y += (int)dev_EL;
    if (y >= (int)dev_EL) y -= (int)dev_EL;
    if (z < 0) z += (int)dev_EL;
    if (z >= (int)dev_EL) z -= (int)dev_EL;

    int dx = x - (int)dev_CENTER;
    int dy = y - (int)dev_CENTER;
    int dz = z - (int)dev_CENTER;
    int r2 = dx*dx + dy*dy + dz*dz;
    int rmax2 = (int)dev_RMAX * (int)dev_RMAX;

    if (r2 > rmax2) {
        // Antipodal mapping through the centre
        x = 2 * (int)dev_CENTER - x;
        y = 2 * (int)dev_CENTER - y;
        z = 2 * (int)dev_CENTER - z;

        // Wrap again in case the antipode lands outside the array
        if (x < 0) x += (int)dev_EL;
        if (x >= (int)dev_EL) x -= (int)dev_EL;
        if (y < 0) y += (int)dev_EL;
        if (y >= (int)dev_EL) y -= (int)dev_EL;
        if (z < 0) z += (int)dev_EL;
        if (z >= (int)dev_EL) z -= (int)dev_EL;
    }
}

// Neighbor with spherical antipodal wrapping for spatial + periodic for W
static __device__ ::CellDevice d_getNeighbor(::CellDevice* d_curr_lattice,
                                           unsigned x_curr, 
                                           unsigned y_curr, 
                                           unsigned z_curr, 
                                           unsigned w_curr, 
                                           int i) 
{
    static const int disp[8][4] = {
        {+1, 0, 0, 0}, {-1, 0, 0, 0},
        { 0,+1, 0, 0}, { 0,-1, 0, 0},
        { 0, 0,+1, 0}, { 0, 0,-1, 0},
        { 0, 0, 0,+1}, { 0, 0, 0,-1}
    };

    int nx = (int)x_curr + disp[i][0];
    int ny = (int)y_curr + disp[i][1];
    int nz = (int)z_curr + disp[i][2];
    int nw = (int)w_curr + disp[i][3];

    // Antipodal spherical wrapping for spatial coordinates
    dev_spherical_wrap(nx, ny, nz);

    // Additional integer bounds guarantee
    if (nx < 0) nx = 0;
    if (nx >= (int)dev_EL) nx = (int)dev_EL - 1;
    if (ny < 0) ny = 0;
    if (ny >= (int)dev_EL) ny = (int)dev_EL - 1;
    if (nz < 0) nz = 0;
    if (nz >= (int)dev_EL) nz = (int)dev_EL - 1;

    // Periodic wrapping for W dimension
    nw = nw % (int)dev_W_USED;
    if (nw < 0) nw += (int)dev_W_USED;

    return d_getCell(d_curr_lattice, nx, ny, nz, nw);
}

#define ZERO_C(c) (!(c[0] | c[1] | c[2]))

// Charge-bit access for CellDevice (mirrors CPU)
#define DEV_W1(cell)  ((cell).ch & 0x20)
#define DEV_W0(cell)  ((cell).ch & 0x10)
#define DEV_Q(cell)   ((cell).ch & 0x08)
#define DEV_C2(cell)  ((cell).ch & 0x04)
#define DEV_C1(cell)  ((cell).ch & 0x02)
#define DEV_C0(cell)  ((cell).ch & 0x01)
#define DEV_COLOR(cell)     ((cell).ch & COLOR_MASK)
#define DEV_ANTICOLOR(cell) ((~(cell).ch) & COLOR_MASK)

// Color neutrality test (mirrors CPU neutralColor)
static __device__ inline bool dev_neutralColor(const ::CellDevice& a, const ::CellDevice& b)
{
    int color_a = a.ch & 0x07;
    int color_b = b.ch & 0x07;
    return (color_a ^ color_b) == 0x07;
}

// Weak neutrality test (mirrors CPU neutralWeak)
static __device__ inline bool dev_neutralWeak(const ::CellDevice& a, const ::CellDevice& b)
{
    int weak_a = (a.ch >> 3) & 0x03;
    int weak_b = (b.ch >> 3) & 0x03;
    return (weak_a ^ weak_b) == 0x03;
}

// Simple hash PRNG (for convolute1 random c[] values)
__device__ inline unsigned dev_hash_random(unsigned seed, unsigned mod)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    seed *= 2654435761u;
    return seed % mod;
}

// Device helper: effective wavefront radius (triangle wave)
static __device__ inline unsigned dev_effective_t(unsigned t) {
    unsigned period = 2 * dev_RMAX;
    unsigned raw = t % period;
    return (raw <= dev_RMAX) ? raw : (2 * dev_RMAX - raw);
}

// Device helper: pulsating sphere threshold (triangle wave on r²)
// Matches sine2/pulsating.h: max_r2 = 0.92 * R_MAX^2, step = (L+15)/30.
static __device__ inline unsigned dev_pulse_from_time(unsigned t) {
    const unsigned max_r2 = (dev_RMAX * dev_RMAX * 92u) / 100u;
    const unsigned step = (dev_EL + 15u) / 30u;
    if (step == 0) return 0;
    unsigned span = max_r2;
    if (span == 0) return 0;
    unsigned period = 2 * span;
    unsigned phase = (t * step) % period;
    if (phase < span)
        return phase;
    else
        return max_r2 - (phase - span);
}

// Device integer square root (table-free)
static __device__ inline int dev_isqrt(int n)
{
    if (n <= 0) return 0;
    int result = 0;
    int bit = 1 << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

// Compute the next (u,v) for one cell using the 3-D integer wave equation.
static __device__ inline void dev_phase_step_cell(
    ::CellDevice& c,
    unsigned w,
    ::CellDevice* src,
    unsigned x, unsigned y, unsigned z,
    unsigned pulse_tick)
{
    if (dev_RMAX == 0)
    {
        c.u = 0; c.v = 0; c.active = 0;
        c.phiB = 0; c.pB = 0; c.sB = 0; c.s2B = 0;
        return;
    }

    int cx, cy, cz;
    dev_source_center(w, cx, cy, cz);
    int dx = (int)c.x[0] - cx;
    int dy = (int)c.x[1] - cy;
    int dz = (int)c.x[2] - cz;
    int r2_int = dx*dx + dy*dy + dz*dz;
    if (r2_int < 0) r2_int = 0;
    c.r2 = (uint32_t)r2_int;
    c.r  = dev_isqrt(r2_int);

    // Wave parameters (same scaling as the former SincWave test).
    int R = (dev_RMAX > 0u) ? (int)dev_RMAX : 1;
    int shellR = (int)((dev_RMAX * 24u) / 100u);
    int shellW = (int)(dev_RMAX / 5u);
    if (shellW < 1) shellW = 1;
    int absorbW = (R / 27 > 2) ? (R / 27) : 2;

    int diffDivShift = 2;
    if (R >= 384)       diffDivShift = 6;
    else if (R >= 192)  diffDivShift = 5;
    else if (R >= 96)   diffDivShift = 4;
    else if (R >= 40)   diffDivShift = 3;

    int velDampShift = diffDivShift + 3;
    const int DIFF_SHIFT = 4;
    const int SHELL_TARGET = 16384;

    unsigned int pulseR2 = dev_pulse_from_time(pulse_tick);
    unsigned int pulseTol = (dev_EL * dev_EL + 150u) / 300u;
    if (pulseTol == 0u) pulseTol = 1u;
    unsigned int r2diff = (c.r2 > pulseR2) ? (c.r2 - pulseR2) : (pulseR2 - c.r2);
    bool active = (c.r2 != 0xFFFFFFFFu && r2diff <= pulseTol);

    // Hard zero on spatial boundaries and outside the processed sphere,
    // except for the source-center cell (r2 == 0) which may sit on a face.
    if (c.r2 != 0 && (x == 0 || x == dev_EL - 1 ||
        y == 0 || y == dev_EL - 1 ||
        z == 0 || z == dev_EL - 1 ||
        c.r < 0 || c.r >= R))
    {
        c.u = 0; c.v = 0;
        c.active = active ? 1u : 0u;
        c.phiB   = c.active;
        c.pB = 0; c.sB = 0; c.s2B = 0;
        return;
    }

    int u = c.u;
    int v = c.v;
    int r = c.r;

    int neighbors_u = 0;
    if (x + 1 < dev_EL) neighbors_u += d_getCell(src, (int)x + 1, (int)y, (int)z, (int)w).u;
    if (x > 0)          neighbors_u += d_getCell(src, (int)x - 1, (int)y, (int)z, (int)w).u;
    if (y + 1 < dev_EL) neighbors_u += d_getCell(src, (int)x, (int)y + 1, (int)z, (int)w).u;
    if (y > 0)          neighbors_u += d_getCell(src, (int)x, (int)y - 1, (int)z, (int)w).u;
    if (z + 1 < dev_EL) neighbors_u += d_getCell(src, (int)x, (int)y, (int)z + 1, (int)w).u;
    if (z > 0)          neighbors_u += d_getCell(src, (int)x, (int)y, (int)z - 1, (int)w).u;

    int lap = neighbors_u - 6 * u;

    int diffShift = DIFF_SHIFT + 1 - (r >> diffDivShift);
    if (diffShift < DIFF_SHIFT - 1)
        diffShift = DIFF_SHIFT - 1;

    int v_new = v + (lap >> diffShift);
    int u_new = u + v_new;
    v_new -= (v_new >> velDampShift);

    // Spherical-shell source forcing.
    int dr = r - shellR;
    if (dr < 0) dr = -dr;
    if (dr <= shellW)
    {
        if (u > SHELL_TARGET)
            v_new -= (u - SHELL_TARGET) >> 4;
        else if ((pulse_tick & 3u) == 0u)
            v_new += ((SHELL_TARGET - u) >> 10) + 1;
    }

    // Absorbing outer boundary.
    if (R > absorbW && r > R - absorbW)
    {
        int dist = r - (R - absorbW);
        if (dist >= absorbW)
            u_new = 0;
        else if (dist > 0)
            u_new /= (1 << dist);
    }

    // Global damping.
    u_new -= (u_new >> 12);
    v_new -= (v_new >> 12);

    // Per-w angular offset so different W copies see distinct pB/sB patterns
    // while the underlying (u,v) wave field stays the same for all layers.
    int helixR = (int)dev_RMAX;
    unsigned int phase_full = 2u * (unsigned int)helixR * (unsigned int)helixR;
    unsigned int w_offset = (unsigned int)(((unsigned long long)w * (unsigned long long)phase_full) / (unsigned long long)dev_W_USED);
    unsigned int cell_phase = w_offset % phase_full;
    int m = (int)(cell_phase / (unsigned int)helixR);
    int cos_w, sin_w;
    if (m < helixR)
    {
        int arg = m * (helixR - m);
        int s = dev_isqrt(arg);
        cos_w = helixR - 2 * m;
        sin_w = 2 * s;
    }
    else
    {
        int m2 = m - helixR;
        int arg = m2 * (helixR - m2);
        int s = dev_isqrt(arg);
        cos_w = 2 * m - 3 * helixR;
        sin_w = -2 * s;
    }

    int ru = (u_new * cos_w - v_new * sin_w) / helixR;
    int rv = (u_new * sin_w + v_new * cos_w) / helixR;

    c.u      = u_new;
    c.v      = v_new;
    c.active = active ? 1u : 0u;
    c.phiB   = c.active;
    c.pB     = (ru > 0) ? 1 : 0;
    c.sB     = (rv > 0) ? 1 : 0;

    // Sieve trigger: probability proportional to positive wave amplitude.
    bool s2B_trigger = false;
    if (u_new > 0)
    {
        int64_t prod = (int64_t)u_new * (int64_t)(pulse_tick + 1);
        int64_t mod = prod % (int64_t)SHELL_TARGET;
        if (mod < (int64_t)u_new) s2B_trigger = true;
    }
    c.s2B    = (active && s2B_trigger) ? 1u : 0u;
}

// ===================================================================
// PHASE STEP KERNEL — run before ca_update_kernel each tick
// ===================================================================
__global__ void phase_step_kernel(::CellDevice* src, ::CellDevice* dst, unsigned pulse_tick)
{
    unsigned tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (dev_EL == 0 || dev_W_USED == 0) return;

    unsigned total = dev_EL * dev_EL * dev_EL * dev_W_USED;
    if (tid >= total) return;

    unsigned w = tid % dev_W_USED;
    unsigned idx3d = tid / dev_W_USED;
    unsigned z = idx3d % dev_EL;
    unsigned y = (idx3d / dev_EL) % dev_EL;
    unsigned x = idx3d / (dev_EL * dev_EL);

    ::CellDevice c = d_getCell(src, (int)x, (int)y, (int)z, (int)w);
    dev_phase_step_cell(c, w, src, x, y, z, pulse_tick);
    d_getCell(dst, (int)x, (int)y, (int)z, (int)w) = c;
}

// ===================================================================
// DEVICE CONVOLUTE FUNCTIONS (mirror convolutes.cpp)
// ===================================================================

__device__ inline void dev_convolute0(::CellDevice& /*curr*/, ::CellDevice& /*draft*/,
                                      ::CellDevice& /*mirror*/, unsigned /*w*/, unsigned /*tid*/)
{
    // Scenario 0: no interaction
}

__device__ inline void dev_convolute1(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& /*mirror*/, unsigned w, unsigned tid)
{
    if (curr.active && dev_effective_t(curr.t) == dev_RMAX / 2 && w == 0)
    {
        int old = atomicExch(&dev_ctrl, 0);
        if (old == 1)
        {
            draft.c[0] = dev_hash_random(tid * 3 + 1, dev_EL);
            draft.c[1] = dev_hash_random(tid * 3 + 2, dev_EL);
            draft.c[2] = dev_hash_random(tid * 3 + 3, dev_EL);
        }
    }
}

__device__ inline void dev_convolute2(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& /*mirror*/, unsigned w, unsigned /*tid*/)
{
    if (curr.active && dev_effective_t(curr.t) == dev_RMAX / 2 && w == 0)
    {
        int old = atomicExch(&dev_ctrl, 0);
        if (old == 1) draft.a = dev_W_USED;
    }
}

__device__ inline void dev_convolute3(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& /*mirror*/, unsigned w, unsigned /*tid*/)
{
    if (curr.active && dev_effective_t(curr.t) == dev_RMAX / 2 && w == 0)
    {
        int old = atomicExch(&dev_ctrl, 0);
        if (old == 1)
        {
            draft.a = dev_W_USED;
            draft.leader_w = DEV_NO_LEADER_W;
            draft.cB = 1;
        }
    }
}

__device__ inline void dev_convolute4(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& /*mirror*/, unsigned w, unsigned /*tid*/)
{
    if (curr.active && dev_effective_t(curr.t) == dev_RMAX / 2 && curr.sB && w == 0)
    {
        int old = atomicExch(&dev_ctrl, 0);
        if (old == 1) draft.hB = 1;
    }
}

__device__ inline void dev_convolute5(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& /*mirror*/, unsigned w, unsigned /*tid*/)
{
    if (curr.active && dev_effective_t(curr.t) == dev_RMAX / 2 && curr.pB && w == 0 &&
        !curr.cB && curr.a != dev_W_USED)
    {
        int old = atomicExch(&dev_ctrl, 0);
        if (old == 1)
        {
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.cB = 1;
            draft.a = dev_W_USED;
            draft.leader_w = DEV_NO_LEADER_W;
        }
    }
}

__device__ inline void dev_convolute6(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& mirror, unsigned /*w*/, unsigned /*tid*/)
{
    // Cells awaken?
    if (curr.active && mirror.active)
    {
        // Test superposition
        if (curr.x[0] == mirror.x[0] &&
            curr.x[1] == mirror.x[1] &&
            curr.x[2] == mirror.x[2])
        {
            // Test dispersion
            if (curr.a != dev_W_USED &&
                DEV_W1(curr) != DEV_W1(mirror) &&
                !curr.cB &&
                dev_effective_t(curr.t) == dev_RMAX / 2)
            {
                if (curr.pB && mirror.sB)
                {
                    draft.c[0] = curr.x[0];
                    draft.c[1] = curr.x[1];
                    draft.c[2] = curr.x[2];
                    draft.cB = 1;
                    draft.a = dev_W_USED;
                    draft.leader_w = DEV_NO_LEADER_W;
                }
                else if (curr.sB && !mirror.pB)
                {
                    draft.hB = 1;
                    draft.cB = 1;
                    draft.a = dev_W_USED;
                    draft.leader_w = DEV_NO_LEADER_W;
                }
            }
        }
    }
}

__device__ inline void dev_convolute7_legacy(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& mirror, unsigned /*w*/, unsigned /*tid*/,
                                      ::CellDevice* /*d_curr*/, ::CellDevice* /*d_draft*/)
{
    // Cells awaken?
    if (curr.active && mirror.active)
    {
        // --- A) SAME POSITION (superposition) ---
        if (curr.x[0] == mirror.x[0] &&
            curr.x[1] == mirror.x[1] &&
            curr.x[2] == mirror.x[2])
        {
            // Test dispersion
            if (DEV_W1(curr) != DEV_W1(mirror) &&
                dev_effective_t(curr.t) == dev_RMAX / 2 &&
                !curr.cB && curr.a != dev_W_USED)
            {
                // Who has the pB true interacts once
                if (curr.pB && !mirror.pB)
                {
                    draft.c[0] = curr.x[0];
                    draft.c[1] = curr.x[1];
                    draft.c[2] = curr.x[2];
                    draft.cB = 1;
                }
                // Who has pB false interacts with the last pB true
                if (!curr.pB && mirror.pB)
                {
                    draft.hB = 1;
                    draft.cB = 1;
                }
            }
            // Test single pair
            else if (curr.f == curr.t && mirror.f == mirror.t)
            {
                // Different sectors?
                if (DEV_W1(curr) != DEV_W1(mirror))
                {
                    // Momentum (Graviton)
                    if (curr.pB && mirror.pB)
                    {
                        draft.f += curr.t;
                        draft.s2B &= curr.phiB;
                        draft.a = min(curr.a, mirror.a);
                    }
                }
                else if ((DEV_Q(curr)  ^ DEV_Q(mirror))  &&
                         (DEV_W1(curr) == DEV_W1(mirror)) &&
                         (DEV_W0(curr) ^ DEV_W0(mirror))  &&
                         (DEV_C2(curr) == DEV_C2(mirror)) &&
                         (DEV_C1(curr) == DEV_C1(mirror)) &&
                         (DEV_C0(curr) == DEV_C0(mirror)))
                {
                    // Photon
                    draft.f += curr.t;
                    draft.s2B &= curr.phiB;
                    draft.a = min(curr.a, mirror.a);
                    draft.bB = 1;
                }
                else if ((curr.ch == 0 && mirror.ch == 0) ||
                         (curr.ch == 63 && mirror.ch == 63))
                {
                    // Neutrino
                    draft.f += curr.t;
                    draft.s2B &= curr.phiB;
                    draft.a = min(curr.a, mirror.a);
                }
                else if ((!DEV_Q(curr) && !DEV_Q(mirror)) &&
                         (!DEV_W1(curr) && !DEV_W1(mirror)) &&
                         (DEV_W0(curr) && DEV_W0(mirror)) &&
                         (DEV_COLOR(curr) == DEV_COLOR(mirror)) &&
                         (DEV_COLOR(curr) != 0 && DEV_COLOR(curr) != 7))
                {
                    // Boson W-
                    draft.f += curr.t;
                    draft.s2B &= curr.phiB;
                    draft.a = min(curr.a, mirror.a);
                    draft.bB = 1;
                }
                else if ((DEV_Q(curr) && DEV_Q(mirror)) &&
                         (DEV_W1(curr) && DEV_W1(mirror)) &&
                         (!DEV_W0(curr) && !DEV_W0(mirror)) &&
                         (DEV_COLOR(curr) == DEV_COLOR(mirror)) &&
                         (DEV_COLOR(curr) != 0 && DEV_COLOR(curr) != 7))
                {
                    // Boson W+
                    draft.f += curr.t;
                    draft.s2B &= curr.phiB;
                    draft.a = min(curr.a, mirror.a);
                    draft.bB = 1;
                }
                else if ((DEV_Q(curr) != DEV_Q(mirror)) &&
                         (DEV_W1(curr) && DEV_W1(mirror)) &&
                         (!DEV_W0(curr) && !DEV_W0(mirror)) &&
                         (DEV_COLOR(curr) == DEV_COLOR(mirror)) &&
                         (DEV_COLOR(curr) != 0 && DEV_COLOR(curr) != 7))
                {
                    // Boson Z
                    draft.f += curr.t;
                    draft.s2B &= curr.phiB;
                    draft.a = min(curr.a, mirror.a);
                    draft.bB = 1;
                }
            }
            // Blob formation
            else if (curr.f != curr.t && mirror.f != mirror.t && curr.bB)
            {
                draft.f += curr.f + mirror.f;
                draft.s2B &= curr.phiB;
                draft.a = min(curr.a, mirror.a);
            }
        }
        // --- B) DIFFERENT POSITION (distinct bubbles) ---
        else
        {
            // Same sector
            if (DEV_W1(curr) == DEV_W1(mirror))
            {
                // Annihilation?
                if (!curr.kB && !mirror.kB &&
                    DEV_Q(curr) != DEV_Q(mirror) &&
                    DEV_W0(curr) != DEV_W0(mirror) &&
                    DEV_COLOR(curr) == DEV_ANTICOLOR(mirror) &&
                    curr.f == curr.t && mirror.f == mirror.t)
                {
                    draft.c[0] = curr.x[0];
                    draft.c[1] = curr.x[1];
                    draft.c[2] = curr.x[2];
                    draft.kB = 1;
                    draft.a = curr.x[3];
                    draft.leader_w = curr.x[3];
                }
                // Fermion cohesion?
                else if (curr.ch == mirror.ch &&
                         curr.f == curr.t && mirror.f == mirror.t)
                {
                    if (curr.c[3] > mirror.c[3])
                    {
                        draft.c[0] = curr.x[0];
                        draft.c[1] = curr.x[1];
                        draft.c[2] = curr.x[2];
                        draft.a = min(curr.a, mirror.a);
                    }
                    else
                    {
                        draft.hB = 1;
                        draft.a = min(curr.a, mirror.a);
                    }
                }
                // Same affinity?
                else if (curr.a == mirror.a)
                {
                    if (curr.pB && !mirror.pB)
                    {
                        draft.c[0] = curr.c[0];
                        draft.c[1] = curr.c[1];
                        draft.c[2] = curr.c[2];
                    }
                    // Parallel transport?
                    else if (!curr.pB && mirror.pB)
                    {
                        draft.c[0] = dev_EL + (curr.x[0] - mirror.x[0]) % dev_EL;
                        draft.c[1] = dev_EL + (curr.x[1] - mirror.x[1]) % dev_EL;
                        draft.c[2] = dev_EL + (curr.x[2] - mirror.x[2]) % dev_EL;
                    }
                }
                // Strong interaction
                else if (dev_neutralColor(curr, mirror))
                {
                    // Gluon x gluon
                    if (curr.f > curr.t && mirror.f > mirror.t)
                    {
                        draft.c[0] = curr.x[0];
                        draft.c[1] = curr.x[1];
                        draft.c[2] = curr.x[2];
                        draft.ch = (curr.ch & ~COLOR_MASK) | (mirror.ch & COLOR_MASK);
                    }
                    // Quark x gluon
                    else if (curr.f == curr.t && mirror.f > mirror.t)
                    {
                        draft.c[0] = curr.x[0];
                        draft.c[1] = curr.x[1];
                        draft.c[2] = curr.x[2];
                        draft.ch = (curr.ch & ~COLOR_MASK) | (mirror.ch & COLOR_MASK);
                    }
                }
                // Electroweak interaction: Harmonic?
                else if (curr.phiB && mirror.phiB)
                {
                    // Weak interaction
                    if (dev_neutralWeak(curr, mirror))
                    {
                        if ((curr.pB && !mirror.pB) || (curr.sB && mirror.sB))
                        {
                            draft.c[0] = curr.x[0];
                            draft.c[1] = curr.x[1];
                            draft.c[2] = curr.x[2];
                            draft.kB = 1;
                        }
                    }
                    // Electric interaction
                    else if (curr.pB)
                    {
                        draft.c[0] = curr.x[0];
                        draft.c[1] = curr.x[1];
                        draft.c[2] = curr.x[2];
                        if (mirror.pB)
                        {
                            draft.kB = 1;
                        }
                        else
                        {
                            draft.a = mirror.a;
                            draft.leader_w = (mirror.a == dev_W_USED ? DEV_NO_LEADER_W : mirror.a);
                            draft.t = mirror.t;
                            draft.c[0] = mirror.x[0];
                            draft.c[1] = mirror.x[1];
                            draft.c[2] = mirror.x[2];
                        }
                    }
                    // Magnetic interaction
                    else if (curr.sB)
                    {
                        draft.c[0] = curr.x[0];
                        draft.c[1] = curr.x[1];
                        draft.c[2] = curr.x[2];
                        if (mirror.sB)
                        {
                            draft.kB = 1;
                        }
                        else
                        {
                            draft.a = mirror.a;
                            draft.leader_w = (mirror.a == dev_W_USED ? DEV_NO_LEADER_W : mirror.a);
                            draft.t = mirror.t;
                            draft.c[0] = mirror.x[0];
                            draft.c[1] = mirror.x[1];
                            draft.c[2] = mirror.x[2];
                        }
                    }
                }
            }
        }
    }
    // Different sectors
    else
    {
        // Singularization
        if (curr.ch == ((~mirror.ch) & CHARGE_MASK))
        {
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.a = curr.x[3];
            draft.leader_w = curr.x[3];
        }
        // Electroweak interaction: Harmonic?
        else if (curr.phiB && mirror.phiB)
        {
            // Weak interaction
            if (dev_neutralWeak(curr, mirror))
            {
                if ((curr.pB && !mirror.pB) || (curr.sB && mirror.sB))
                {
                    draft.c[0] = curr.x[0];
                    draft.c[1] = curr.x[1];
                    draft.c[2] = curr.x[2];
                    draft.kB = 1;
                }
            }
            // Electric interaction
            else if (curr.pB)
            {
                draft.c[0] = curr.x[0];
                draft.c[1] = curr.x[1];
                draft.c[2] = curr.x[2];
                if (mirror.pB)
                {
                    draft.kB = 1;
                }
            }
            // Magnetic interaction
            else if (curr.sB)
            {
                draft.c[0] = curr.x[0];
                draft.c[1] = curr.x[1];
                draft.c[2] = curr.x[2];
                if (mirror.sB)
                {
                    draft.kB = 1;
                }
            }
        }
    }
}

// ===================================================================
// K/S/D/P SOURCE-INTERACTION HELPERS
// ===================================================================

static __device__ inline int dev_Q(const ::CellDevice& c)
{
    return (c.ch & 0x08) ? 1 : 0;
}

static __device__ inline ::CellDevice& dev_source_center_cell(::CellDevice* lattice, int w)
{
    int cx, cy, cz;
    dev_source_center((unsigned)w, cx, cy, cz);
    return d_getCell(lattice, cx, cy, cz, w);
}

static __device__ inline void dev_reemitSourceAt(::CellDevice& srcDraft,
                                                  int dx, int dy, int dz,
                                                  ::CellDevice* /*d_draft*/)
{
    // Preserve the long-term momentum direction m; accumulate the pending
    // displacement in the consumable relocation vector reloc.
    srcDraft.reloc[0] += dx;
    srcDraft.reloc[1] += dy;
    srcDraft.reloc[2] += dz;
    srcDraft.t = 0;
    srcDraft.f = 0;
}

static __device__ inline void dev_moveOneStep(::CellDevice& srcDraft,
                                              int fromCx, int fromCy, int fromCz,
                                              int toCx, int toCy, int toCz,
                                              ::CellDevice* d_draft)
{
    int M = (int)dev_EL;
    int dx = dev_sign(dev_shortest_delta(fromCx, toCx, M));
    int dy = dev_sign(dev_shortest_delta(fromCy, toCy, M));
    int dz = dev_sign(dev_shortest_delta(fromCz, toCz, M));
    dev_reemitSourceAt(srcDraft, dx, dy, dz, d_draft);
}

static __device__ inline void dev_moveOneStepAway(::CellDevice& srcDraft,
                                                  int selfCx, int selfCy, int selfCz,
                                                  int otherCx, int otherCy, int otherCz,
                                                  ::CellDevice* d_draft)
{
    int M = (int)dev_EL;
    int dx = dev_sign(dev_shortest_delta(otherCx, selfCx, M));
    int dy = dev_sign(dev_shortest_delta(otherCy, selfCy, M));
    int dz = dev_sign(dev_shortest_delta(otherCz, selfCz, M));
    dev_reemitSourceAt(srcDraft, dx, dy, dz, d_draft);
}

static __device__ inline void dev_reemitAtContact(::CellDevice& srcDraft,
                                                  const ::CellDevice& contact,
                                                  ::CellDevice* d_draft)
{
    int M = (int)dev_EL;
    int dx = dev_shortest_delta((int)srcDraft.x[0], (int)contact.x[0], M);
    int dy = dev_shortest_delta((int)srcDraft.x[1], (int)contact.x[1], M);
    int dz = dev_shortest_delta((int)srcDraft.x[2], (int)contact.x[2], M);
    dev_reemitSourceAt(srcDraft, dx, dy, dz, d_draft);
}

static __device__ inline bool dev_canFormPair(const ::CellDevice& a, const ::CellDevice& b)
{
    uint8_t ca = a.ch;
    uint8_t cb = b.ch;
    if (ca == 0x00 && cb == 0x00) return true;
    if (ca == 0x3F && cb == 0x3F) return true;
    if ((ca ^ cb) == 0x3F) return true;

    bool qa = (ca & 0x08) != 0;
    bool qb = (cb & 0x08) != 0;
    bool w1a = (ca & 0x20) != 0;
    bool w1b = (cb & 0x20) != 0;
    bool w0a = (ca & 0x10) != 0;
    bool w0b = (cb & 0x10) != 0;
    uint8_t cola = ca & 0x07;
    uint8_t colb = cb & 0x07;

    if ((qa ^ qb) && (w1a == w1b) && (w0a ^ w0b) && ((cola ^ colb) == 0x07)) return true;
    if (!qa && !qb && !w1a && !w1b && w0a && w0b && cola == colb && cola != 0x00 && cola != 0x07) return true;
    if (qa && qb && w1a && w1b && !w0a && !w0b && cola == colb && cola != 0x00 && cola != 0x07) return true;
    return false;
}

static __device__ inline void dev_adoptLeader(::CellDevice& dst, uint32_t leader)
{
    dst.leader_w = leader;
    dst.a = leader;
}

static __device__ inline uint32_t dev_dominantLeader(const ::CellDevice& a, const ::CellDevice& b)
{
    uint32_t leader = (a.leader_w < b.leader_w) ? a.leader_w : b.leader_w;
    if (leader == DEV_NO_LEADER_W)
        leader = (a.x[3] < b.x[3]) ? a.x[3] : b.x[3];
    return leader;
}

__device__ inline void dev_convolute7(::CellDevice& curr, ::CellDevice& draft,
                                      ::CellDevice& mirror, unsigned /*w*/, unsigned /*tid*/,
                                      ::CellDevice* d_curr, ::CellDevice* d_draft)
{
    if (!curr.active || !mirror.active)
        return;
    if (curr.x[3] == mirror.x[3])
        return;

    // Sieve: the electroweak interaction channel is only active where s2B is set.
    if (curr.s2B == 0)
        return;

    int currW   = (int)curr.x[3];
    int mirrorW = (int)mirror.x[3];

    ::CellDevice& currSrc   = dev_source_center_cell(d_curr, currW);
    ::CellDevice& mirrorSrc = dev_source_center_cell(d_curr, mirrorW);
    ::CellDevice& currDraft = dev_source_center_cell(d_draft, currW);
    ::CellDevice& mirrorDraft = dev_source_center_cell(d_draft, mirrorW);

    int currCx, currCy, currCz;
    dev_source_center((unsigned)currW, currCx, currCy, currCz);
    int mirrorCx, mirrorCy, mirrorCz;
    dev_source_center((unsigned)mirrorW, mirrorCx, mirrorCy, mirrorCz);

    bool samePos = (curr.x[0] == mirror.x[0] &&
                    curr.x[1] == mirror.x[1] &&
                    curr.x[2] == mirror.x[2]);
    bool sameT   = (curr.t == mirror.t);

    // Pair formation (photon-like P sources).
    if (samePos && sameT && dev_canFormPair(currSrc, mirrorSrc))
    {
        bool dressing = (currSrc.leader_w != DEV_NO_LEADER_W &&
                         currSrc.leader_w == mirrorSrc.leader_w);
        uint32_t newLeader = dressing ? currSrc.leader_w : DEV_NO_LEADER_W;
        uint32_t newA      = dressing ? newLeader : dev_W_USED;
        uint32_t parent    = dressing ? currSrc.leader_w : DEV_NO_PARENT;

        bool alreadyPaired = (currSrc.kind == SRC_P &&
                              mirrorSrc.kind == SRC_P &&
                              currSrc.pair_idx == (uint32_t)mirrorW &&
                              mirrorSrc.pair_idx == (uint32_t)currW);
        if (alreadyPaired)
            return;

        uint32_t newCount = 1;
        if (currSrc.kind == SRC_P) newCount += currSrc.pair_count;
        if (mirrorSrc.kind == SRC_P) newCount += mirrorSrc.pair_count;

        currDraft.kind  = SRC_P;
        mirrorDraft.kind = SRC_P;
        currDraft.pair_idx   = (uint32_t)mirrorW;
        mirrorDraft.pair_idx = (uint32_t)currW;
        currDraft.pair_count = newCount;
        mirrorDraft.pair_count = newCount;
        currDraft.leader_w  = newLeader;
        mirrorDraft.leader_w = newLeader;
        currDraft.a  = newA;
        mirrorDraft.a = newA;
        currDraft.parent  = parent;
        mirrorDraft.parent = parent;

        dev_reemitAtContact(currDraft, curr, d_draft);
        dev_reemitAtContact(mirrorDraft, mirror, d_draft);
        return;
    }

    // pB triggers the electric channel, sB the magnetic channel.
    bool electricContact  = curr.pB || mirror.pB;
    bool magneticContact  = curr.sB || mirror.sB;
    bool electricCollapse = curr.pB && mirror.pB;
    bool magneticCollapse = curr.sB && mirror.sB;
    bool collapse         = electricCollapse || magneticCollapse;

    if (!electricContact && !magneticContact)
        return;

    if (collapse)
    {
        draft.kB = 1;
        draft.cB = 1;
    }
    else
    {
        uint32_t minLeader = dev_dominantLeader(currSrc, mirrorSrc);
        dev_adoptLeader(currDraft, minLeader);
        dev_adoptLeader(mirrorDraft, minLeader);
        uint32_t tmpT = currDraft.t;
        currDraft.t  = mirrorDraft.t;
        mirrorDraft.t = tmpT;

        dev_moveOneStep(currDraft,  currCx,  currCy,  currCz,
                        mirrorCx, mirrorCy, mirrorCz, d_draft);
        dev_moveOneStep(mirrorDraft, mirrorCx, mirrorCy, mirrorCz,
                        currCx,  currCy,  currCz, d_draft);
        return;
    }

    // 1. K x K
    if (currSrc.kind == SRC_K && mirrorSrc.kind == SRC_K)
    {
        dev_moveOneStepAway(currDraft, currCx, currCy, currCz,
                            mirrorCx, mirrorCy, mirrorCz, d_draft);
        return;
    }

    // 2. S x K (current = S, mirror = K)
    if (currSrc.kind == SRC_S && mirrorSrc.kind == SRC_K)
    {
        currDraft.kind = SRC_D;
        currDraft.parent = (uint32_t)mirrorW;
        dev_adoptLeader(currDraft, mirrorSrc.leader_w == DEV_NO_LEADER_W ? (uint32_t)mirrorW : mirrorSrc.leader_w);
        currDraft.spin_target = 1;
        dev_moveOneStep(currDraft, currCx, currCy, currCz,
                        mirrorCx, mirrorCy, mirrorCz, d_draft);
        return;
    }

    // 3. S x S
    if (currSrc.kind == SRC_S && mirrorSrc.kind == SRC_S)
    {
        if (dev_Q(currSrc) == dev_Q(mirrorSrc))
        {
            dev_moveOneStepAway(currDraft, currCx, currCy, currCz,
                                mirrorCx, mirrorCy, mirrorCz, d_draft);
        }
        else
        {
            uint32_t leader = dev_dominantLeader(currSrc, mirrorSrc);
            currDraft.kind  = SRC_D;
            mirrorDraft.kind = SRC_D;
            currDraft.parent  = leader;
            mirrorDraft.parent = leader;
            dev_adoptLeader(currDraft, leader);
            dev_adoptLeader(mirrorDraft, leader);
        }
        return;
    }

    // 4. S x D / D x S
    if ((currSrc.kind == SRC_S && mirrorSrc.kind == SRC_D) ||
        (currSrc.kind == SRC_D && mirrorSrc.kind == SRC_S))
    {
        ::CellDevice* sDraft  = (currSrc.kind == SRC_S ? &currDraft : &mirrorDraft);
        ::CellDevice* dDraft  = (currSrc.kind == SRC_S ? &mirrorDraft : &currDraft);
        ::CellDevice* dSrc    = (currSrc.kind == SRC_S ? &mirrorSrc : &currSrc);

        int sCx = (currSrc.kind == SRC_S ? currCx : mirrorCx);
        int sCy = (currSrc.kind == SRC_S ? currCy : mirrorCy);
        int sCz = (currSrc.kind == SRC_S ? currCz : mirrorCz);
        int dCx = (currSrc.kind == SRC_S ? mirrorCx : currCx);
        int dCy = (currSrc.kind == SRC_S ? mirrorCy : currCy);
        int dCz = (currSrc.kind == SRC_S ? mirrorCz : currCz);

        uint32_t leader = (dSrc->leader_w == DEV_NO_LEADER_W ? dSrc->parent : dSrc->leader_w);
        if (leader == DEV_NO_LEADER_W) leader = dSrc->x[3];
        sDraft->kind = SRC_D;
        sDraft->parent = (dSrc->parent == DEV_NO_PARENT ? leader : dSrc->parent);
        dev_adoptLeader(*sDraft, leader);

        dev_moveOneStep(*sDraft, sCx, sCy, sCz, dCx, dCy, dCz, d_draft);
        dev_moveOneStep(*dDraft, dCx, dCy, dCz, sCx, sCy, sCz, d_draft);
        return;
    }

    // 5. D x D
    if (currSrc.kind == SRC_D && mirrorSrc.kind == SRC_D)
    {
        if (currSrc.parent != mirrorSrc.parent)
        {
            dev_moveOneStepAway(currDraft, currCx, currCy, currCz,
                                mirrorCx, mirrorCy, mirrorCz, d_draft);
            currDraft.reloc[0] += mirrorSrc.m[0];
            currDraft.reloc[1] += mirrorSrc.m[1];
            currDraft.reloc[2] += mirrorSrc.m[2];
        }
        else
        {
            currDraft.spin_target = mirrorSrc.spin_target;
            uint32_t leader = dev_dominantLeader(currSrc, mirrorSrc);
            dev_adoptLeader(currDraft, leader);
        }
        return;
    }

    // 6. P x K / P x D / P x S
    if (currSrc.kind == SRC_P &&
        (mirrorSrc.kind == SRC_K || mirrorSrc.kind == SRC_S || mirrorSrc.kind == SRC_D))
    {
        dev_reemitAtContact(currDraft, curr, d_draft);
        mirrorDraft.reloc[0] += currSrc.m[0];
        mirrorDraft.reloc[1] += currSrc.m[1];
        mirrorDraft.reloc[2] += currSrc.m[2];
        return;
    }

    // 7. K/D/S x P
    if ((currSrc.kind == SRC_K || currSrc.kind == SRC_S || currSrc.kind == SRC_D) &&
        mirrorSrc.kind == SRC_P)
    {
        dev_reemitAtContact(currDraft, curr, d_draft);
        currDraft.reloc[0] += mirrorSrc.m[0];
        currDraft.reloc[1] += mirrorSrc.m[1];
        currDraft.reloc[2] += mirrorSrc.m[2];
        return;
    }

    // P x P is suppressed.
}

// ===================================================================
// MAIN UPDATE KERNEL - COMPLETE CA LOGIC
// ===================================================================

__global__ void ca_update_kernel(::CellDevice* d_curr, ::CellDevice* d_draft, ::CellDevice* d_mirror,
                                 unsigned CONVOL, unsigned GSLOT_Z,
                                 unsigned SLOT1, unsigned SLOT2, unsigned SLOT3,
                                 unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6,
                                 unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE,
                                 unsigned FLOOD, unsigned FRAME, int scenario)
{
    unsigned int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (dev_EL == 0 || dev_W_USED == 0) return;

    unsigned int total_cells = dev_EL * dev_EL * dev_EL * dev_W_USED;
    if (idx >= total_cells) return;

    // Map 1D -> 4D
    unsigned w = idx % dev_W_USED;
    unsigned idx_3d = idx / dev_W_USED;
    unsigned z = idx_3d % dev_EL;
    unsigned y = (idx_3d / dev_EL) % dev_EL;
    unsigned x = idx_3d / (dev_EL * dev_EL);
    
    ::CellDevice curr = d_getCell(d_curr, x, y, z, w);
    ::CellDevice draft = curr;
    ::CellDevice mirror = d_getCell(d_mirror, x, y, z, w);

    // Neighbors (spherical antipodal wrap for spatial, periodic for W)
    ::CellDevice forward = d_getNeighbor(d_curr, x, y, z, w, 6); // FORWARD
    ::CellDevice north   = d_getNeighbor(d_curr, x, y, z, w, 0); // NORTH
    ::CellDevice east    = d_getNeighbor(d_curr, x, y, z, w, 1); // EAST
    ::CellDevice south   = d_getNeighbor(d_curr, x, y, z, w, 2); // SOUTH
    ::CellDevice west    = d_getNeighbor(d_curr, x, y, z, w, 3); // WEST
    ::CellDevice up      = d_getNeighbor(d_curr, x, y, z, w, 4); // UP
    ::CellDevice down    = d_getNeighbor(d_curr, x, y, z, w, 5); // DOWN

    // ===================================================================
    // CONVOLUTION PHASE (k < CONVOL)
    // ===================================================================
    if (curr.k < CONVOL) {
        switch (scenario) {
            case 0: dev_convolute0(curr, draft, mirror, w, idx); break;
            case 1: dev_convolute1(curr, draft, mirror, w, idx); break;
            case 2: dev_convolute2(curr, draft, mirror, w, idx); break;
            case 3: dev_convolute3(curr, draft, mirror, w, idx); break;
            case 4: dev_convolute4(curr, draft, mirror, w, idx); break;
            case 5: dev_convolute5(curr, draft, mirror, w, idx); break;
            case 6: dev_convolute6(curr, draft, mirror, w, idx); break;
            case 7: dev_convolute7(curr, draft, mirror, w, idx, d_curr, d_draft); break;
            default: break;
        }
    }
    // ===================================================================
    // GSLOT PHASES (not yet implemented)
    // ===================================================================
    else if (curr.k < GSLOT_Z) {
        // timing slots – reserved for glider transport
    }
    // ===================================================================
    // DIFFUSION PHASE (k < DIFFUSION)
    // ===================================================================
    else if (curr.k < DIFFUSION) {
        // SLOT I
        if (curr.k < SLOT1) {
            if ((north.a == dev_W_USED && curr.r2 >= north.r2) ||
                (west.a  == dev_W_USED && curr.r2 >= west.r2)  ||
                (down.a  == dev_W_USED && curr.r2 >= down.r2)  ||
                (south.a == dev_W_USED && curr.r2 >= south.r2) ||
                (east.a  == dev_W_USED && curr.r2 >= east.r2)  ||
                (up.a    == dev_W_USED && curr.r2 >= up.r2)) {
                draft.a = dev_W_USED;
                draft.leader_w = DEV_NO_LEADER_W;
            }
        }
        // SLOT II
        if (curr.k < SLOT2) {
            if ((north.a == dev_W_USED && curr.r2 >= north.r2) ||
                (west.a  == dev_W_USED && curr.r2 >= west.r2)  ||
                (down.a  == dev_W_USED && curr.r2 >= down.r2)  ||
                (south.a == dev_W_USED && curr.r2 >= south.r2) ||
                (east.a  == dev_W_USED && curr.r2 >= east.r2)  ||
                (up.a    == dev_W_USED && curr.r2 >= up.r2)) {
                draft.a = dev_W_USED;
                draft.leader_w = DEV_NO_LEADER_W;
            }
            // Hunting using hB (matches CPU: pulse_from_time condition, no modulo on c[])
            if (curr.active) {
                if (north.hB) { draft.c[0] = north.c[0] + 1; curr.sB = !draft.hB; }
                else if (west.hB)  { draft.c[1] = west.c[1] + 1; curr.sB = !draft.hB; }
                else if (down.hB)  { draft.c[2] = down.c[2] + 1; curr.sB = !draft.hB; }
                else if (south.hB) { draft.c[1] = south.c[1] + 1; curr.sB = !draft.hB; }
                else if (east.hB)  { draft.c[0] = east.c[0] + 1; curr.sB = !draft.hB; }
                else if (up.hB)    { draft.c[2] = up.c[2] + 1; curr.sB = !draft.hB; }
            }
        }
        // SLOT III
        else if (curr.k < SLOT3) {
            // Propagate c[] to all cells in layer 0 (matches CPU)
            if (curr.x[3] == 0) {
                if (!ZERO_C(north.c)) {
                    draft.c[0] = north.c[0]; draft.c[1] = north.c[1]; draft.c[2] = north.c[2];
                    if (north.kB) draft.kB = north.kB;
                } else if (!ZERO_C(south.c)) {
                    draft.c[0] = south.c[0]; draft.c[1] = south.c[1]; draft.c[2] = south.c[2];
                    if (south.kB) draft.kB = south.kB;
                } else if (!ZERO_C(east.c)) {
                    draft.c[0] = east.c[0]; draft.c[1] = east.c[1]; draft.c[2] = east.c[2];
                    if (east.kB) draft.kB = east.kB;
                } else if (!ZERO_C(west.c)) {
                    draft.c[0] = west.c[0]; draft.c[1] = west.c[1]; draft.c[2] = west.c[2];
                    if (west.kB) draft.kB = west.kB;
                } else if (!ZERO_C(up.c)) {
                    draft.c[0] = up.c[0]; draft.c[1] = up.c[1]; draft.c[2] = up.c[2];
                    if (up.kB) draft.kB = up.kB;
                } else if (!ZERO_C(down.c)) {
                    draft.c[0] = down.c[0]; draft.c[1] = down.c[1]; draft.c[2] = down.c[2];
                    if (down.kB) draft.kB = down.kB;
                }
            }

            draft.f = max(down.f, max(west.f, max(north.f,
                        max(south.f, max(east.f, up.f)))));

            if (!curr.cB) {
                if (north.cB && north.r2 > curr.r2) {
                    draft.cB = 1;
                    if (north.a != dev_W_USED) { draft.a = north.a; draft.leader_w = north.a; }
                } else if (south.cB && south.r2 > curr.r2) {
                    draft.cB = 1;
                    if (south.a != dev_W_USED) { draft.a = south.a; draft.leader_w = south.a; }
                } else if (east.cB && east.r2 > curr.r2) {
                    draft.cB = 1;
                    if (east.a != dev_W_USED) { draft.a = east.a; draft.leader_w = east.a; }
                } else if (west.cB && west.r2 > curr.r2) {
                    draft.cB = 1;
                    if (west.a != dev_W_USED) { draft.a = west.a; draft.leader_w = west.a; }
                } else if (down.cB && down.r2 > curr.r2) {
                    draft.cB = 1;
                    if (down.a != dev_W_USED) { draft.a = down.a; draft.leader_w = down.a; }
                } else if (up.cB && up.r2 > curr.r2) {
                    draft.cB = 1;
                    if (up.a != dev_W_USED) { draft.a = up.a; draft.leader_w = up.a; }
                }
            }
        }
        // SLOT IV (matches CPU: centered coordinates + clamp)
        else if (curr.k < SLOT4) {
            if (forward.kB && forward.a == curr.a) {
                int half = (int)dev_EL / 2;
                int cx = (int)curr.x[0] - half;
                int cy = (int)curr.x[1] - half;
                int cz = (int)curr.x[2] - half;
                int fx = (int)forward.x[0] - half;
                int fy = (int)forward.x[1] - half;
                int fz = (int)forward.x[2] - half;
                int delta_x = cx - fx;
                int delta_y = cy - fy;
                int delta_z = cz - fz;
                int ncx = (int)forward.c[0] + delta_x;
                int ncy = (int)forward.c[1] + delta_y;
                int ncz = (int)forward.c[2] + delta_z;
                ncx = max(0, min((int)dev_EL - 1, ncx));
                ncy = max(0, min((int)dev_EL - 1, ncy));
                ncz = max(0, min((int)dev_EL - 1, ncz));
                draft.c[0] = ncx;
                draft.c[1] = ncy;
                draft.c[2] = ncz;
                draft.kB = forward.kB;
                draft.cB = forward.cB;
            }
            draft.f = max(forward.f, curr.f);
        }
        // SLOT V
        else if (curr.k < SLOT5) {
            if (curr.a == dev_W_USED && curr.r2 < curr.t * curr.t) {
                draft.a = curr.x[3];
                draft.leader_w = curr.x[3];
            }
        }
    }
    // ===================================================================
    // RELOCATION PHASE (k < RELOC) — restore original coordinates (matches CPU)
    // ===================================================================
    else if (curr.k < RELOC) {
        // Save 3D address (CPU restores after relocation)
        unsigned save_x = curr.x[0];
        unsigned save_y = curr.x[1];
        unsigned save_z = curr.x[2];
        // SLOT VI (x direction)
        if (curr.k < SLOT6) {
            if (north.c[0] > 0) {
                draft = north;
                draft.c[0]--;
            }
        }
        // SLOT VII (y direction)
        else if (curr.k < SLOT7) {
            if (west.c[1] > 0) {
                draft = west;
                draft.c[1]--;
            }
        }
        // SLOT VIII (z direction)
        else if (curr.k < SLOT8) {
            if (down.c[2] > 0) {
                draft = down;
                draft.c[2]--;
            }
        }
        // Restore original 3D address (matches CPU relocate)
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
        if (curr.active) {
            if (north.r2 > curr.r2) { draft.a = north.a; draft.leader_w = (north.a == dev_W_USED ? DEV_NO_LEADER_W : north.a); }
            if (south.r2 > curr.r2) { draft.a = south.a; draft.leader_w = (south.a == dev_W_USED ? DEV_NO_LEADER_W : south.a); }
            if (east.r2 > curr.r2) { draft.a = east.a; draft.leader_w = (east.a == dev_W_USED ? DEV_NO_LEADER_W : east.a); }
            if (west.r2 > curr.r2) { draft.a = west.a; draft.leader_w = (west.a == dev_W_USED ? DEV_NO_LEADER_W : west.a); }
            if (up.r2 > curr.r2) { draft.a = up.a; draft.leader_w = (up.a == dev_W_USED ? DEV_NO_LEADER_W : up.a); }
            if (down.r2 > curr.r2) { draft.a = down.a; draft.leader_w = (down.a == dev_W_USED ? DEV_NO_LEADER_W : down.a); }
        }
        if (curr.cB) {
            draft.cB = 0;
            if (curr.a != dev_W_USED && curr.r2 < 4) {
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

    // Update frame counters
    draft.k = (curr.k + 1) % FRAME;
    if (draft.k == 0) {
        if (curr.a == dev_W_USED && curr.t <= dev_RMAX) {
            draft.t++;
        } else {
            draft.t = (curr.t + 1) % (2 * dev_RMAX);
        }
    }

    d_getCell(d_draft, x, y, z, w) = draft;
}

// ===================================================================
// MIRROR UPDATE KERNEL (only when k == 0)
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
// SHIFT-MIRROR KERNEL (cyclic shift along w dimension)
// ===================================================================
__global__ void shiftMirrorKernel(const CellDevice* src,
                                  CellDevice* dst,
                                  unsigned totalCells)
{
    unsigned tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= totalCells) return;
    unsigned w = tid % dev_W_USED;
    unsigned idx3d = tid / dev_W_USED;
    unsigned src_w = (w + dev_W_USED - 1) % dev_W_USED;
    unsigned src_tid = idx3d * dev_W_USED + src_w;
    dst[tid] = src[src_tid];
}

// ===================================================================
// CONSTANT MEMORY SETUP (must be in same .cu as kernels)
// ===================================================================

extern "C" void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX)
{
    cudaError_t err;
    unsigned CENTER = (EL - 1) / 2;

    printf("Setting dev_EL = %u\n", EL);
    err = cudaMemcpyToSymbol(dev_EL, &EL, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_EL: %s (code %d)\n",
                cudaGetErrorString(err), err);
        return;
    }

    printf("Setting dev_W_USED = %u\n", W_USED);
    err = cudaMemcpyToSymbol(dev_W_USED, &W_USED, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_W_USED: %s (code %d)\n",
                cudaGetErrorString(err), err);
        return;
    }

    printf("Setting dev_RMAX = %u\n", RMAX);
    err = cudaMemcpyToSymbol(dev_RMAX, &RMAX, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_RMAX: %s (code %d)\n",
                cudaGetErrorString(err), err);
        return;
    }

    printf("Setting dev_CENTER = %u\n", CENTER);
    err = cudaMemcpyToSymbol(dev_CENTER, &CENTER, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_CENTER: %s (code %d)\n",
                cudaGetErrorString(err), err);
        return;
    }

    int initCtrl = 1;
    err = cudaMemcpyToSymbol(dev_ctrl, &initCtrl, sizeof(int));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_ctrl: %s (code %d)\n",
                cudaGetErrorString(err), err);
        return;
    }

    printf("All constants set successfully\n");
}

extern "C" void setCudaSourceCenters(const unsigned* centers, unsigned W)
{
    if (W > 32) W = 32;
    if (W == 0) return;
    cudaError_t err = cudaMemcpyToSymbol(dev_lcenters, centers, W * 3 * sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_lcenters: %s (code %d)\n",
                cudaGetErrorString(err), err);
    }
}

extern "C" void resetCudaCtrl()
{
    int val = 1;
    cudaError_t err = cudaMemcpyToSymbol(dev_ctrl, &val, sizeof(int));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error resetting dev_ctrl: %s (code %d)\n",
                cudaGetErrorString(err), err);
    } else {
        printf("dev_ctrl reset to 1\n");
    }
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
    // Constants are set by setCudaConstants (called from bridge_cuda.cu)
    if (!init_cuda_memory(EL, W_USED)) return false;
    g_cuda_initialized = true;
    printf("✓ CUDA simulation initialized successfully\n");
    return true;
}

void free_cuda_memory()
{
    if (d_lattice_curr) cudaFree(d_lattice_curr);
    if (d_lattice_draft) cudaFree(d_lattice_draft);
    if (d_lattice_mirror) cudaFree(d_lattice_mirror);
    d_lattice_curr = d_lattice_draft = d_lattice_mirror = nullptr;
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
    size_t size = totalCells * sizeof(::CellDevice);
    CUDA_CHECK(cudaMemcpy(d_lattice_curr, hostCells, size, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_lattice_draft, d_lattice_curr, size, cudaMemcpyDeviceToDevice));
    CUDA_CHECK(cudaMemcpy(d_lattice_mirror, d_lattice_curr, size, cudaMemcpyDeviceToDevice));
    return true;
}

bool downloadLatticeFromCuda(::CellDevice* hostCells, size_t totalCells)
{
    if (!g_cuda_initialized) return false;
    size_t size = totalCells * sizeof(::CellDevice);
    CUDA_CHECK(cudaMemcpy(hostCells, d_lattice_curr, size, cudaMemcpyDeviceToHost));
    return true;
}

void cudaSimulationStep(
    unsigned CONVOL, unsigned GSLOT_Z,
    unsigned SLOT1, unsigned SLOT2, unsigned SLOT3,
    unsigned SLOT4, unsigned DIFFUSION, unsigned SLOT5, unsigned SLOT6,
    unsigned SLOT7, unsigned SLOT8, unsigned RELOC, unsigned REISSUE,
    unsigned FLOOD, unsigned FRAME, unsigned RMAX, int scenario,
    unsigned pulse_tick)
{
    if (!g_cuda_initialized) {
        fprintf(stderr, "ERROR: CUDA not initialized\n");
        return;
    }

    // Get dimensions from automaton namespace (they are host variables)
    unsigned L = automaton::EL;
    unsigned W = automaton::W_USED;
    if (L == 0 || W == 0) {
        fprintf(stderr, "ERROR: EL or W_USED is zero\n");
        return;
    }

    size_t total_cells = (size_t)L * L * L * W;
    if (total_cells == 0) {
        fprintf(stderr, "ERROR: total_cells = 0\n");
        return;
    }

    const int BLOCK_SIZE = 256;
    int GRID = (int)((total_cells + BLOCK_SIZE - 1) / BLOCK_SIZE);
    // printf("Launching kernel: total_cells=%zu, GRID=%d, BLOCK=%d, EL=%u, W_USED=%u, RMAX=%u\n",
    //        total_cells, GRID, BLOCK_SIZE, L, W, RMAX);

    // Upload current source centers before the phase step uses them.
    if (W > 0 && !automaton::lcenters.empty())
        setCudaSourceCenters(automaton::lcenters[0].data(), W);

    // Phase step: update r2/r, (u,v), active and emergent pB/sB/phiB into d_lattice_draft,
    // then swap so the main CA kernel reads the updated phase.
    phase_step_kernel<<<GRID, BLOCK_SIZE>>>(d_lattice_curr, d_lattice_draft, pulse_tick);

    cudaError_t phaseErr = cudaGetLastError();
    if (phaseErr != cudaSuccess) {
        fprintf(stderr, "phase_step_kernel launch failed: %s\n", cudaGetErrorString(phaseErr));
        return;
    }
    phaseErr = cudaDeviceSynchronize();
    if (phaseErr != cudaSuccess) {
        fprintf(stderr, "phase_step_kernel execution failed: %s\n", cudaGetErrorString(phaseErr));
        return;
    }

    ::CellDevice* phaseTmp = d_lattice_curr;
    d_lattice_curr = d_lattice_draft;
    d_lattice_draft = phaseTmp;

    // Launch main CA kernel
    ca_update_kernel<<<GRID, BLOCK_SIZE>>>(
        d_lattice_curr, d_lattice_draft, d_lattice_mirror,
        CONVOL, GSLOT_Z, SLOT1, SLOT2, SLOT3, SLOT4, DIFFUSION,
        SLOT5, SLOT6, SLOT7, SLOT8, RELOC, REISSUE,
        FLOOD, FRAME, scenario
    );

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel launch failed: %s\n", cudaGetErrorString(err));
        return;
    }

    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel execution failed: %s\n", cudaGetErrorString(err));
        return;
    }

    // Swap curr and draft
    ::CellDevice* temp = d_lattice_curr;
    d_lattice_curr = d_lattice_draft;
    d_lattice_draft = temp;

    // Apply per-source relocation impulse: move the source center, preserve the
    // long-term momentum direction m, and clear the old center.  This mirrors
    // CPU applyMomentum().
    for (unsigned iw = 0; iw < W; ++iw)
    {
        int cx = (int)automaton::lcenters[iw][0];
        int cy = (int)automaton::lcenters[iw][1];
        int cz = (int)automaton::lcenters[iw][2];
        size_t idx = (size_t)((((cx * (int)L) + cy) * (int)L) + cz) * (int)W + iw;
        ::CellDevice centerCell;
        err = cudaMemcpy(&centerCell, d_lattice_curr + idx, sizeof(::CellDevice), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess)
            continue;

        // Free photon pairs expand and are gradually consumed. At maximum
        // radius (t == RMAX) one pair is consumed; when the stack empties the
        // two partner source centers are released as singletons moving apart.
        if (centerCell.kind == SRC_P &&
            centerCell.a == (uint32_t)W &&
            centerCell.pair_idx != DEV_NO_PAIR &&
            centerCell.pair_idx < (uint32_t)W &&
            centerCell.t == (uint32_t)RMAX &&
            centerCell.x[3] < centerCell.pair_idx)
        {
            if (centerCell.pair_count > 0)
                centerCell.pair_count--;

            uint32_t pw = centerCell.pair_idx;
            int pcx = (int)automaton::lcenters[pw][0];
            int pcy = (int)automaton::lcenters[pw][1];
            int pcz = (int)automaton::lcenters[pw][2];
            size_t idxPartner = (size_t)((((pcx * (int)L) + pcy) * (int)L) + pcz) * (int)W + (size_t)pw;
            ::CellDevice partner;
            err = cudaMemcpy(&partner, d_lattice_curr + idxPartner, sizeof(::CellDevice), cudaMemcpyDeviceToHost);
            if (err == cudaSuccess)
            {
                if (centerCell.pair_count == 0)
                {
                    centerCell.kind       = SRC_S;
                    centerCell.pair_idx   = DEV_NO_PAIR;
                    centerCell.pair_count = 0;
                    centerCell.leader_w   = DEV_NO_LEADER_W;
                    centerCell.a          = (uint32_t)W;

                    partner.kind       = SRC_S;
                    partner.pair_idx   = DEV_NO_PAIR;
                    partner.pair_count = 0;
                    partner.leader_w   = DEV_NO_LEADER_W;
                    partner.a          = (uint32_t)W;

                    int axis = (int)(centerCell.x[3] % 3u);
                    int sign = ((centerCell.x[3] & 1u) ? +1 : -1);
                    centerCell.reloc[axis] += sign;
                    partner.reloc[axis]    -= sign;
                }
                else
                {
                    partner.pair_count = centerCell.pair_count;
                }
                cudaMemcpy(d_lattice_curr + idxPartner, &partner, sizeof(::CellDevice), cudaMemcpyHostToDevice);
            }
        }

        int dx = centerCell.reloc[0];
        int dy = centerCell.reloc[1];
        int dz = centerCell.reloc[2];
        if (dx == 0 && dy == 0 && dz == 0)
            continue;

        // Update the long-term momentum direction from the consumed impulse.
        int new_m[3] = { centerCell.m[0], centerCell.m[1], centerCell.m[2] };
        {
            int abs_dx = (dx < 0) ? -dx : dx;
            int abs_dy = (dy < 0) ? -dy : dy;
            int abs_dz = (dz < 0) ? -dz : dz;
            int axis = 0, best = abs_dx;
            if (abs_dy > best) { axis = 1; best = abs_dy; }
            if (abs_dz > best) { axis = 2; }
            int val = (axis == 0 ? dx : (axis == 1 ? dy : dz));
            new_m[0] = new_m[1] = new_m[2] = 0;
            new_m[axis] = (val < 0) ? -1 : +1;
        }

        int M = (int)L;
        int nx = (cx + dx) % M;
        int ny = (cy + dy) % M;
        int nz = (cz + dz) % M;
        if (nx < 0) nx += M;
        if (ny < 0) ny += M;
        if (nz < 0) nz += M;

        size_t idxNew = (size_t)((((nx * (int)L) + ny) * (int)L) + nz) * (int)W + iw;
        ::CellDevice newCell;
        err = cudaMemcpy(&newCell, d_lattice_curr + idxNew, sizeof(::CellDevice), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess)
            continue;

        // Carry source identity, momentum direction, and re-seed the wave.
        newCell.kind        = centerCell.kind;
        newCell.parent      = centerCell.parent;
        newCell.spin_target = centerCell.spin_target;
        newCell.pair_idx    = centerCell.pair_idx;
        newCell.pair_count  = centerCell.pair_count;
        newCell.leader_w    = centerCell.leader_w;
        newCell.a           = centerCell.a;
        newCell.t           = 0;
        newCell.f           = 0;
        newCell.u           = 2048;
        newCell.v           = 0;
        newCell.m[0]        = new_m[0];
        newCell.m[1]        = new_m[1];
        newCell.m[2]        = new_m[2];
        newCell.reloc[0]    = newCell.reloc[1] = newCell.reloc[2] = 0;

        // Old cell is no longer a source center.
        centerCell.kind        = SRC_S;
        centerCell.parent      = DEV_NO_PARENT;
        centerCell.spin_target = 0;
        centerCell.pair_idx    = DEV_NO_PAIR;
        centerCell.pair_count  = 0;
        centerCell.leader_w    = DEV_NO_LEADER_W;
        centerCell.a           = (uint32_t)W;
        centerCell.t           = 0;
        centerCell.f           = 0;
        centerCell.u           = 0;
        centerCell.v           = 0;
        centerCell.m[0]        = centerCell.m[1] = centerCell.m[2] = 0;
        centerCell.reloc[0]    = centerCell.reloc[1] = centerCell.reloc[2] = 0;

        cudaMemcpy(d_lattice_curr + idxNew, &newCell, sizeof(::CellDevice), cudaMemcpyHostToDevice);
        cudaMemcpy(d_lattice_curr + idx, &centerCell, sizeof(::CellDevice), cudaMemcpyHostToDevice);

        automaton::lcenters[iw][0] = (unsigned)nx;
        automaton::lcenters[iw][1] = (unsigned)ny;
        automaton::lcenters[iw][2] = (unsigned)nz;
    }

    // Read new k from first cell (only after successful kernel execution)
    unsigned new_k = 0;
    err = cudaMemcpy(&new_k, &d_lattice_curr[0].k, sizeof(unsigned), cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to read k from device: %s\n", cudaGetErrorString(err));
        return;
    }

    if (new_k == 0) {
        updateMirrorKernel<<<GRID, BLOCK_SIZE>>>(d_lattice_curr, d_lattice_mirror, (unsigned)total_cells);
        cudaDeviceSynchronize();
    }

    if (new_k < CONVOL) {
        size_t size = (size_t)total_cells * sizeof(::CellDevice);
        err = cudaMemcpy(d_lattice_draft, d_lattice_mirror, size, cudaMemcpyDeviceToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "Mirror copy for shift failed: %s\n", cudaGetErrorString(err));
            return;
        }
        shiftMirrorKernel<<<GRID, BLOCK_SIZE>>>(d_lattice_draft, d_lattice_mirror, (unsigned)total_cells);
        cudaDeviceSynchronize();
    }
}

void cudaUpdateVoxelsLayer(unsigned selectedW)
{
    // TODO: implement GPU-accelerated voxel coloring (optional)
}

uint32_t* getMappedVoxels()
{
    return nullptr;
}

// ===================================================================
// NAMESPACE WRAPPER FUNCTIONS
// ===================================================================
namespace automaton 
{
    ::CellDevice convertToDevice(const Cell& src) {
        ::CellDevice dst;
        dst.ch = src.ch;
        dst.pB = src.pB ? 1 : 0;
        dst.sB = src.sB ? 1 : 0;
        dst.a = src.a;
        for (int i = 0; i < 4; ++i) dst.x[i] = src.x[i];
        dst.r2 = src.r2;
        dst.r = src.r;
        dst.u = src.u;
        dst.v = src.v;
        dst.active = src.active ? 1u : 0u;
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
        dst.gB = src.gB ? 1 : 0;
        for (int i = 0; i < 3; ++i) dst.g[i] = static_cast<int32_t>(src.g[i]);

        dst.kind = static_cast<uint8_t>(src.kind);
        dst.parent = src.parent;
        dst.spin_target = static_cast<int32_t>(src.spin_target);
        dst.pair_idx = src.pair_idx;
        dst.leader_w = src.leader_w;
        dst.pair_count = static_cast<uint32_t>(src.pair_count);
        for (int i = 0; i < 3; ++i) dst.m[i] = static_cast<int32_t>(src.m[i]);
        for (int i = 0; i < 3; ++i) dst.reloc[i] = static_cast<int32_t>(src.reloc[i]);
        return dst;
    }

    void convertToHost(const ::CellDevice& src, Cell& dst) {
        dst.ch = src.ch;
        dst.pB = src.pB != 0;
        dst.sB = src.sB != 0;
        dst.a = src.a;
        for (int i = 0; i < 4; ++i) dst.x[i] = src.x[i];
        dst.r2 = src.r2;
        dst.r = src.r;
        dst.u = src.u;
        dst.v = src.v;
        dst.active = src.active != 0;
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
        dst.gB = src.gB != 0;
        for (int i = 0; i < 3; ++i) dst.g[i] = static_cast<int>(src.g[i]);

        dst.kind = static_cast<SourceKind>(src.kind);
        dst.parent = src.parent;
        dst.spin_target = static_cast<int8_t>(src.spin_target);
        dst.pair_idx = src.pair_idx;
        dst.leader_w = src.leader_w;
        dst.pair_count = static_cast<uint8_t>(src.pair_count);
        for (int i = 0; i < 3; ++i) dst.m[i] = static_cast<int>(src.m[i]);
        for (int i = 0; i < 3; ++i) dst.reloc[i] = static_cast<int>(src.reloc[i]);
    }

    bool swap_lattices_gpu() { return true; }

    void free_cuda_memory() { ::free_cuda_memory(); }
}