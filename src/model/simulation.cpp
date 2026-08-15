/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include <thread>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <array>
#include <cstring>
#include "model/simulation.h"
#include "config.h"

#ifdef USE_CUDA
extern void cudaSimulationStepWrapper();
extern bool isCudaEnabled();
extern void updateMirrorOnGPU();
#endif

namespace automaton
{
  using namespace std;

  // Grid constants
  unsigned EL;
  unsigned W_DIM;
  unsigned W_USED;
  unsigned L2;
  unsigned L3 = 0;
  unsigned long BLOCK = 0;
  unsigned DIAG = 0;
  unsigned RMAX = 0;
  unsigned CONTRACT = 0;
  unsigned UPDATE = 0;
  unsigned CONVOL = 0;
  unsigned GSLOT_X = 0, GSLOT_Y = 0, GSLOT_Z = 0;
  unsigned SLOT1 = 0, SLOT2 = 0, SLOT3 = 0, SLOT4 = 0, SLOT5 = 0;
  unsigned SLOT6 = 0, SLOT7 = 0, SLOT8 = 0;
  unsigned DIFFUSION = 0;
  unsigned RELOC = 0;
  unsigned REISSUE = 0;
  unsigned FLOOD = 0;
  unsigned FRAME = 0;
  unsigned ORDER;
  unsigned CENTER;
  unsigned FCENTER;
  unsigned int pulse_tick = 0;

  // Lattices
  std::vector<Cell> lattice_curr;
  std::vector<Cell> lattice_draft;
  std::vector<Cell> lattice_mirror;

  string lastAllocationError;
  std::vector<std::array<unsigned, 3>> lcenters;

  // ============================================================
  // SPHERICAL (ANTIPODAL) WRAPPING
  // ============================================================

  inline void spherical_wrap(int& x, int& y, int& z, int& w)
  {
    if (x < 0 || x >= (int)EL ||
        y < 0 || y >= (int)EL ||
        z < 0 || z >= (int)EL ||
        w < 0 || w >= (int)W_USED)
    {
        x = EL - 1 - x;
        y = EL - 1 - y;
        z = EL - 1 - z;
        w = W_USED - 1 - w;

        x = (x % (int)EL + EL) % EL;
        y = (y % (int)EL + EL) % EL;
        z = (z % (int)EL + EL) % EL;
        w = (w % (int)W_USED + W_USED) % W_USED;
    }
  }

  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w)
  {
    lcenters[w][0] = x;
    lcenters[w][1] = y;
    lcenters[w][2] = z;
  }

  // ============================================================
  // PULSATING SPHERE — BFS WAVEFRONT PROPAGATION
  // ============================================================

  void update_pulsating_wavefront()
  {
    // Copy current r2 values into draft
    for (size_t i = 0; i < BLOCK; ++i)
        lattice_draft[i].r2 = lattice_curr[i].r2;

    for (unsigned w = 0; w < W_USED; ++w)
    {
        int cx = (int)lcenters[w][0];
        int cy = (int)lcenters[w][1];
        int cz = (int)lcenters[w][2];

    for (unsigned x = 0; x < EL; ++x)
    for (unsigned y = 0; y < EL; ++y)
    for (unsigned z = 0; z < EL; ++z)
    {
        Cell &curr = getCell(lattice_curr, x, y, z, w);

        if (curr.r2 == INF_R2)
            continue;

        int dx_ = (int)x - cx; int ax = dx_ < 0 ? -dx_ : dx_;
        int dy_ = (int)y - cy; int ay = dy_ < 0 ? -dy_ : dy_;
        int dz_ = (int)z - cz; int az = dz_ < 0 ? -dz_ : dz_;

        // 6-connected spatial neighbors (no w propagation)
        static const int offsets[6][3] = {
            {+1,0,0}, {-1,0,0},
            {0,+1,0}, {0,-1,0},
            {0,0,+1}, {0,0,-1}
        };

        for (int dir = 0; dir < 6; ++dir)
        {
            int nx = (int)x + offsets[dir][0];
            int ny = (int)y + offsets[dir][1];
            int nz = (int)z + offsets[dir][2];

            if (nx < 0 || nx >= (int)EL ||
                ny < 0 || ny >= (int)EL ||
                nz < 0 || nz >= (int)EL)
                continue;

            // Incremental r2 difference
            unsigned diff;
            if (dir < 2)
                diff = 2 * ax + 1;
            else if (dir < 4)
                diff = 2 * ay + 1;
            else
                diff = 2 * az + 1;

            unsigned int new_r2 = curr.r2 + diff;

            Cell &nxt = getCell(lattice_draft, nx, ny, nz, w);

            if (new_r2 < nxt.r2)
                nxt.r2 = new_r2;
        }
    }
    }

    // Ensure each source center stays at 0
    for (unsigned w = 0; w < W_USED; ++w)
    {
        int cx = (int)lcenters[w][0];
        int cy = (int)lcenters[w][1];
        int cz = (int)lcenters[w][2];
        getCell(lattice_draft, (unsigned)cx, (unsigned)cy, (unsigned)cz, w).r2 = 0;
    }

    // Copy r2 back to curr and update integer radius
    for (size_t i = 0; i < BLOCK; ++i)
    {
        lattice_curr[i].r2 = lattice_draft[i].r2;
        if (lattice_curr[i].r2 == INF_R2)
            lattice_curr[i].r = -1;
        else
            lattice_curr[i].r = isqrt((int)lattice_curr[i].r2);
    }
  }

  // ============================================================
  // RADIAL POLARISATION — (u,v) pair, evolved as a 3-D integer wave
  // ============================================================

  static void phase_step()
  {
    if (RMAX == 0 || BLOCK == 0)
      return;

    // Wave parameters (same scaling as the former SincWave test).
    int R = (RMAX > 0u) ? (int)RMAX : 1;
    int shellR = (int)((RMAX * 24u) / 100u);
    int shellW = (int)(RMAX / 5u);
    if (shellW < 1) shellW = 1;
    int absorbW = (R / 27 > 2) ? (R / 27) : 2;

    int diffDivShift = 2;
    if (R >= 384)       diffDivShift = 6;
    else if (R >= 192)  diffDivShift = 5;
    else if (R >= 96)   diffDivShift = 4;
    else if (R >= 40)   diffDivShift = 3;

    int velDampShift = diffDivShift + 3;
    constexpr int DIFF_SHIFT = 4;
    constexpr int SHELL_TARGET = 16384;

    int ELi = (int)EL;
    int Wi  = (int)W_USED;

    unsigned int pulseR2 = pulse_from_time(pulse_tick);
    unsigned int pulseTol = (EL * EL + 150u) / 300u;
    if (pulseTol < 1u) pulseTol = 1u;

    // First pass: compute next (u,v) and active/pB/sB into lattice_draft.
    for (int x = 0; x < ELi; ++x)
    for (int y = 0; y < ELi; ++y)
    for (int z = 0; z < ELi; ++z)
    for (int w = 0; w < Wi;  ++w)
    {
        const Cell& c = getCell(lattice_curr, x, y, z, w);
        Cell&       d = getCell(lattice_draft, x, y, z, w);

        // Start from the current CA state and overwrite only the (u,v) wave fields.
        d = c;

        unsigned int r2diff = (c.r2 > pulseR2) ? (c.r2 - pulseR2) : (pulseR2 - c.r2);
        bool active = (c.r2 != INF_R2 && r2diff <= pulseTol);

        // Hard zero on spatial boundaries and outside the processed sphere.
        if (x == 0 || x == ELi - 1 ||
            y == 0 || y == ELi - 1 ||
            z == 0 || z == ELi - 1 ||
            c.r < 0 || c.r >= R)
        {
            d.u = 0;
            d.v = 0;
            d.active = active ? 1u : 0u;
            d.phiB = active;
            d.pB = false;
            d.sB = false;
            d.s2B = false;
            continue;
        }

        int u = c.u;
        int v = c.v;
        int r = c.r;

        int neighbors_u =
            getCell(lattice_curr, x + 1, y, z, w).u
          + getCell(lattice_curr, x - 1, y, z, w).u
          + getCell(lattice_curr, x, y + 1, z, w).u
          + getCell(lattice_curr, x, y - 1, z, w).u
          + getCell(lattice_curr, x, y, z + 1, w).u
          + getCell(lattice_curr, x, y, z - 1, w).u;

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
        int helixR = (int)RMAX;
        unsigned int phase_full = 2u * (unsigned int)helixR * (unsigned int)helixR;
        unsigned int w_offset = (unsigned int)(((uint64_t)w * (uint64_t)phase_full) / (uint64_t)Wi);
        unsigned int cell_phase = w_offset % phase_full;
        int m = (int)(cell_phase / (unsigned int)helixR);
        int cos_w, sin_w;
        if (m < helixR)
        {
            int arg = m * (helixR - m);
            int s = isqrt(arg);
            cos_w = helixR - 2 * m;
            sin_w = 2 * s;
        }
        else
        {
            int m2 = m - helixR;
            int arg = m2 * (helixR - m2);
            int s = isqrt(arg);
            cos_w = 2 * m - 3 * helixR;
            sin_w = -2 * s;
        }

        int ru = (u_new * cos_w - v_new * sin_w) / helixR;
        int rv = (u_new * sin_w + v_new * cos_w) / helixR;

        d.u      = u_new;
        d.v      = v_new;
        d.active = active ? 1u : 0u;
        d.phiB   = active;
        d.pB     = (ru > 0);
        d.sB     = (rv > 0);

        // Sieve trigger: probability proportional to positive wave amplitude.
        bool s2B_trigger = false;
        if (u_new > 0)
        {
            int64_t prod = (int64_t)u_new * (int64_t)(pulse_tick + 1);
            int64_t mod = prod % (int64_t)SHELL_TARGET;
            if (mod < (int64_t)u_new) s2B_trigger = true;
        }
        d.s2B    = active && s2B_trigger;
    }

    // Copy the new wave state back to lattice_curr for the interaction FSM.
    for (size_t i = 0; i < BLOCK; ++i)
    {
        lattice_curr[i].u      = lattice_draft[i].u;
        lattice_curr[i].v      = lattice_draft[i].v;
        lattice_curr[i].active = lattice_draft[i].active;
        lattice_curr[i].phiB   = lattice_draft[i].phiB;
        lattice_curr[i].pB     = lattice_draft[i].pB;
        lattice_curr[i].sB     = lattice_draft[i].sB;
        lattice_curr[i].s2B    = lattice_draft[i].s2B;
    }
  }

  // ============================================================
  // CPU UPDATE — BFS + interaction FSM
  // ============================================================

  // Apply per-source momentum stored in the source-center cell and clear it.
  static void applyMomentum()
  {
    for (unsigned w = 0; w < W_USED; ++w)
    {
      unsigned cx = lcenters[w][0];
      unsigned cy = lcenters[w][1];
      unsigned cz = lcenters[w][2];
      Cell& c = getCell(lattice_draft, cx, cy, cz, w);

      int dx = c.m[0];
      int dy = c.m[1];
      int dz = c.m[2];

      if (dx || dy || dz)
      {
        int M = (int)EL;
        int nx = ((int)cx + dx) % M;
        int ny = ((int)cy + dy) % M;
        int nz = ((int)cz + dz) % M;
        if (nx < 0) nx += M;
        if (ny < 0) ny += M;
        if (nz < 0) nz += M;

        // Move the source center.  The momentum vector m is immutable, so it
        // is copied to the new source-center cell; the old cell is cleared.
        Cell& newCenter = getCell(lattice_draft, (unsigned)nx, (unsigned)ny, (unsigned)nz, w);
        newCenter.m[0] = dx;
        newCenter.m[1] = dy;
        newCenter.m[2] = dz;

        lcenters[w][0] = (unsigned)nx;
        lcenters[w][1] = (unsigned)ny;
        lcenters[w][2] = (unsigned)nz;

        c.m[0] = c.m[1] = c.m[2] = 0;
      }
    }
  }

  void update_lattice_cpu()
  {
    // Phase 1: BFS propagation of r2 (replaces ad-hoc d initialization)
    update_pulsating_wavefront();
    pulse_tick++;

    // Phase 2: radial polarisation pair (u,v) and active wavefront flag
    phase_step();

    // The phase output is in lattice_draft.  Promote it to the live state so
    // the FSM can read it and write its own modifications back to lattice_draft.
    std::swap(lattice_curr, lattice_draft);

    // DEBUG: throttle a snapshot of the central cell so we can verify (u,v) are evolving.
    if (pulse_tick % 100 == 0) {
        const Cell& c = getCell(lattice_curr, CENTER, CENTER, CENTER, 0);
        printf("DEBUG phase tick %u: center u=%d v=%d active=%u pB=%d sB=%d\n",
               pulse_tick, c.u, c.v, c.active, c.pB ? 1 : 0, c.sB ? 1 : 0);
    }

    // Phase 3: FSM interaction loop (uses r2 instead of d)
    for (unsigned w = 0; w < W_USED; ++w)
    {
        if (w == 0)
        {
            const Cell& first = lattice_curr.front();
            if (gConfig.delays.convol && first.k < CONVOL)
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
            else if (diffuse_delay && first.k >= CONVOL && first.k < DIFFUSION)
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            else if (reloc_delay && first.k >= DIFFUSION && first.k < RELOC)
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }

        for (unsigned x = 0; x < EL; ++x)
        for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
        {
            Cell &curr   = getCell(lattice_curr, x, y, z, w);
            Cell &draft  = getCell(lattice_draft, x, y, z, w);
            Cell &mirror = getCell(lattice_mirror, x, y, z, w);

            draft = curr;

            // Ensure correct coordinates
            curr.x[0] = x;
            curr.x[1] = y;
            curr.x[2] = z;
            curr.x[3] = w;

            Cell &forward = curr.getNeighbor(FORWARD);
            Cell &north   = curr.getNeighbor(NORTH);
            Cell &west    = curr.getNeighbor(WEST);
            Cell &down    = curr.getNeighbor(DOWN);
            Cell &south   = curr.getNeighbor(SOUTH);
            Cell &east    = curr.getNeighbor(EAST);
            Cell &up      = curr.getNeighbor(UP);

            if (curr.k < CONVOL) {
                convolute(curr, draft, mirror);
            } else if (curr.k < GSLOT_Z) {
                // glider slots
            } else if (curr.k < DIFFUSION) {
                diffuse(curr, draft, forward, north, west, down, south, east, up);
            } else if (curr.k < RELOC) {
                relocate(curr, draft, north, west, down);
            } else if (curr.k < REISSUE) {
                reissue(curr, draft, forward, north, west, down, south, east, up);
            } else if (curr.k < FLOOD) {
                flood(curr, draft, forward, north, west, down, south, east, up);
            }

            draft.k = (curr.k + 1) % FRAME;

            if (draft.k == 0) {
                if (curr.a == W_USED && curr.t <= RMAX)
                    draft.t++;
                else
                    draft.t = (curr.t + 1) % (2 * RMAX);
            }
        }
    }

    // Apply source-center momentum and update pulsation centers.
    applyMomentum();
  }

  void update_lattice()
  {
#ifdef USE_CUDA
    if (isCudaEnabled()) {
        cudaSimulationStepWrapper();
        return;
    }
#endif
    update_lattice_cpu();
  }

  bool swap_lattices_cpu()
  {
    if (BLOCK == 0 || lattice_curr.empty())
        return false;

    bool newLightFrame = false;

    std::copy(
        lattice_draft.begin(),
        lattice_draft.begin() + BLOCK,
        lattice_curr.begin());

    Cell &repr = getCell(lattice_curr, 0, 0, 0, 0);

    if (repr.k == 0)
    {
      for (unsigned w = 0; w < W_USED; ++w)
      for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
      for (unsigned z = 0; z < EL; ++z)
      {
          Cell &curr = getCell(lattice_curr, x, y, z, w);
          Cell &mirror = getCell(lattice_mirror, x, y, z, w);
          mirror = curr;
          mirror.f = mirror.t;
      }
      newLightFrame = true;
    }

    if (repr.k < CONVOL)
        shiftMirror();

    return newLightFrame;
  }

  bool swap_lattices()
  {
#ifdef USE_CUDA
    if (isCudaEnabled()) {
        Cell &repr = getCell(lattice_curr, 0, 0, 0, 0);
        return (repr.k == 0);
    }
#endif
    return swap_lattices_cpu();
  }

  bool simulation()
  {
    update_lattice();
    return swap_lattices();
  }

  // ============================================================
  // Neighbor accessor
  // ============================================================

  Cell &Cell::getNeighbor(int i)
  {
    static int disp[8][4] =
    {
        {+1,0,0,0},{-1,0,0,0},
        {0,+1,0,0},{0,-1,0,0},
        {0,0,+1,0},{0,0,-1,0},
        {0,0,0,+1},{0,0,0,-1}
    };

    int nx = x[0] + disp[i][0];
    int ny = x[1] + disp[i][1];
    int nz = x[2] + disp[i][2];
    int nw = x[3] + disp[i][3];

    spherical_wrap(nx, ny, nz, nw);

    return getCell(lattice_curr, nx, ny, nz, nw);
  }
} // namespace automaton