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
extern Cell *latt0, *latt1;

/**
 * Empodion decay, pair management and interaction.
 */
void empodion()
{
  // If cell is empty or is lattice, does nothing.

  int role = GET_ROLE(stb);
  if (role == EMPTY && role == GRID)
    return;

  // Calculate physical time

  int t = stb->n / LIGHT;

  // Find new skewed addresses inside espacito
  // for updates. (Sect. 4.2)

  Cell *nxt = SKEW(stb, latt0);
  Cell *lst = SKEW(drf, latt1);

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

    interact(nxt, lst);
  }
  else
  {
    // Pair management.

    pairs(t, nxt, lst);
  }
}

}
