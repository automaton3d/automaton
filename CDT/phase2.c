/*
 * affine.c
 *
 *  Created on: 20 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

extern Cell *stb, *drf;

/*
 * Superluminal updates.
 */
void phase2()
{
  // If cell is empty, does nothing.

  if (GET_ROLE(stb) == EMPTY)
    return;

  // Calculate physical time

  //int t = stb->n / LIGHT;

  // Affine operations?

  if (GET_ROLE(stb) == TRAVELLER)
  {
    // This sequence is asynchronous, that is, happens
    // many times in a light step.

    // Find new skewed addresses inside espacito
    // for updates. (Sect. 4.2)

	  /*
    int oE = stb->off % SIDE3;
    int off;
    if (oE % 2 == 0)
      off = (oE + (t + 1)) % SIDE3;
    else
      off = (oE - (t + 1)) % SIDE3;
    Cell *nxt = stb - oE + off;
    Cell *lst = drf - oE + off;
*/

    Cell *nxt = stb + (stb->n * SIDE) % SIDE3;
    Cell *lst = drf + (drf->n * SIDE) % SIDE3;

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
  }
}
