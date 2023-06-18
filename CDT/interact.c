/*
 * interact.c
 *
 *  Created on: 3 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

extern Cell *stb, *drf;

/**
 * Detects interactions.
 */
void interact (Cell *nxt, Cell *lst)
{
  // Empty cells not allowed.

  if (!BUSY(stb) || !BUSY(nxt))
    goto TICK;

  // Calculate the general type of interaction.

  int code = (nxt->a1 == drf->a1) | (!ZERO(nxt->p) << 1) |
             (!ZERO(drf->p) << 2);

  // Not settled?

  if (!ZERO(stb->po) || !ZERO(nxt->po))
    goto TICK;

  int t = stb->n / LIGHT;

  // Self interference (Sect. 5.6.3)?
  // (Sciarretta)

  if (stb->a1 == nxt->a1 && !ZERO(stb->p) && !ZERO(nxt->p))
  {
    // From the same region?
    // (Eq. 7, Eq. 8)

    int dif[3];
    SUB(dif, stb->o, nxt->o);
    if (DOT(nxt->o, stb->o) == 1 && MAG(dif) == 0)
    {
      // Induce empodion (Sect. 5.6.3).
      // With reciprocity.

      if (nxt->k == FERMION && ISSAT(stb->o))
      {
        CP(lst->m, drf->pP);
      }
      else if (ISSAT(nxt->o) && stb->k == FERMION)
      {
        CP(drf->m, nxt->pP);
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3).
      // With reciprocity.

      if (nxt->k == FERMION && GET_ROLE(stb) == GRID)
      {
        CP(lst->o, drf->o);
      }
      else if (stb->k == FERMION && GET_ROLE(nxt) == GRID)
      {
        CP(drf->o, nxt->o);
      }
    }
  }

  // Detect interaction.

  else if (stb->n < stb->u * t)
  {
    // Default reissue at c.p.

    drf->obj = SIDE3;
    lst->obj = SIDE3;
    RSET(drf->po);
    RSET(lst->po);
    RSET(drf->m);
    RSET(lst->m);

    // Test for same or different sectors.

    if (W1(nxt) == W1(drf))
    {
      // Same-sector.

      switch(code)
      {
        case 2:case 4:    // Weak interaction

          // Particles are different.
          // Virtual photon capture.
          // Part of electrical, magnetic or
          // interference interactions.

          if (stb->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Draft is now a propeller.

            drf->a1 = nxt->a1;
          }
          else if (nxt->k == PHOTON &&
                   DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Last is now a propeller.

            lst->a1 = stb->a1;
          }

          // Static forces.

          else if (drf->k == FERMION && !ZERO(nxt->p))
          {
            // Electric radial force.

            if (Q(nxt) == Q(stb))
            {
              // Use tick parity to distinguish E or M case.

              if (stb->n % SIDE_2 == 0)
              {
                // Repulsion.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, stb->o, nxt->o);
              }
              else
              {
                // Magnetic lateral kick.
                // drf is now a messenger.
                // (Sect. 5.6.2)

                CROSS(lst->m, lst->m, nxt->s);
              }
            }
            else
            {
              // Use tick parity to distinguish E or M case.

              if (stb->n % SIDE_2 == 0)
              {
                // Attraction.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, nxt->o, drf->o);
              }
              else
              {
                // Magnetic lateral kick.
                // drf is now a messenger.
                // (Sect. 5.6.2)

                CROSS(lst->m, lst->m, nxt->s);
              }
            }
          }

          // boson x boson.

          else if (nxt->k > FERMION && stb->k > FERMION)
          {
            // Exchange charges and spins.

            drf->ch &= (C_MASK | W0_MASK);
            drf->ch |= (nxt->ch & (C_MASK | W0_MASK));
            CP(drf->s, nxt->s);
            CP(drf->po, stb->p);

            // Reciprocity.

            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
            CP(lst->po, nxt->p);
            CP(lst->s, stb->s);
          }
          break;

        case 3:     // Inertia

          // Calculate parallel transported pole.
          // bubbles have the same a1 value.
          // nxt is the master, stb is the slave.

          SUB(drf->po, nxt->o, stb->o);
          break;

        case 5:    // Inertia

          // Counterpart.
          // stb is the master, nxt is the slave.

          SUB(lst->po, stb->o, nxt->o);
          break;

        case 6:    // Collapse

          // Covers annihilation , light-matter
          // and scattering.

          drf->k = COLLAPSE;
          lst->k = COLLAPSE;
          drf->obj = drf->a1;
          lst->obj = lst->a1;
          break;

        case 7:    // Internal collision

          // Cohesion.
          // (Sect. 4.1)

          if (stb->k == FERMION && nxt->k == FERMION)
          {
            CPNEG(drf->po, drf->p);
            CPNEG(lst->po, lst->p);
          }
          else if (stb->k == WB && nxt->k == WB)
          {
            CPNEG(lst->po, lst->p);
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            CPNEG(lst->po, lst->p);
          }
          break;
      }
    }

    // Inter-sector interaction.
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion.

      if (nxt->k == FERMION && stb->k == SPHOTON)
      {
        CPNEG(drf->po, drf->p);
      }
      else if (stb->k == FERMION && nxt->k == SPHOTON)
      {
        CPNEG(lst->po, lst->p);
      }
    }
  }

  TICK: ;
}
