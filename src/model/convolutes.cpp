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

    // Pair-formation test from the six superposing-bubble rules.
    // A pair can form when two overlapping bubbles have complementary charges.
    bool canFormPair(const Cell& a, const Cell& b)
    {
      unsigned char ca = a.ch;
      unsigned char cb = b.ch;
      // Rule 3: both neutral (000000)
      if (ca == 0x00 && cb == 0x00) return true;
      // Rule 4: both anti-neutral (111111)
      if (ca == 0x3F && cb == 0x3F) return true;
      // Rule 1: every charge bit complementary
      if ((ca ^ cb) == 0x3F) return true;

      bool qa = (ca & 0x08) != 0;
      bool qb = (cb & 0x08) != 0;
      bool w1a = (ca & 0x20) != 0;
      bool w1b = (cb & 0x20) != 0;
      bool w0a = (ca & 0x10) != 0;
      bool w0b = (cb & 0x10) != 0;
      unsigned char cola = ca & 0x07;
      unsigned char colb = cb & 0x07;

      // Rule 2: q, w0 and color complementary; w1 identical
      if ((qa ^ qb) && (w1a == w1b) && (w0a ^ w0b) && ((cola ^ colb) == 0x07)) return true;
      // Rule 5: q=0, w1=0, w0=1, same non-neutral color
      if (!qa && !qb && !w1a && !w1b && w0a && w0b && cola == colb && cola != 0x00 && cola != 0x07) return true;
      // Rule 6: q=1, w1=1, w0=0, same non-neutral color
      if (qa && qb && w1a && w1b && !w0a && !w0b && cola == colb && cola != 0x00 && cola != 0x07) return true;
      return false;
    }

    // Add an impulse (dx, dy, dz) to the source-center cell and reemit phase 0.
    // The long-term momentum-direction vector m is preserved; the consumable
    // relocation vector reloc records the pending displacement.  The actual move
    // is performed after the FSM by applyMomentum().
    void reemitSourceAt(Cell& srcDraft, int dx, int dy, int dz)
    {
      srcDraft.reloc[0] += dx;
      srcDraft.reloc[1] += dy;
      srcDraft.reloc[2] += dz;
      srcDraft.t = 0;
      srcDraft.f = 0;
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
  // Convenience: make a cell adopt a leader identity.
  inline void adoptLeader(Cell& dst, WIndex leader)
  {
    dst.leader_w = leader;
    dst.a = (unsigned)leader;
  }

  // Choose the dominant leader between two sources; if neither has one,
  // fall back to the smaller intrinsic W address.
  inline WIndex dominantLeader(const Cell& a, const Cell& b)
  {
    WIndex leader = std::min(a.leader_w, b.leader_w);
    if (leader == NO_LEADER_W)
      leader = std::min(a.w, b.w);
    return leader;
  }

  bool convolute7(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (!curr.active || !mirror.active)
      return false;

    // A source does not interact with itself (same W-island).
    if (curr.x[3] == mirror.x[3])
      return false;

    // Sieve: the electroweak interaction channel is only active where s2B is set.
    if (!curr.s2B)
      return false;

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
    bool sameT   = (curr.t == mirror.t);

    // ---------------------------------------------------------------
    // Pair formation (photon-like P sources).
    // Two overlapping bubbles with the same wavefront time and complementary
    // charges can form a pair. The pair is "dressing" if both bubbles already
    // share the same leader; otherwise it is a free photon.
    // ---------------------------------------------------------------
    if (samePos && sameT && canFormPair(currSrc, mirrorSrc))
    {
      bool dressing = (currSrc.leader_w != NO_LEADER_W &&
                       currSrc.leader_w == mirrorSrc.leader_w);
      WIndex newLeader = dressing ? currSrc.leader_w : NO_LEADER_W;
      unsigned newA    = dressing ? (unsigned)newLeader : W_USED;
      WIndex parent    = dressing ? currSrc.leader_w : NO_PARENT;

      // If both sources are already a pair with each other, do not re-form.
      bool alreadyPaired = (currSrc.kind == SourceKind::P &&
                            mirrorSrc.kind == SourceKind::P &&
                            currSrc.pair_idx == mirrorSrc.w &&
                            mirrorSrc.pair_idx == currSrc.w);
      if (alreadyPaired)
        return false;

      uint8_t newCount = 1;
      if (currSrc.kind == SourceKind::P) newCount += currSrc.pair_count;
      if (mirrorSrc.kind == SourceKind::P) newCount += mirrorSrc.pair_count;

      currDraft.kind  = SourceKind::P;
      mirrorDraft.kind = SourceKind::P;
      currDraft.pair_idx   = mirrorSrc.w;
      mirrorDraft.pair_idx = currSrc.w;
      currDraft.pair_count = newCount;
      mirrorDraft.pair_count = newCount;
      currDraft.leader_w  = newLeader;
      mirrorDraft.leader_w = newLeader;
      currDraft.a  = newA;
      mirrorDraft.a = newA;
      currDraft.parent  = parent;
      mirrorDraft.parent = parent;

      // Move both source centers to the contact point and reset their clocks.
      reemitAtContact(currDraft, curr);
      reemitAtContact(mirrorDraft, mirror);
      return false;
    }

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
      // Collapse flag propagates inward and triggers reissue.
      draft.kB = true;
      draft.cB = true;
    }
    else
    {
      // Adiabatic: no collapse, but the two sources exchange
      // leader identity, affinity and light clock, then drift one step
      // toward each other.
      WIndex minLeader = dominantLeader(currSrc, mirrorSrc);
      adoptLeader(currDraft, minLeader);
      adoptLeader(mirrorDraft, minLeader);
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
      currDraft.parent = mirrorSrc.w;
      adoptLeader(currDraft, mirrorSrc.leader_w == NO_LEADER_W ? mirrorSrc.w : mirrorSrc.leader_w);
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
        // Opposite field sign: both become D delegates of the dominant leader.
        WIndex leader = dominantLeader(currSrc, mirrorSrc);
        currDraft.kind  = SourceKind::D;
        mirrorDraft.kind = SourceKind::D;
        currDraft.parent  = leader;
        mirrorDraft.parent = leader;
        adoptLeader(currDraft, leader);
        adoptLeader(mirrorDraft, leader);
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
      WIndex leader = (dSrc.leader_w == NO_LEADER_W ? dSrc.parent : dSrc.leader_w);
      if (leader == NO_LEADER_W) leader = dSrc.w;
      sSrcDraft.kind = SourceKind::D;
      sSrcDraft.parent = dSrc.parent == NO_PARENT ? leader : dSrc.parent;
      adoptLeader(sSrcDraft, leader);

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
        // Simplified momentum exchange: add the mirror's momentum direction
        // to the target's relocation impulse. applyMomentum will update m.
        currDraft.reloc[0] += mirrorSrc.m[0];
        currDraft.reloc[1] += mirrorSrc.m[1];
        currDraft.reloc[2] += mirrorSrc.m[2];
      }
      else
      {
        // Same tribe: outer delegate imposes spin_target on inner one.
        // (Tangential/radial orbital motion is left as a refinement.)
        currDraft.spin_target = mirrorSrc.spin_target;
        // Enforce a common leader identity.
        WIndex leader = dominantLeader(currSrc, mirrorSrc);
        adoptLeader(currDraft, leader);
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
      mirrorDraft.reloc[0] += currSrc.m[0];
      mirrorDraft.reloc[1] += currSrc.m[1];
      mirrorDraft.reloc[2] += currSrc.m[2];
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
      currDraft.reloc[0] += mirrorSrc.m[0];
      currDraft.reloc[1] += mirrorSrc.m[1];
      currDraft.reloc[2] += mirrorSrc.m[2];
      return false;
    }

    // P x P is suppressed.
    return false;
  }

}
