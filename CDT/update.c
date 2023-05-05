/*
 * update.c
 *
 *  Created on: 3 de mai. de 2023
 *      Author: Alexandre
 */

#include <assert.h>
#include "simulation.h"

/**
 * Detects new pair and multipair.
 */
void pairFormation(int t, Cell *stb, Cell *drf, Cell *nxt, Cell *lst)
{
  // Pair already formed?

  if (stb->k > FERMION || nxt->k > FERMION)
  {
    // Not symmetry breaking?

    if (stb->k != FOWLS && nxt->k != FOWLS)
    {
      // Combine pairs to form multi-pairs.
      // (Sect. 5.2)

      if (stb->k == nxt->k && stb->k > FERMION &&
          stb->a != nxt->a)
      {
        drf->a = nxt->a;
        lst->a = drf->a;
      }
    }
    return;
  }

  // Superposing bubbles?

  if (stb->oL == nxt->oL &&
      ZERO(stb->o) && ZERO(nxt->o) &&
      ZERO(stb->p) && ZERO(nxt->p))
  {
    // Check if different sectors.
    // (Sec. 4.7.7)

    if (W1(stb) != W1(nxt))
    {
      // Super photon formation.
      // (Sect. 3.1)

      if (C(stb) == _C(nxt) && W0(stb) == !W0(nxt) &&
          Q(stb) == !Q(nxt))
      {
        drf->k = SPHOTON;
        lst->k = SPHOTON;

        // Compose the common value for affinity.
        // (Sect. 4.2)

        drf->a |= (nxt->a << ORDER);
        lst->a |= (stb->a << ORDER);
      }
      else
      {
        // Symmetry breaking after singularity.
        // (Sect. 4.6.7)

        CP(drf->po, stb->p);
        CP(lst->po, nxt->p);
        drf->k = FOWLS;
        lst->k = FOWLS;
      }
    }

    // Clump bubbles together to form particle pair fragments.
    // (Sect. 4.2)

    else if (stb->a != nxt->a &&
             stb->k == FERMION &&
	         nxt->k == FERMION && 0)
    {
      if (MAT(stb) != MAT(nxt) && W0(stb) == !W0(nxt) &&
          Q(stb) == !Q(nxt))
      {
        drf->k = GLUON;
        lst->k = GLUON;
      }
      else if (C(stb) == C(nxt) && C(stb) != 0 && C(stb) != 7 &&
               Q(stb) == Q(nxt) && Q(stb) == W1(stb))
      {
        drf->k = UP;
        lst->k = UP;
      }
      else if (C(stb) == _C(nxt) && W0(stb) == !W0(nxt) &&
               Q(stb) == !Q(nxt))
      {
        drf->k = PHOTON;
        lst->k = PHOTON;
      }
      else if (C(stb) == C(nxt) &&
              (C(stb) == 0 || C(stb) == 7) &&
              Q(stb) != Q(nxt) && W0(stb) == W0(nxt) &&
              W0(stb) != W1(stb))
      {
        drf->k = NEUTRINO;
        lst->k = NEUTRINO;
      }
      else if (C(stb) == _C(nxt) &&
               (W0(stb) != W1(stb) || W0(nxt) != W1(stb)) &&
	            Q(stb) == !Q(nxt))
      {
        drf->k = ZB;
        lst->k = ZB;
      }
      else if (C(stb) == _C(nxt) && W0(stb) == W0(nxt) &&
               Q(stb) == Q(nxt) && W0(stb) != W1(stb))
      {
        drf->k = WB;
        lst->k = WB;
      }
      else
      {
    	return;
      }

      // Calculate the common value for affinity.
      // (Sect. 4.2)

      drf->a = stb->a | (nxt->a << ORDER);
      lst->a = drf->a;
    }
  }
}

