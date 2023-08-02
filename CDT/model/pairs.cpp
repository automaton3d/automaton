/*
 * pairs.cpp
 *
 *  Created on: 3 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

namespace automaton
{

// Pointers to the current stable and draft cells.

extern Cell *stb, *drf;

/**
 * Detects multipair or new pair.
 *
 * @t physical time
 * @nxt a stable cell with same x, y, z coords. in espacito
 * @last a drafy cell with same x, y, z coords. in espacito
 */
void pairs(int t, Cell *nxt, Cell *lst)
{
  // Bubbles not superposing?

  if (!superpose(stb->off, nxt->off) || !ZERO(stb->o) ||
      !ZERO(nxt->o) || ZERO(stb->p) || ZERO(nxt->p))
  {
    return;
  }

  // Combine pairs to form multi-pairs.
  // (Sect. 5.2)

  //printCell(stb);
  //printCell(nxt);
  if (stb->k > FERMION && stb->k == nxt->k)
  {
    if (stb->a1 != nxt->a1)
    {
      // Shift a1 to a2.

      drf->a2 = stb->a1;
      lst->a2 = nxt->a1;

      // Both a1 receive the same new value.
      // Care must be taken to avoid collision of values.

      drf->a1 = stb->a1 | (nxt->a1 << ORDER);
      lst->a1 = drf->a1;
    }

    // Pair homogenization.

    else
    {
      if (stb->a1 != nxt->a1)
      {
        unsigned temp = 0;
        if (stb->a1 == nxt->a2)
        {
          // Swap.

          temp = nxt->a1;
          nxt->a1 = nxt->a2;
          nxt->a2 = temp;
        }
        if (stb->a2 == nxt->a1)
        {
          // Swap.

          temp = stb->a1;
          stb->a1 = stb->a2;
          stb->a2 = temp;
        }
      }
    }
  }
  else if (stb->k == FERMION && nxt->k == FERMION)
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
        assert(!ANTI(stb->p, nxt->p));
        drf->k = SPHOTON;
        lst->k = SPHOTON;

        // Compose the common value for affinity.
        // (Sect. 4.2)

        drf->a1 |= (nxt->a1 << ORDER);
        lst->a1 |= (stb->a1 << ORDER);
      }

      // Momenta misaligned?

      else if (ANTI(stb->p, nxt->p))
      {
        // Symmetry breaking after singularity.
        // (Sect. 4.6.7)

      std::cout << "symm break" << std::endl;
      printCell(stb);
        CP(drf->po, stb->p);
        CP(lst->po, nxt->p);
      }
    }

    // Clump bubbles together to form particle pair fragments.
    // (Sect. 4.2)

    else if (stb->a1 != nxt->a1)
    {
      if (MAT(stb) != MAT(nxt) && W0(stb) == !W0(nxt) &&
          Q(stb) == !Q(nxt))
      {
//        puts("\tgluon");
        drf->k = GLUON;
        lst->k = GLUON;
      }
      else if (C(stb) == C(nxt) &&
               C(stb) != 0 && C(stb) != 7 &&
               Q(stb) == Q(nxt) && Q(stb) == W1(stb))
      {
//        puts("\tup");
        drf->k = UP;
        lst->k = UP;
      }
      else if (C(stb) == _C(nxt) && W0(stb) == !W0(nxt) &&
               Q(stb) == !Q(nxt))
      {
//        puts("\tphoton");
        drf->k = PHOTON;
        lst->k = PHOTON;
      }
      else if (C(stb) == C(nxt) &&
              (C(stb) == 0 || C(stb) == 7) &&
              Q(stb) != Q(nxt) && W0(stb) == W0(nxt) &&
              W0(stb) != W1(stb))
      {
//        puts("\tnu");
        drf->k = NEUTRINO;
        lst->k = NEUTRINO;
      }
      else if (C(stb) == _C(nxt) &&
              (W0(stb) != W1(stb) || W0(nxt) != W1(stb)) &&
              Q(stb) == !Q(nxt))
      {
      // puts("\tZ");
        drf->k = ZB;
        lst->k = ZB;
      }
      else if (C(stb) == _C(nxt) && W0(stb) == W0(nxt) &&
               Q(stb) == Q(nxt) && W0(stb) != W1(stb))
      {
//        puts("\tW");
        drf->k = WB;
        lst->k = WB;
      }
      else
      {
        return;
      }

      // Calculate the common value for affinity.
      // (Sect. 4.2)

      drf->a1 = stb->a1 | (nxt->a1 << ORDER);
      lst->a1 = drf->a1;
    }
  }
}

}
