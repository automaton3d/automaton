/*
 * interact.c
 *
 *  Created on: 3 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"
#include "utils.h"

/**
 * Detects interactions.
 */
void interact (Cell *stb, Cell*drf, Cell *nxt, Cell *lst)
{
  int pole[3];
  int code = (nxt->a == drf->a) | (!ZERO(nxt->p) << 1) |
	           (!ZERO(drf->p) << 2);

  // Internal process?

  if (stb->a == nxt->a && !ZERO(stb->p) && !ZERO(nxt->p))
  {
    // Self interference (Sect. 5.6.3).
    // (Sciarretta)

    int dif[3];
    SUB(dif, stb->o, nxt->o);

    // From the same region?
    // (Eq. 7, Eq. 8)

    if (DOT(nxt->o, stb->o) == 1 && MOD2(dif) == 0)
    {
      // Induce empodion (Sect. 5.6.3).

      if (nxt->k == FERMION && ISSAT(stb->o))
      {
        CP(lst->m, drf->pP);
        goto TICK;
      }

      // Reciprocity.

      else if (ISSAT(nxt->o) && stb->k == FERMION)
      {
        CP(drf->m, nxt->pP);
        goto TICK;
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3).

      if (nxt->k == FERMION && ISSAT(stb->o))
      {
        CP(lst->o, drf->o);
        goto TICK;
      }

      // Reciprocity.

      else if (ISSAT(nxt->o) && stb->k == FERMION)
      {
        CP(drf->o, nxt->o);
        goto TICK;
      }
    }
  }

  // Play pseudo dice to decide bubble mixing.

  else if (stb->r < drf->pmf && code > 1)
  {
    // Default values

    RSET(drf->po);
    RSET(lst->po);

    // Test for same or different sectors.

    if (W1(nxt) == W1(drf))
    {
      // Same-sector.

      switch(code)
      {
        case 2:    // Rendez vous (different particles)
        case 4:    // Rendez vous (different particles)

          // Virtual photon capture.
          // Part of electrical, magnetic or
          // interference interactions.

          if (drf->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Draft is now a propeller.

            drf->a = nxt->a;
            RSET(lst->m);
          }
          else if (lst->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Last is now a propeller.

            lst->a = stb->a;
            RSET(drf->m);
          }

          // Static forces.

          else if (BUSY(nxt) && drf->k == FERMION &&
                   !ZERO(nxt->p))
          {
            if (stb->r % 2 == 0)
            {
              // Electric radial force.

              if (Q(nxt) == Q(stb))
              {
                // Repulsion.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, drf->o, nxt->o);
              }
              else
              {
                // Attraction.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, nxt->o, drf->o);
              }
            }
            if (nxt->r % 2 == 0)
            {
              // Magnetic lateral kick.
              // drf is now a messenger.
              // (Sect. 5.6.2)

              CROSS(lst->m, lst->m, nxt->s);
            }
          }

          // boson x boson.

          else if (lst->k > FERMION && drf->k > FERMION)
          {
            // Exchange charges and spins.

            CP(drf->s, nxt->s);
            drf->ch &= (C_MASK | W0_MASK);
            drf->ch |= (nxt->ch & (C_MASK | W0_MASK));
            CP(drf->po, stb->p);

            // Reciprocity.

            CP(lst->po, nxt->p);
            CP(lst->s, stb->s);
            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole.

          SUB(drf->po, nxt->o, drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial.

          CP(drf->po, stb->p);
          break;

        case 6:    // Collapse

          // Covers annihilation , light-matter
          // and scattering.

          drf->k = COLLAPSE;
          lst->k = COLLAPSE;
          drf->obj = drf->a;
          lst->obj = lst->a;
          break;

        case 7:    // Internal collision

          // Cohesion.
          // (Sect. 4.1)

          if (stb->k == FERMION && nxt->k == FERMION)
          {
            // Pole is the negation of p.

            CP(pole, drf->p);
            NEG(pole);
            CP(drf->po, pole);
            //
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
          }
          else if (stb->k == WB && nxt->k == WB)
          {
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
          }
          break;
      }
    }

    // Inter-sector interaction.
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion.

      if (nxt->k == FERMION && drf->k == SPHOTON)
      {
        CP(pole, drf->p);
        NEG(pole);
        CP(drf->po, pole);
      }
      else if (drf->k == FERMION && nxt->k == SPHOTON)
      {
        CP(pole, lst->p);
        NEG(pole);
        CP(lst->po, pole);
      }
    }
  }

  TICK: ;
}

