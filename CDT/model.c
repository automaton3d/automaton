/*
 * model.c
 */

#include "simulation.h"

Cell *latt0, *latt1, *stb, *drf;

int getRole(Cell *c)
{
	int role;
	if (c->a1 == 0)
		role = EMPTY;
	else if (!ZERO(c->p))
		role = SEED;
	else if (!ZERO(c->s))
		role = WAVE;
	else if (ISSAT(c->o))
		role = GRID;
	else if (!ZERO(c->pP))
		role = TRAIL;
	else
		role = TRAVELLER;
	return role;
}

void model(int phase)
{
  // If cell is empty, does nothing.

  if (getRole(stb) == EMPTY)
    goto TICK;

  if (phase == 0)
  {
    // --- Beginning of affine operations ---

    // This sequence is asynchronous, that is, happens
    // many times in a light step.

    int t = stb->n / LIGHT;  // Calculate physical time

    // Find new skewed addresses inside espacito
    // for updates. (Sect. 4.2)

    int oE = stb->off % SIDE3;
    int off;
    if (oE % 2 == 0)
      off = (oE + (t + 1)) % SIDE3;
    else
      off = (oE - (t + 1)) % SIDE3;
    Cell *nxt = stb - oE + off;
    Cell *lst = drf - oE + off;

    // Synchronize code.

    //join();

    // A collapse forces all affine bubbles to reissue.
    // nxt->a1 is the skewed affinity being explored.
    // stb->obj is the affinity inherited from interaction.
    // Only non trivial p matters.
    // Annihilation makes all affinities different.
    // Secondary match is attempted using a2, so that
    // composite particles are considered.

    if ((nxt->a1 == stb->obj || nxt->a2 == stb->obj) &&
        !ZERO(nxt->p) && stb->k == COLLAPSE)
    {
      // Target found.
      // Reissue this particular bubble.
      // Collapse happens at c.p.

      RSET(lst->po);    // reissue from c.p.
      RSET(lst->o);     // radius 0
      lst->syn = 0;     // ready for immediate expansion
      lst->n   = 0;     // synchronize clocks
      lst->obj = SIDE3; // no target in view

      // Reset sine wave.
      // Sect. 6.3 and (Eq. 2).

      lst->u   = 0;     // always reset sine
      RSET(lst->m);     // not a messenger

      // Dissolution?

      if (ANNIHIL(stb, nxt))
      {
        // Free from particle restoring
        // default value.

        lst->a1 = (nxt->off % SIDE3) + 1;
      }
    }

    // No reissue ocurred so the net effect
    // is a high frequency sine, that is,
    // all related bubbles are bumped.

    else if (nxt->a1 == stb->obj)
    {
      // Bump sine generator.
      // (Euler formula for sine)

      int tn = nxt->n / LIGHT;  // skewed time
      if (tn > 0)
      {
        lst->u = nxt->u * (tn * tn - MOD2(nxt->o)) / (tn * tn);
      }
    }

    // Last pass of wavefront?

    if (stb->n % LIGHT == 0)
    {
      // Particle empodion decay (Eqn. 6)?

      if (!ZERO(stb->pP))
      {
        // Cell belongs to a particle empodion.
        // Vector p shrinks (Eq. 6) to 0, when then
        // empodion disconnects Sec. 4.7.3).

        drf->pP[0] -= drf->pP[0] / (2 * t);
        drf->pP[1] -= drf->pP[1] / (2 * t);
        drf->pP[2] -= drf->pP[2] / (2 * t);
        if (ZERO(drf->pP))
          drf->a1 = (stb->off % SIDE3) + 1; // default vale

        // Update lattice decay.

        if (ISSAT(stb->o) && t > 0)
        {
          // Lattice track decay (Eq. 5).
          // When pP=0, empodion disconects. (Sect. 4.7.3)

          drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
          drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
          drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);
          if (ZERO(drf->pP))
            drf->a1 = (stb->off % SIDE3) + 1; // default vale
        }
      }

      // A light pass was completed, detect interactions.

      interact(stb, drf, nxt, lst);
    }
    else
    {
      // Pair management.

      managePairs(t, stb, drf, nxt, lst);
    }

    // --- End of affine operations ---

    // Synchronize code.

    //join();

    // The cell has been modified, do not expand now.

    goto TICK;
  }

  // Spherical expansion.

  for(int dir = 0; dir < NDIR; dir++)
  {
    // The first part of this block propagates info
    // asynchronously.

    Cell* nei = drf->ws[dir];

    // Do not touch wavefront.

    if (nei->occ == 0 && !BUSY(nei) && getRole(stb) == TRAVELLER)
    {
      // Transmit superluminal info

      nei->occ = stb->occ;
      nei->n   = stb->n;    // used to calculate skew
      nei->obj = stb->obj;  // used for collapse
      CP(nei->p, stb->p);	// used to find pole
    }

    // The second part of the block deals with
    // spherical synchronization.

    int org[3];
    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;
    if (abs(org[i]) < abs(stb->o[i]))
      continue;

    // Wrapped?
    // This serves both for luminal and superluminal.

    if(ISMILD(org))
      continue;

    // Transmit superluminal info

    nei->n   = stb->n;    // ticks
    nei->obj = stb->obj;  // target
    nei->ch  = stb->ch;   // charges
    nei->a1  = stb->a1;   // affinity LO
    nei->a2  = stb->a2;   // affinity HO
    nei->k   = stb->k;    // content
    nei->occ = stb->occ;  // occupation status
    CP(nei->m, stb->m);   // messenger status

    // Propagate exclusive spherical info.

    nei->u = stb->u;      // sine
    CP(nei->s, stb->s);   // spin
    CP(nei->o, stb->o);   // bubble origin
    CP(nei->pP, stb->pP); // decay

    // Spherical synchronism.

    nei->syn = LIGHT2 * MOD2(org);  // mature

    // Let neighbor wrappable by default.

    MILD(nei->po);

    // Select momentum destination (Sect. 3.1).
    // Uses calculated alignment with spin.

    if (DOT(org, stb->s) == 1)
    {
      CP(nei->p, stb->p);   // propagate momentum
      CP(nei->po, stb->po); // propagate pole
      nei->occ = SIDE_2 - 1;
    }
    else
    {
      RSET(nei->p);
    }
  }

  // This marks cell as empty (no wavefront), but
  // trail info remains.

  SAT(drf->po);

  // Obs.: momentum was left in cell as a trail.

  TICK: ;
}
