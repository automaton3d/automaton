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
  // on the crest of the wave.

  if(!ZERO(stb->p))
    drf->occ = LIGHT;

  // --- Execute interaction decisions ---

  // Symmetry breaking?

  if (stb->k == FOWLS && !ZERO(stb->p))
    drf->k = FERMION;  // friends now

  // --- Beginning of affine operations ---

  // Calculate physical time.

  int t = stb->n / LIGHT;

  // Find new skewed addresses for updates.
  // (Sect. 4.2)

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
  // stb->ob not null means collapse.
  // Only non trivial p matters.

  if (nxt->a == stb->obj && !ZERO(nxt->p))
  {
    // Reissue this particular bubble.
    // Collapse happens at c.p.

    RSET(lst->po);      // reissue from c.p.
    RSET(lst->o);       // radius 0
    lst->syn = 0;       // ready for immediate expansion
    lst->n   = 0;       // synchronize clocks
    lst->obj = SIDE3;   // no target in view

    // Reset sine wave.
    // Sect. 6.3 and (Eq. 2).

    lst->u   = 0;       // always reset sine
    lst->pmf = 0;       // and PMF
    lst->den = 1;       // 2^0
    lst->pow = 1;       // y^0

    // Always let searched pole wrappable.
    // When effectively reissued, give proper value.

    MILD(lst->po);
    RSET(lst->m);       // not a messenger
  }

  // All related bubbles are bumbed.

  else if (nxt->a == stb->obj)
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

  // Last pass of wavefront?

  if (stb->n % LIGHT == 0)
  {
    // Particle empodion decay (Eqn. 6)?

    if (!ZERO(stb->pP))
    {
      // Cell belongs to a particle empodion.

      // Vector p shrinks (Eq. 6).

      assert(t > 0);
      drf->pP[0] -= drf->pP[0] / (2 * t);
      drf->pP[1] -= drf->pP[1] / (2 * t);
      drf->pP[2] -= drf->pP[2] / (2 * t);

      // Disconnect empodion?
      // (Sec. 4.7.3)

      if (ZERO(drf->pP))
        drf->a = stb->oE + 1;  // default value

      // TODO: check pair homogenization.

      // Update lattice decay.
      // A saturated o vector means lattice role.

      if (ISSAT(stb->o) && t > 0)
      {
        // Lattice track decay (Eq. 5).

        assert(t > 0);
        drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
        drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
        drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);

        // Disconnect empodion?
        // (Sect. 4.7.3)

        if (ZERO(drf->pP))
          drf->a = stb->oE + 1; // default
      }
    }

    // A light pass was completed:
    // detect interactions.

    interact(stb, drf, nxt, lst);
  }
  else
  {
    // Update stuff here.

    pairFormation(t, stb, drf, nxt, lst);
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

    nei = drf->ws[dir];

    if (nei->occ == 0)
      nei->occ = LIGHT;
    else
      continue;

    if(ISMILD(org))
      continue;

    // Timing

    if (stb->n * stb->n < stb->syn)
    {
      nei->syn = stb->syn;
    }
    else
    {
      // Transmit spherical info.
      // Set timing for spherical pattern (Sect. 3).

      nei->syn = LIGHT2 * MOD2(org);
    }

    // Momentum is always at the eventual reissue cell

    if(EQ(stb->o, stb->po))
    {
      RSET(nei->p);
    }
    else
    {
      CP(nei->p, stb->p);

      // Calculate alignment with spin.

      int dot = abs(DOT(org, stb->s));

      // Select momentum destination.
      // (Sect. 3.1)

      if (dot >= dotMax)
      {
        dotMax = dot;
        tx = nei;
      }
    }

    // Transmit superluminal info

    nei->obj = stb->obj;
  }
  if(tx != NULL)
  {
    RSET(drf->p);
    CP(tx->po, stb->po);
  }

  // Clock tick.

  drf->n++;
}
