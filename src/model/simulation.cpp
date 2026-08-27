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
#include "model/polarization.h"
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
  unsigned ISLAND_SIZE = 0;
  unsigned ISLAND_COUNT = 0;

  // Lattices
  std::vector<Cell> lattice_curr;
  std::vector<Cell> lattice_draft;
  std::vector<Cell> lattice_mirror;

  string lastAllocationError;
  std::vector<std::array<unsigned, 3>> lcenters;

  // ============================================================
  // TOROIDAL (TRANSLATION) NEIGHBOUR ADDRESSING
  // Manuscript Sect. "Boundary behavior": the lattice is a 3-torus.
  // A step across a face of the cube continues from the opposite face as a
  // pure translation -- orientation preserved, nothing mirrored.
  // ============================================================

  inline void spherical_wrap(int& x, int& y, int& z, int& w)
  {
    x = ((x % (int)EL) + (int)EL) % (int)EL;
    y = ((y % (int)EL) + (int)EL) % (int)EL;
    z = ((z % (int)EL) + (int)EL) % (int)EL;

    // The w slot keeps its historical edge pairing (self-loop at the first/
    // last layer): cross-layer adjacency is owned by shiftMirror()'s
    // rotation schedule, not by spatial geometry.
    if (w < 0) w = 0;
    if (w >= (int)W_USED) w = (int)W_USED - 1;
  }

  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w)
  {
    lcenters[w][0] = x;
    lcenters[w][1] = y;
    lcenters[w][2] = z;
  }

  // ============================================================
  // DISTANCE-FIELD UPDATE — CaRaSh-style incremental r²/r update
  // ============================================================
  // Maintains the integer radius r and squared radius r² from each layer's
  // moving source center using only additions and square-boundary tests.
  // The active wavefront is scheduled in phase_step by comparing r with
  // effective_t(t), so no sqrt/isqrt is needed here.
  // ============================================================

  void update_pulsating_wavefront()
  {
    // Copy current r2 and integer radius r into draft
    for (size_t i = 0; i < BLOCK; ++i)
    {
        lattice_draft[i].r2 = lattice_curr[i].r2;
        lattice_draft[i].r  = lattice_curr[i].r;
    }

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

        // Toroidal axis offsets to the source centre (manuscript Sect.
        // "Boundary behavior"): shortest wrapped distance per axis, so the
        // squared-distance relaxation measures geodesics on the 3-torus.
        int dx_ = (int)x - cx;
        int dy_ = (int)y - cy;
        int dz_ = (int)z - cz;
        int ax = dx_ < 0 ? -dx_ : dx_;
        int ay = dy_ < 0 ? -dy_ : dy_;
        int az = dz_ < 0 ? -dz_ : dz_;
        if (ax > (int)EL - ax) ax = (int)EL - ax;
        if (ay > (int)EL - ay) ay = (int)EL - ay;
        if (az > (int)EL - az) az = (int)EL - az;

        // 6-connected spatial neighbors (no w propagation)
        static const int offsets[6][3] = {
            {+1,0,0}, {-1,0,0},
            {0,+1,0}, {0,-1,0},
            {0,0,+1}, {0,0,-1}
        };

        for (int dir = 0; dir < 6; ++dir)
        {
            // Periodic address on the 3-torus: a step across a face of the
            // cube re-enters through the opposite face (no flux is lost).
            int nx = ((int)x + offsets[dir][0] + (int)EL) % (int)EL;
            int ny = ((int)y + offsets[dir][1] + (int)EL) % (int)EL;
            int nz = ((int)z + offsets[dir][2] + (int)EL) % (int)EL;

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
            {
                nxt.r2 = new_r2;

                // Propagate the integer radius without isqrt.
                // Moving one 6-neighbor step changes the true radius by 0 or 1,
                // so the new radius is either the parent's r or r+1.
                int child_r = (curr.r < 0) ? 0 : curr.r;
                unsigned int next_sq = (unsigned int)(child_r + 1) * (unsigned int)(child_r + 1);
                if (new_r2 >= next_sq)
                    child_r++;

                // Tiny correction for the parent's r being one unit stale
                // (source centers move at most one cell per light frame).
                while (child_r > 0 && (unsigned int)new_r2 < (unsigned int)child_r * (unsigned int)child_r)
                    child_r--;
                while ((unsigned int)(child_r + 1) * (unsigned int)(child_r + 1) <= (unsigned int)new_r2)
                    child_r++;

                nxt.r = child_r;
            }
        }
    }
    }

    // Ensure each source center stays at 0
    for (unsigned w = 0; w < W_USED; ++w)
    {
        int cx = (int)lcenters[w][0];
        int cy = (int)lcenters[w][1];
        int cz = (int)lcenters[w][2];
        Cell &src = getCell(lattice_draft, (unsigned)cx, (unsigned)cy, (unsigned)cz, w);
        src.r2 = 0;
        src.r  = 0;
    }

    // Copy r2 and r back to curr; no isqrt needed.
    for (size_t i = 0; i < BLOCK; ++i)
    {
        lattice_curr[i].r2 = lattice_draft[i].r2;
        lattice_curr[i].r  = lattice_draft[i].r;
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

        // Active wavefront: shell of integer radius pulseR moving at one cell per
        // light frame.  c.r is propagated/corrected by square-boundary tests in
        // update_pulsating_wavefront, so no sqrt or isqrt is needed here.
        int pulseR = (int)effective_t(c.t);
        bool active = (c.r2 != INF_R2 && c.r >= 0 && c.r == pulseR);

        // Hard zero outside the processed sphere (radial dead zone).  On the
        // 3-torus faces are not special: the spherical cavity is enforced
        // radially, and the source-centre cell (r2 == 0) stays exempt.
        if (c.r2 != 0 && (c.r < 0 || c.r >= R))
        {
            d.u = 0;
            d.v = 0;
            d.active = active ? 1u : 0u;
            d.phiB = active;
            d.pol_u = 0;
            d.pol_v = 0;
            d.pB = false;
            d.sB = false;
            d.s2B = false;
            continue;
        }

        int u = c.u;
        int v = c.v;
        int r = c.r;

        // NOTE: neighbour reads are made in-bounds by periodic wrapping
        // (3-torus, manuscript Sect. "Boundary behavior").  A wrapped image
        // contributes flux exactly like the interior, so the wave exchanges
        // no spurious amplitude across seams; only the explicit radial
        // sponge, damping and shell-source terms below change the totals.
        auto uAt = [&](int xx, int yy, int zz) -> int
        {
            xx = ((xx % ELi) + ELi) % ELi;
            yy = ((yy % ELi) + ELi) % ELi;
            zz = ((zz % ELi) + ELi) % ELi;
            return getCell(lattice_curr, xx, yy, zz, w).u;
        };

        int neighbors_u =
              uAt(x + 1, y, z)
            + uAt(x - 1, y, z)
            + uAt(x, y + 1, z)
            + uAt(x, y - 1, z)
            + uAt(x, y, z + 1)
            + uAt(x, y, z - 1);

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

        // Emergent transverse polarisation (manuscript, Sect. "Emergent
        // polarization pair"): the pair (pol_u, pol_v) is NOT a geometric
        // function of the local radius — it is reconstructed from the
        // broadcasted arrival stamp b(x) of the elected momentum vector,
        // approximating pol_u^2 + pol_v^2 = R^4 via isqrt (R = L/2 - 2 emergent).
        // No trigonometric tables, no precomputed spiral and no per-cell
        // fixed constants: the direction comes from the payload tournament
        // over the W-ledger and the phase from the isqrt relation applied
        // to the arrival time.
        int pol_u = 0, pol_v = 0;
        polarization::reconstructPair(c.bstamp, (int)RMAX - 2, pol_u, pol_v);
        d.pol_u = pol_u;
        d.pol_v = pol_v;

        d.u      = u_new;
        d.v      = v_new;
        d.active = active ? 1u : 0u;
        d.phiB   = active;
        d.pB     = (pol_u > 0);
        d.sB     = (pol_v > 0);

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
        lattice_curr[i].pol_u  = lattice_draft[i].pol_u;
        lattice_curr[i].pol_v  = lattice_draft[i].pol_v;
        lattice_curr[i].pB     = lattice_draft[i].pB;
        lattice_curr[i].sB     = lattice_draft[i].sB;
        lattice_curr[i].s2B    = lattice_draft[i].s2B;
    }
  }

  // ============================================================
  // CPU UPDATE — BFS + interaction FSM
  // ============================================================

  // Apply the consumable relocation/impulse stored in the source-center cell.
  // The long-term momentum-direction vector m is preserved; only reloc is
  // consumed when a non-zero displacement is pending.
  static int wrapCoord(int v)
  {
    int M = (int)EL;
    int r = v % M;
    if (r < 0) r += M;
    return r;
  }

  static void applyMomentum()
  {
    for (unsigned w = 0; w < W_USED; ++w)
    {
      unsigned cx = lcenters[w][0];
      unsigned cy = lcenters[w][1];
      unsigned cz = lcenters[w][2];
      Cell& old = getCell(lattice_draft, cx, cy, cz, w);

      // Free photon pairs expand and are gradually consumed. At maximum
      // radius (t == RMAX) one pair is consumed; when the stack empties the
      // two partner source centers are released as singletons moving apart.
      if (old.kind == SourceKind::P &&
          old.a == W_USED &&
          old.pair_idx != NO_PAIR &&
          old.t == (unsigned)RMAX &&
          old.w < old.pair_idx)
      {
          if (old.pair_count > 0)
              old.pair_count--;

          WIndex pw = old.pair_idx;
          Cell& partner = getCell(lattice_draft,
                                  (unsigned)lcenters[pw][0],
                                  (unsigned)lcenters[pw][1],
                                  (unsigned)lcenters[pw][2],
                                  pw);

          if (old.pair_count == 0)
          {
              // Last pair consumed: release two singletons.
              old.kind       = SourceKind::S;
              old.pair_idx   = NO_PAIR;
              old.pair_count = 0;
              old.leader_w   = NO_LEADER_W;
              old.a          = W_USED;

              partner.kind       = SourceKind::S;
              partner.pair_idx   = NO_PAIR;
              partner.pair_count = 0;
              partner.leader_w   = NO_LEADER_W;
              partner.a          = W_USED;

              int axis = (int)(old.w % 3u);
              int sign = ((old.w & 1u) ? +1 : -1);
              old.reloc[axis]     += sign;
              partner.reloc[axis] -= sign;
          }
          else
          {
              // The stack still holds pairs; keep the remaining count in sync.
              partner.pair_count = old.pair_count;
          }
      }

      int dx = old.reloc[0];
      int dy = old.reloc[1];
      int dz = old.reloc[2];

      if (dx == 0 && dy == 0 && dz == 0)
        continue;

      int nx = wrapCoord((int)cx + dx);
      int ny = wrapCoord((int)cy + dy);
      int nz = wrapCoord((int)cz + dz);

      Cell& nw = getCell(lattice_draft, (unsigned)nx, (unsigned)ny, (unsigned)nz, w);

      // Update the long-term momentum direction from the consumed impulse.
      // m stays a unit vector along one Cartesian axis, but its sign can flip
      // to match the dominant component of the relocation impulse.
      int new_m[3] = { old.m[0], old.m[1], old.m[2] };
      if (dx != 0 || dy != 0 || dz != 0)
      {
        int abs_dx = (dx < 0) ? -dx : dx;
        int abs_dy = (dy < 0) ? -dy : dy;
        int abs_dz = (dz < 0) ? -dz : dz;
        int axis = 0, best = abs_dx;
        if (abs_dy > best) { axis = 1; best = abs_dy; }
        if (abs_dz > best) { axis = 2; best = abs_dz; }
        int sign = (axis == 0 ? dx : (axis == 1 ? dy : dz));
        new_m[0] = new_m[1] = new_m[2] = 0;
        new_m[axis] = (sign < 0) ? -1 : +1;
      }

      // Carry the source identity and the stable momentum direction m to the
      // new center, and re-seed the pulsating wave there.
      nw.kind        = old.kind;
      nw.parent      = old.parent;
      nw.spin_target = old.spin_target;
      nw.pair_idx    = old.pair_idx;
      nw.pair_count  = old.pair_count;
      nw.leader_w    = old.leader_w;
      nw.t           = 0;
      nw.f           = 0;
      nw.u           = 2048;
      nw.v           = 0;
      nw.m[0]        = new_m[0];
      nw.m[1]        = new_m[1];
      nw.m[2]        = new_m[2];
      nw.reloc[0]    = nw.reloc[1] = nw.reloc[2] = 0;

      // The old cell is no longer a source center.
      old.kind        = SourceKind::S;
      old.parent      = NO_PARENT;
      old.spin_target = 0;
      old.pair_idx    = NO_PAIR;
      old.pair_count  = 0;
      old.leader_w    = NO_LEADER_W;
      old.a           = W_USED;
      old.t           = 0;
      old.f           = 0;
      old.u           = 0;
      old.v           = 0;
      old.m[0]        = old.m[1] = old.m[2] = 0;
      old.reloc[0]    = old.reloc[1] = old.reloc[2] = 0;

      lcenters[w][0] = (unsigned)nx;
      lcenters[w][1] = (unsigned)ny;
      lcenters[w][2] = (unsigned)nz;
    }
  }

  // ============================================================
  // Fatia 2 — M/Mbar turnaround hook (flag-gated, default OFF).
  //
  // Implements the fsm.md §10 "future hook for charge inversion at
  // t == RMAX" as a controlled knob:
  //   simulation.mm_eps    C/CP-like bias of the conjugation rate
  //   simulation.mm_pbase  base probability per turnaround
  //   simulation.mm_seed   xorshift32 seed (deterministic runs)
  //
  // At each island turnaround (draft centre t == RMAX, edge-latched) the
  // bubble conjugates with probability
  //     p = mm_pbase * (1 + mm_eps * bias),   bias = +1 matter, -1 anti,
  // where matter/anti is read from the CURRENT charge word.  The
  // current-charge feedback is the only coupling that produced a
  // persistent excess in the virada.c ablation (birth-identity anchors
  // and sector mirrors wash out to noise).
  //
  // The conjugation is sector-preserving (toy "mode 0") and stays inside
  // the 32 valid charge words: ch ^= 0x1F flips color (c -> 7-c), q and
  // w0 while keeping w1 — so the q = w0 ^ w1 invariant still holds and
  // the word crosses the matter/anti threshold (popcount s -> 3-s).
  //
  // With mm_eps == 0 this routine is an exact no-op (bit-identical run).
  // ============================================================
  void applyChargeConjugation()
  {
    static bool inited = false;
    static double cfgEps = 0.0, cfgPbase = 1.0;
    static unsigned latchedSize = 0;
    static std::vector<unsigned char> latched;   // per-island edge latch
    static uint32_t rng = 1u;

    if (lattice_draft.empty() || BLOCK == 0 || W_USED == 0)
      return;

    if (!inited)
    {
      inited      = true;
      cfgEps      = gConfig.simulation.mm_eps;
      cfgPbase    = gConfig.simulation.mm_pbase;
      rng         = gConfig.simulation.mm_seed | 1u;
      latched.assign(W_USED, 0);
      latchedSize = W_USED;
    }
    if (cfgEps == 0.0 || latchedSize != W_USED)
      return;                    // hook disabled (default) / lattice resized

    unsigned events = 0, flips = 0;

    for (unsigned w = 0; w < W_USED; ++w)
    {
      const unsigned cx = lcenters[w][0];
      const unsigned cy = lcenters[w][1];
      const unsigned cz = lcenters[w][2];
      Cell& dc = getCell(lattice_draft, (int)cx, (int)cy, (int)cz, (int)w);

      if (dc.t != RMAX) { latched[w] = 0; continue; }
      if (latched[w])   continue;  // same turnaround already handled
      latched[w] = 1;

      if (dc.a == W_USED) continue;  // released singleton: no island left

      ++events;

      const bool isMatter =
          ((dc.ch & 1u) + ((dc.ch >> 1) & 1u) + ((dc.ch >> 2) & 1u)) < 2u;
      const double bias = isMatter ? 1.0 : -1.0;

      double p = cfgPbase * (1.0 + cfgEps * bias);
      if (p < 0.0) p = 0.0;
      if (p > 1.0) p = 1.0;

      rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
      const double u = (double)(rng >> 8) / 16777216.0;    // uniform [0,1)
      if (u >= p) continue;

      const unsigned char chOld = dc.ch;
      const unsigned char chNew = (unsigned char)(chOld ^ 0x1Fu);

      // Conjugate every attached cell of the island in all three lattices,
      // so the flip survives the swap and the boundary mirror stays coherent.
      for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
      for (unsigned z = 0; z < EL; ++z)
      {
        Cell& d = getCell(lattice_draft, (int)x, (int)y, (int)z, (int)w);
        if (d.a != W_USED) d.ch = chNew;
        Cell& c2 = getCell(lattice_curr, (int)x, (int)y, (int)z, (int)w);
        if (c2.a != W_USED) c2.ch = chNew;
        Cell& m2 = getCell(lattice_mirror, (int)x, (int)y, (int)z, (int)w);
        if (m2.a != W_USED) m2.ch = chNew;
      }

      ++flips;
      printf("[mm] tick=%u w=%u %s ch=0x%02X->0x%02X p=%.3f\n",
             pulse_tick, w, isMatter ? "M->A" : "A->M", chOld, chNew, p);
    }

    if (events > 0)
      printf("[mm] tick=%u turnarounds=%u flips=%u\n",
             pulse_tick, events, flips);
  }

  void update_lattice_cpu()
  {
    // Phase 1: CaRaSh-style incremental distance field (r2/r) from each
    // moving source center using only additions and square comparisons.
    update_pulsating_wavefront();
    pulse_tick++;

    // Fatia 1 instrumentation (read-only): virgin-wrap ledger + throttled
    // matter/antimatter census.  Never writes to the lattice.
    chargesSampleTurnarounds();
    chargesReport(pulse_tick);

    // Phase 2: radial polarisation pair (u,v) and active wavefront flag.
    // The active shell is c.r == effective_t(c.t): one cell per light frame.
    phase_step();

    // The phase output is in lattice_draft.  Promote it to the live state so
    // the FSM can read it and write its own modifications back to lattice_draft.
    std::swap(lattice_curr, lattice_draft);

    // Phase 2b: emergent polarization broadcast.  At every expansion limit
    // of the breathing wavefront a momentum direction is elected out of
    // the automaton's own W-ledger; a helical walker then stamps arrival
    // ticks b(x) around the elected axis, so phase_step() can reconstruct
    // the transverse pair on the NEXT tick.  Runs on the promoted live
    // lattice; the FSM below copies the stamps into draft/mirror.
    polarization::tick();

    // DEBUG: throttle a snapshot of the central cell so we can verify (u,v) are evolving.
    if (pulse_tick % 100 == 0) {
        const Cell& c = getCell(lattice_curr, CENTER, CENTER, CENTER, 0);
        printf("DEBUG phase tick %u: center u=%d v=%d active=%u pB=%d sB=%d pol=(%d,%d) bstamp=%u\n",
               pulse_tick, c.u, c.v, c.active, c.pB ? 1 : 0, c.sB ? 1 : 0,
               c.pol_u, c.pol_v, c.bstamp);
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

    // Fatia 2 — M/Mbar hook: conjugate islands at their breathing turnaround.
    // Runs BEFORE applyMomentum() so it sees the same draft t == RMAX state
    // the pair consumption uses.  Exact no-op when mm_eps == 0.
    applyChargeConjugation();

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