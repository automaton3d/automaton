/*
 * phase3.c
 *
 *  Created on: 20 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

namespace automaton
{

extern Cell *stb, *drf;

/**
 * Empodion decay, pair management and interaction.
 */
void empodion()
{
  // If cell is empty, do nothing.

  int role = GET_ROLE(stb);
  if (role == EMPTY)
    return;

  // Calculate physical time

  int t = stb->n / LIGHT;

  // Find new skewed addresses inside espacito
  // for updates. (Sect. 4.2)

  Cell *nxt = skew(stb);
  Cell *lst = skew(drf);

  // Last pass of wavefront?

  if (stb->n % LIGHT == 0)
  {
    // Particle empodion decay (Eqn. 6)?

    if (!ZERO(stb->pP))
    {
      if (role == GRID)
      {
        // Update lattice decay.
        if (ISSAT(stb->o) && t > 0)
        {
          // Lattice track decay (Eq. 5).

          drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
          drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
          drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);

          // When pP=0, empodion disconects. (Sect. 4.7.3)

          if (ZERO(drf->pP))
            drf->a1 = (stb->off % SIDE3) + 1; // default vale
        }

        // The only processing allowed to GRID role ends here.

        return;
      }
      else
      {
        // Cell belongs to a particle empodion.
        // Vector p shrinks (Eq. 6) to 0, when then
        // empodion disconnects Sec. 4.7.3).

        drf->pP[0] -= drf->pP[0] / (2 * t);
        drf->pP[1] -= drf->pP[1] / (2 * t);
        drf->pP[2] -= drf->pP[2] / (2 * t);
        if (ZERO(drf->pP))
          drf->a1 = (stb->off % SIDE3) + 1; // default vale
      }
    }

    // A light pass was completed, detect interactions.

    interact(nxt, lst);
  }
  else
  {
    // Pair management.

    pairs(t, nxt, lst);
  }
}

}
