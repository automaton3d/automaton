#ifndef POLARIZATION_H_
#define POLARIZATION_H_

/*
 * polarization.h — Emergent polarization pair (u,v).
 *
 * Three-stage mechanism described in the manuscript section
 * "Emergent polarization pair (u,v)":
 *
 *   1) ELECTION — at every expansion limit of the breathing wavefront
 *      (the exact tick where the wave turns from ascending to descending,
 *      i.e. the source-centre counter t == RMAX), every active shell cell
 *      is seeded with the payload
 *          payload(x) = (score(x) << 24) | code(x),
 *          score(x)   = H((nu(x) + s) mod 9L, code(x)),
 *      where nu(x) is the cell's W-island index, code(x) its linear index,
 *      H a fixed integer hash and s a global seed dial.  Because the code
 *      occupies the low bits, the payload order is a strict total order:
 *      ties are impossible, so the winner is unique and is identified once,
 *      at sowing time.  The displacement from the lattice centre to the
 *      winning cell becomes the momentum vector m, rescaled to |m| = L/2.
 *
 *   2) BROADCAST — a helical walker traces the spiral arm around the
 *      cylinder whose axis is the elected direction (arbitrary-axis port
 *      of the spiral.c reference implementation): it advances one cell per
 *      tick keeping the radial error within about one cell using only
 *      additions, subtractions, shifts and comparisons among its six face
 *      neighbours.  Each newly computed spiral point stamps the current
 *      tick into Cell::bstamp of a second value lattice (0 = never
 *      reached).  Every other tick, with alternating sweep directions,
 *      every cell copies the greatest neighbour stamp whenever it exceeds
 *      its own, so the news diffuses to every cell in finite time.
 *
 *   3) RECONSTRUCTION — the arrival stamp is the phase source:
 *          phase_full = 2 R^2 (R = L/2 - 2 emergent bubble radius),
 *          cell_phase = (b(x)-1) mod phase_full,  j = floor(cell_phase / R),
 *          j < R : u = R(R - 2j),        v =  2R isqrt(j(R-j)),
 *          j >= R: u = R(2j - 3R),       v = -2R isqrt(j2(R-j2)), j2 = j-R.
 *      These formulas approximate the circle u^2 + v^2 = R^4 (exact only when isqrt is exact).
 */

#include "model/simulation.h"

namespace automaton
{
  namespace polarization
  {
    /// Release all per-layer walker state (call before re-allocation).
    void resetAll();

    /// One automaton tick of election / helical broadcast orchestration.
    /// Must be called once per update_lattice_cpu(), AFTER lattice_curr
    /// holds the promoted phase state (post std::swap).
    void tick();

    /// True while layer w's helical walk is still advancing.
    bool walkLive(unsigned w);

    /// Elected axis for layer w (|axis| = RMAX), or nullptr before the
    /// first election.  Host-side read-only view for GUI/debug.
    const int* electedAxis(unsigned w);

    /// Reconstruction stage: convert an arrival stamp into the transverse
    /// polarisation pair approximating the circle pol_u^2 + pol_v^2 = R^4.
    /// Stamps store (arrival tick + 1), so 0 unambiguously means "never
    /// reached" and cannot alias with a tick congruent to 0 mod 2R^2.
    inline void reconstructPair(unsigned int bstamp_, int R, int& pu, int& pv)
    {
      if (bstamp_ == 0u || R <= 0)
      {
        pu = 0;
        pv = 0;
        return;
      }
      unsigned int tick = bstamp_ - 1u;
      unsigned int phase_full = 2u * (unsigned int)R * (unsigned int)R;
      unsigned int cell_phase = tick % phase_full;
      int j = (int)(cell_phase / (unsigned int)R);
      int s;
      if (j < R)
      {
        pu = R * (R - 2 * j);
        s = isqrt(j * (R - j));
        pv = 2 * R * s;
      }
      else
      {
        int j2 = j - R;
        pu = R * (2 * j - 3 * R);
        s = isqrt(j2 * (R - j2));
        pv = -2 * R * s;
      }
    }
  }
}

#endif /* POLARIZATION_H_ */
