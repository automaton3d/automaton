/*
 * convolutes.cpp
 *
 * Convolution rules for each scenario (Sect. 5.4 of the manuscript).
 *
 * During the CONVOL phase (k < CONVOL), the CA compares each cell against
 * its mirror counterpart (cyclically shifted in the W dimension) to detect
 * wavefront crossings and trigger interactions.
 *
 * The scenarios form a progressive test suite: each one isolates a single
 * mechanism of the CA so it can be validated independently before enabling
 * the full rule set.
 *
 * Common gate condition (scenarios 1-5):
 *   curr.active                  — cell is on the active wavefront
 *   pulse_from_time(t) == (RMAX/2)²  — wavefront is at mid-radius
 *   x[3] == 0                 — only layer 0 (w = 0)
 *   ctrl                      — fire-once flag (prevents re-triggering)
 */

#include "model/simulation.h"
#include <algorithm>

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;

  extern std::vector<Cell> lattice_curr;
  extern std::vector<Cell> lattice_draft;
  extern std::vector<std::array<unsigned, 3>> lcenters;

  extern bool ctrl;

  namespace
  {
    inline const std::array<unsigned, 3>& sourceCenter(const Cell& c)
    {
      return lcenters[c.x[3]];
    }

    inline Cell& sourceCenterDraft(const Cell& c)
    {
      const auto& p = sourceCenter(c);
      return getCell(lattice_draft, p[0], p[1], p[2], c.x[3]);
    }

    inline Cell& sourceCenterCurr(const Cell& c)
    {
      const auto& p = sourceCenter(c);
      return getCell(lattice_curr, p[0], p[1], p[2], c.x[3]);
    }

    inline int shortestDelta(int a, int b, int mod)
    {
      int d = b - a;
      if (mod > 0)
      {
        int half = mod / 2;
        if (d > half) d -= mod;
        else if (d < -half) d += mod;
      }
      return d;
    }

    inline unsigned wrapCoord(int v)
    {
      int m = (int)EL;
      int r = v % m;
      if (r < 0) r += m;
      return (unsigned)r;
    }

    inline int sign(int v)
    {
      return (v > 0) - (v < 0);
    }

    // Move a source center by (dx, dy, dz) light-steps and reemit phase 0 there.
    void reemitSourceAt(Cell& srcDraft, int dx, int dy, int dz)
    {
      unsigned oldCx = srcDraft.x[0];
      unsigned oldCy = srcDraft.x[1];
      unsigned oldCz = srcDraft.x[2];
      unsigned w = srcDraft.x[3];

      unsigned newCx = wrapCoord((int)oldCx + dx);
      unsigned newCy = wrapCoord((int)oldCy + dy);
      unsigned newCz = wrapCoord((int)oldCz + dz);

      // Source state is stored in the source-center cell.  Place the reemitted
      // source at the new center so the next BFS wavefront starts from there.
      Cell& newDraft = getCell(lattice_draft, newCx, newCy, newCz, w);
      newDraft.kind        = srcDraft.kind;
      newDraft.parent      = srcDraft.parent;
      newDraft.spin_target = srcDraft.spin_target;
      newDraft.pair_idx    = srcDraft.pair_idx;
      newDraft.t           = 0;
      newDraft.f           = 0;

      // Record the displacement on the old center cell; applyMomentum() will
      // update lcenters[w] and then clear it.
      srcDraft.m[0] = dx;
      srcDraft.m[1] = dy;
      srcDraft.m[2] = dz;
    }

    // Move one light-step along the direction from 'from' to 'to'.
    void moveOneStep(Cell& srcDraft, const std::array<unsigned, 3>& from,
                     const std::array<unsigned, 3>& to)
    {
      int M = (int)EL;
      int dx = shortestDelta((int)from[0], (int)to[0], M);
      int dy = shortestDelta((int)from[1], (int)to[1], M);
      int dz = shortestDelta((int)from[2], (int)to[2], M);
      reemitSourceAt(srcDraft, sign(dx), sign(dy), sign(dz));
    }

    // Move one light-step away from the other source center.
    void moveOneStepAway(Cell& srcDraft, const std::array<unsigned, 3>& selfCenter,
                         const std::array<unsigned, 3>& otherCenter)
    {
      int M = (int)EL;
      int dx = shortestDelta((int)otherCenter[0], (int)selfCenter[0], M);
      int dy = shortestDelta((int)otherCenter[1], (int)selfCenter[1], M);
      int dz = shortestDelta((int)otherCenter[2], (int)selfCenter[2], M);
      reemitSourceAt(srcDraft, sign(dx), sign(dy), sign(dz));
    }

    // Reemit at the contact voxel (curr position) without changing kind.
    void reemitAtContact(Cell& srcDraft, const Cell& contact)
    {
      int dx = shortestDelta((int)srcDraft.x[0], (int)contact.x[0], (int)EL);
      int dy = shortestDelta((int)srcDraft.x[1], (int)contact.x[1], (int)EL);
      int dz = shortestDelta((int)srcDraft.x[2], (int)contact.x[2], (int)EL);
      reemitSourceAt(srcDraft, dx, dy, dz);
    }
  }

  /*
   * Scenario 0 — Wavefront propagation test.
   * No interaction: the wavefront expands and contracts freely under
   * antipodal (spherical) wrapping. Validates the basic CA timing,
   * effective_t oscillation, and wrapping topology.
   */
  bool convolute0(Cell& curr, Cell &draft, Cell &mirror)
  {
    return false;
  }

  /*
   * Scenario 1 — Relocation test.
   * At mid-radius on layer 0, assigns a random relocation vector c[].
   * During SLOT III (diffusion), c[] propagates to all cells on the
   * wavefront. During SLOTS VI-VIII (relocation), each cell physically
   * shifts by c[], displacing the entire bubble to a new position.
   * After reissue, t resets and expansion restarts from the new center.
   */
  bool convolute1(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.c[0] = getRandomUnsigned(EL);
      draft.c[1] = getRandomUnsigned(EL);
      draft.c[2] = getRandomUnsigned(EL);
      ctrl = false;
    }
    return false;
  }

  /*
   * Scenario 2 — Orphan mechanism test.
   * Sets affinity a = W_USED on a wavefront cell, marking it as an orphan.
   * Orphans represent the residual field of a bubble after reissue: they
   * continue expanding outward (t++ without modulo) but carry no momentum
   * and are dominated when they meet an active wavefront.
   * Validates SLOT I/II orphan propagation in diffusion.
   */
  bool convolute2(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      ctrl = false;
    }
    return false;
  }

  /*
   * Scenario 3 — Contraction test.
   * Sets orphan (a = W_USED) and contraction flag (cB = true).
   * cB propagates inward during SLOT III (diffusion, toward r2 = 0).
   * When cB reaches the center (r2 < 4) during reissue, t resets to 0,
   * restarting the wavefront expansion. This is the mechanism by which
   * a bubble "collapses" and re-emits from its center.
   */
  bool convolute3(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      draft.cB = true;
      ctrl = false;
    }
    return false;
  }

  /*
   * Scenario 4 — Hunting test.
   * Sets the hunt flag (hB) on a wavefront cell that has sB (spiral bit).
   * During SLOT II (diffusion), hB propagates outward along the wavefront,
   * seeking cells with sB set. This is how a bubble locates its interaction
   * partner — the "hunter" propagates until it finds the "prey" (sB cell).
   */
  bool convolute4(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && effective_t(curr.t) == RMAX / 2 && curr.sB && curr.x[3] == 0 && ctrl)
    {
      draft.hB = true;
      ctrl = false;
    }
    return false;
  }

  /*
   * Scenario 5 — Reissue (affinity propagation) test.
   * Triggers a full reissue cycle on a pB (propeller) cell: sets c[] to
   * the cell's own position, cB = true, and a = W_USED (orphan).
   * During diffusion, c[] and cB propagate inward; during relocation,
   * the bubble physically moves to the target position; during reissue,
   * t resets and the bubble re-expands from the relocated center.
   * This tests the complete relocate-and-reissue pipeline.
   */
  bool convolute5(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && effective_t(curr.t) == RMAX / 2 && curr.pB && curr.x[3] == 0 &&
       !curr.cB && curr.a != W_USED && ctrl)
    {
      draft.c[0] = curr.x[0];
      draft.c[1] = curr.x[1];
      draft.c[2] = curr.x[2];
      draft.cB = true;
      draft.a = W_USED;
      ctrl = false;
    }
    return false;
  }

  /*
   * Scenario 6 — Dispersion (Orbis/Umbra separation) test.
   * Detects inter-sector interaction: curr and mirror have different W1
   * (weak isospin), meaning they belong to different sectors (Orbis vs Umbra).
   *
   * Two outcomes at the same spatial position (superposing wavefronts):
   *   pB && mirror.sB  →  reissue: the propeller cell relocates to its own
   *                        position (c[] = x[]), becomes orphan, and re-emits.
   *   sB && !mirror.pB →  hunting + contraction: the spiral cell hunts for
   *                        a new partner while contracting.
   *
   * This models how bubbles from different sectors interact without
   * annihilating — they separate (disperse) rather than merge.
   */
  bool convolute6(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.active && mirror.active)
    {
      if (curr.x[0] == mirror.x[0] &&
          curr.x[1] == mirror.x[1] &&
          curr.x[2] == mirror.x[2])
      {
        if (curr.a != W_USED &&
            curr.W1() != mirror.W1() &&
            !curr.cB &&
            effective_t(curr.t) == RMAX / 2)
        {
          if (curr.pB && mirror.sB)
          {
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.cB = true;
            draft.a = W_USED;
          }
          else if (curr.sB && !mirror.pB)
          {
            draft.hB = true;
            draft.cB = true;
            draft.a = W_USED;
          }
        }
      }
    }
    return false;
  }

  /*
   * Scenario 7 — Full convolution with K/S/D/P source interactions.
   *
   * curr and mirror must both be on their active wavefronts.  The electric
   * channel is triggered by pB, the magnetic channel by sB:
   *   - both pB true  → electric collapse (kB=1)
   *   - both sB true  → magnetic collapse (kB=1)
   *   - only one pB or one sB true → adiabatic exchange of affinity and phase
   * The source-center cells are then updated according to K/S/D/P rules.
   */
  bool convolute7(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (!curr.active || !mirror.active)
      return false;

    // A source does not interact with itself (same W-island).
    if (curr.x[3] == mirror.x[3])
      return false;

    // Sieve propagation: s2B' = s2B AND active.
    draft.s2B = draft.s2B && (curr.active != 0);

    // Source state is stored in the source-center cell of each W-layer.
    Cell& currSrc  = sourceCenterCurr(curr);
    Cell& mirrorSrc = sourceCenterCurr(mirror);
    Cell& currDraft  = sourceCenterDraft(curr);
    Cell& mirrorDraft = sourceCenterDraft(mirror);

    const auto& currCenter  = sourceCenter(curr);
    const auto& mirrorCenter = sourceCenter(mirror);

    bool samePos = (curr.x[0] == mirror.x[0] &&
                    curr.x[1] == mirror.x[1] &&
                    curr.x[2] == mirror.x[2]);

    // pB triggers the electric channel, sB the magnetic channel.
    bool electricContact   = curr.pB || mirror.pB;
    bool magneticContact   = curr.sB || mirror.sB;
    bool electricCollapse  = curr.pB && mirror.pB;
    bool magneticCollapse  = curr.sB && mirror.sB;
    bool collapse          = electricCollapse || magneticCollapse;

    if (!electricContact && !magneticContact)
      return false;

    if (collapse)
    {
      draft.kB = true;
      draft.cB = true;
    }
    else
    {
      // Adiabatic: no collapse, but the two sources exchange
      // affinity (a) and light clock (t) and drift one step toward each other.
      std::swap(currDraft.a, mirrorDraft.a);
      std::swap(currDraft.t, mirrorDraft.t);
      moveOneStep(currDraft, currCenter, mirrorCenter);
      moveOneStep(mirrorDraft, mirrorCenter, currCenter);
      return false;
    }

    // 1. K x K
    if (currSrc.kind == SourceKind::K && mirrorSrc.kind == SourceKind::K)
    {
      moveOneStepAway(currDraft, currCenter, mirrorCenter);
      return false;
    }

    // 2. K x S (current = K, mirror = S): K does not move; the S becomes a
    //    D delegate of K when it is its turn to be the current cell.
    //    The S-side is handled below.

    // 3. S x K (current = S, mirror = K)
    if (currSrc.kind == SourceKind::S && mirrorSrc.kind == SourceKind::K)
    {
      currDraft.kind = SourceKind::D;
      currDraft.parent = mirrorSrc.x[3];
      // S vector direction relative to K is approximated as outward for now.
      currDraft.spin_target = 1;
      moveOneStep(currDraft, currCenter, mirrorCenter);
      return false;
    }

    // 4. S x S
    if (currSrc.kind == SourceKind::S && mirrorSrc.kind == SourceKind::S)
    {
      if (currSrc.Q() == mirrorSrc.Q())
      {
        // Same field sign: repel one light-step.
        moveOneStepAway(currDraft, currCenter, mirrorCenter);
      }
      else
      {
        // Opposite field sign: both become mutual D delegates.
        currDraft.kind = SourceKind::D;
        currDraft.parent = mirrorSrc.x[3];
      }
      return false;
    }

    // 5. S x D / D x S
    if ((currSrc.kind == SourceKind::S && mirrorSrc.kind == SourceKind::D) ||
        (currSrc.kind == SourceKind::D && mirrorSrc.kind == SourceKind::S))
    {
      Cell& sSrcDraft  = (currSrc.kind == SourceKind::S ? currDraft : mirrorDraft);
      Cell& dSrc       = (currSrc.kind == SourceKind::S ? mirrorSrc : currSrc);
      Cell& dSrcDraft  = (currSrc.kind == SourceKind::S ? mirrorDraft : currDraft);
      const auto& sCenter = (currSrc.kind == SourceKind::S ? currCenter : mirrorCenter);
      const auto& dCenter = (currSrc.kind == SourceKind::S ? mirrorCenter : currCenter);

      // S becomes a D delegate of the K parent of the D.
      sSrcDraft.kind = SourceKind::D;
      sSrcDraft.parent = dSrc.parent;

      // Both reemit and move one light-step toward each other.
      moveOneStep(sSrcDraft, sCenter, dCenter);
      moveOneStep(dSrcDraft, dCenter, sCenter);
      return false;
    }

    // 6. D x D
    if (currSrc.kind == SourceKind::D && mirrorSrc.kind == SourceKind::D)
    {
      if (currSrc.parent != mirrorSrc.parent)
      {
        // Different tribes: reemit, repel one light-step, exchange momentum.
        moveOneStepAway(currDraft, currCenter, mirrorCenter);
        // Simplified momentum exchange: copy the mirror's stored momentum.
        currDraft.m[0] = mirrorSrc.m[0];
        currDraft.m[1] = mirrorSrc.m[1];
        currDraft.m[2] = mirrorSrc.m[2];
      }
      else
      {
        // Same tribe: outer delegate imposes spin_target on inner one.
        // (Tangential/radial orbital motion is left as a refinement.)
        currDraft.spin_target = mirrorSrc.spin_target;
      }
      return false;
    }

    // 7. P x K / P x D / P x S (current = P, mirror = ordinary source)
    if (currSrc.kind == SourceKind::P &&
        (mirrorSrc.kind == SourceKind::K ||
         mirrorSrc.kind == SourceKind::S ||
         mirrorSrc.kind == SourceKind::D))
    {
      // P pair reemits at the contact point with phase 0.
      reemitAtContact(currDraft, curr);

      // Target receives a momentum impulse in the direction of P's momentum.
      mirrorDraft.m[0] = currSrc.m[0];
      mirrorDraft.m[1] = currSrc.m[1];
      mirrorDraft.m[2] = currSrc.m[2];
      return false;
    }

    // 8. K/D/S x P (current = ordinary source, mirror = P)
    if ((currSrc.kind == SourceKind::K ||
         currSrc.kind == SourceKind::S ||
         currSrc.kind == SourceKind::D) &&
        mirrorSrc.kind == SourceKind::P)
    {
      // Target reemits on its own surface at the contact point and gets P's momentum.
      reemitAtContact(currDraft, curr);
      currDraft.m[0] = mirrorSrc.m[0];
      currDraft.m[1] = mirrorSrc.m[1];
      currDraft.m[2] = mirrorSrc.m[2];
      return false;
    }

    // P x P is suppressed.
    return false;
  }

}
