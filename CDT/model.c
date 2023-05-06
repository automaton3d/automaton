/*
 * model.c
 *
 *  Created on: 2 de mai. de 2023
 *      Author: Alexandre
 */

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "simulation.h"
#include "utils.h"

Cell *latt0, *latt1, *stb, *drf;

void model()
{
  // Occupancy vanish.

  if(stb->occ)
    drf->occ--;

  // Momentum is always
  // on the crest of the waves.

  if(!ZERO(stb->p))
    drf->occ = LIGHT;

  // --- Beginning of affine operations ---

  // Calculate physical time.

  int t = stb->n / LIGHT;

  // Find new skewed addresses inside espacito
  // for updates. (Sect. 4.2)

  int off;
  if (stb->oE % 2 == 0)
    off = (stb->oE + (t + 1)) % SIDE3;
  else
    off = (stb->oE - (t + 1)) % SIDE3;
  Cell *nxt = stb - stb->oE + off;
  Cell *lst = drf - drf->oE + off;

  // A collapse forces all affine bubbles to reissue.
  // nxt->a is the skewed cell affinity that the flash
  // is exploring.
  // stb->obj is the affinity inherited from interaction.
  // Only non trivial p matters.
  // Annihilation makes all affinities different.

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
    lst->pmf = 0;     // and PMF
    lst->den = 1;     // 2^0
    lst->pow = 1;     // y^0
    RSET(lst->m);     // not a messenger

    // Dissolution?

    if (ANNIHIL(stb, nxt))
    {
    	// Free from particle

    	lst->a1 = nxt->oE + 1;
    }
  }

  // All related bubbles are bumped.

  else if (nxt->a1 == stb->obj)
  {
    // Bump sine generator.
    // (Euler formula for sine)

    int tn = nxt->n / LIGHT;  // skewed time
    if (tn > 0)
    {
      lst->u = nxt->u * (tn * tn - MOD2(nxt->o)) / (tn * tn);
    }

    // Update the sine signal PMF (Eqn. 4).
    // Only the skewed cell is affected.

    if (nxt->den < SIDE_2)  // equiv. to infinity in summation
    {
      lst->pow *= nxt->pow * nxt->pow;  // y^(2n)
      lst->den <<= 1;                   // 2^n
      lst->pmf += (2 * tn + 1) * lst->pow / lst->den;
    }
  }

  // --- End of affine operations ---

  // Last pass of wavefront?

  if (stb->n % LIGHT == 0)
  {
    // --- Beginning of Sciarreta operations ---

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
        drf->a1 = stb->oE + 1;  // default value

      // Update lattice decay.
      // A saturated o vector means lattice role.

      if (ISSAT(stb->o) && t > 0)
      {
        // Lattice track decay (Eq. 5).
        // When pP=0, empodion disconects. (Sect. 4.7.3)

        drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
        drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
        drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);
        if (ZERO(drf->pP))
          drf->a1 = stb->oE + 1; // default vale
      }
    }

    // --- End of Sciarreta operations ---

    // A light pass was completed, detect interactions.

    interact(stb, drf, nxt, lst);
  }
  else
  {
    // Pair management.

    managePairs(t, stb, drf, nxt, lst);
  }

  // Expansion

  Cell *tx = NULL;
  int dotMax = 0;
  Cell* nei;
  for(int dir = 0; dir < 6; dir++)
  {
    int org[3];
    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;
    if (abs(org[i]) < abs(stb->o[i]))
      continue;

    assert(!(abs(org[0]) == SIDE_2 + 1 ||
	        abs(org[1]) == SIDE_2 + 1 ||
	        abs(org[2]) == SIDE_2 + 1));

    // Wrapped?

    if(ISMILD(org))
      continue;

    nei = drf->ws[dir];

    if (nei->occ == 0)
      nei->occ = LIGHT;
    else
      continue;

    // Momentum propagates by default.

    CP(nei->p, stb->p);

    // Transmit superluminal info

    nei->obj = stb->obj;  // target

    // --- Spherical propagation ---

    // Spherical synchronism.

    if (stb->n * stb->n < stb->syn)
      continue;                     // immature

    // Transmit spherical info.
    // Set timing for spherical pattern (Sect. 3).

    nei->syn = LIGHT2 * MOD2(org);  // mature

    // Let neighbor wrappable by default.

    MILD(nei->po);

    // Momentum already at reissue cell.

    if(EQ(stb->o, stb->po))
    {
      // Cease momentum spreading.

      RSET(nei->p);
    }

    // Propagate other info.

    nei->ch  = stb->ch;   // charges
    nei->a1   = stb->a1;    // affinity
    nei->n   = stb->n;    // ticks
    nei->u   = stb->u;    // sine
    nei->pmf = stb->pmf;  // sine PMF
    nei->k   = stb->k;    // kind
    CP(drf->s, stb->s);   // spin
    CP(drf->o, stb->o);   // bubble origin
    CP(drf->pP, stb->pP); // decay
    CP(drf->m, stb->m);   // messenger

    // Calculate alignment with spin.

    int dot = abs(DOT(org, stb->s));

    // Select momentum destination (Sect. 3.1).

    if (dot >= dotMax)
    {
      dotMax = dot;
      tx = nei;
    }
  }

  // A target cell for momentum was found?

  if(tx != NULL)
  {
    // If pole not found yet, release po.

    if(!EQ(stb->o, stb->po))
      CP(tx->po, stb->po);   // propagate pole
  }

  // Obs.: momentum was left in cell as a trail.

  // Clock tick.

  drf->n++;
}
