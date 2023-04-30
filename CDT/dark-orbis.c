///////////////////////////////
//                           //
// -- Universe's software -- //
//                           //
///////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include "simulation.h"
#include "utils.h"
#include "plot3d.h"
#include "test/test.h"

Cell *latt0, *latt1;

// Current pointers

Cell *stb, *drf;

extern pthread_mutex_t mutex;

/**
 * Copy data.
 */
void copy()
{
  pthread_mutex_lock(&mutex);

  // Clock tick

  drf->n++;  // last draft update before copy itself

  // Lattice update

  stb->f   = drf->f;    // flash
  stb->ch  = drf->ch;   // charges
  stb->a   = drf->a;    // affinity
  stb->n   = drf->n;    // ticks
  stb->syn = drf->syn;  // wf synch
  stb->u   = drf->u;    // sine
  stb->pmf = drf->pmf;  // sine PMF
  stb->k   = drf->k;    // kind
  stb->r   = drf->r;    // random
  stb->occ = drf->occ;  // occupancy
  stb->obj = drf->obj;  // target
  CP(stb->p, drf->p);   // momentum
  CP(stb->s, drf->s);   // spin
  CP(stb->o, drf->o);   // bubble origin
  CP(stb->po, drf->po); // pole
  CP(stb->pP, drf->pP); // decay
  CP(stb->m, drf->m);   // messenger

  // Eq. 4

  stb->pow = drf->pow;
  stb->den = drf->den;

  pthread_mutex_unlock(&mutex);
}

/*
 * Flash greedy expansion.
 * Only flash() can reset vector o.
 */
void flash()
{
  // --- Operations inside separate space ---

  // Decay bubble occupancy

  if(stb->occ > 0)
    drf->occ--;

  // Decay flash occupancy

  if(stb->f > 0)
    drf->f--;
  if (stb->f != LIGHT)
    goto EXPAND;

  // Test if pole was found.
  // stb->po is inherited pole from beginning of flash.
  // Valid for the momentum carrying cell only.

  boolean stop = false;
  if (BUSY(stb) && ZERO(stb->po) && !ZERO(stb->p))
  {
    RSET(drf->o);  // will be used in [CODE 1]
    stop = true;
  }
  else
  {
    RSET(drf->p);  // p will move until po == 0
  }

  // Always let searched pole unreachable.
  // When effectively reissued, give proper value.

  SAT(drf->po);

  // --- End of ops. inside separate space ---

  // Calculate physical time.

  int t = stb->n / LIGHT;

  // --- Beginning of affine operations ---

  // Find new skewed addresses for updates.
  // (Sect. 4.2)

  int off;
  if (stb->offE % 2 == 0)
    off = (stb->offE + (t + 1)) % SIDE3;
  else
    off = (stb->offE - (t + 1)) % SIDE3;
  Cell *nxt = stb - stb->offE + off;
  Cell *lst = drf - drf->offE + off;

  // A collapse forces all affine bubbles to reissue.
  // nxt->a is the skewed cell affinity that the flash
  // is exploring.
  // stb->obj is the affinity inherited from the
  // beginning of flash.
  // It only matters non trivial p.

  if (nxt->a == stb->obj && !ZERO(nxt->p))
  {
    // Reissue this particular bubble.
    // Collapse happens at c.p.

    RSET(lst->po);     // reissue from c.p.
    lst->obj = lst->a; // set default
    RSET(lst->o);      // ok, inside flash()

    // New new new!!

    if (stb->k == COLLAPSE)
      lst->k = FERMION;	// dissolve the particle
  }
  else
  {
    // Bump sine generator.
    // (Euler formula for sine)

    int tn = stb->n / LIGHT;  // skewed time
    if (stb->obj == stb->a && tn > 0)
    {
      lst->u = nxt->u * (tn * tn - MOD2(stb->o)) / (tn * tn);
    }

    // Update the sine signal pmf (Eqn. 4).
    // Only the skewed cell is affected.

    if (nxt->den < SIDE / 2) // eqv. to infinity in summation
    {
      lst->pow *= nxt->pow * nxt->pow; // y^(2n)
      lst->den <<= 1;                  // 2^n
      lst->pmf += (2 * tn + 1) * lst->pow / lst->den;
    }
  }

  // Pair homogenization.

  if (stb->k > FERMION && EQ(stb->o, nxt->o) && stb->a == nxt->a)
  {
    // Unilateral updates are extended to peer. TODO

    puts("homogenization");
  }

  // --- End of affine operations ---

  // Spread the flash:
  // explore von Neumann directions.

  Cell* nei;
  for (int dir = 0; dir < 6; dir ++)
  {
    // Propagate information

    nei = drf->ws[dir];       // branch address

    // Collision avoidance

    if (nei->f == 0)
    {
      // Not tainted by flash already, proceed

      nei->f   = LIGHT;     // flash wavefront
      nei->obj = stb->obj;  // collapse targets
      CP(nei->po, stb->po); // value before shrink

      if(stop)
      {
        // Do not propagate anymore.

        RSET(nei->p);
      }
      else
      {
        // Pole not found yet, propagate p.

        CP(nei->p, drf->p);   // bear momentum drf->p (sic)
      }

      // Do not touch pole if saturated.

      if (ISSAT(nei->po))
        continue;

      // Shrink pole as flash approaches re-emission point.

      switch(dir)
      {
        case 0:
          if (nei->po[0] < 0)
            nei->po[0]++;
          break;
        case 1:
          if (nei->po[0] > 0)
            nei->po[0]--;
          break;
        case 2:
          if (nei->po[1] < 0)
            nei->po[1]++;
          break;
        case 3:
          if (nei->po[1] > 0)
            nei->po[1]--;
          break;
        case 4:
          if (nei->po[2] < 0)
            nei->po[2]++;
          break;
        case 5:
          if (nei->po[2] > 0)
            nei->po[2]--;
          break;
      }
    }
  }

  EXPAND: ;
}

/**
 * Expands wavefront.
 */
void expand()
{
  // Empty?

  if (!BUSY(stb)) goto UPDATE;

  // Reissue?
  // Only the cell bearing momentum is a seed.
  // Vector 'o' was reset by the flash in the
  // previous tick. [CODE 1]

  if (!ZERO(stb->p) && ZERO(stb->o))
  {
    drf->syn = 0;       // ready for immediate expansion
    drf->n   = 0;       // synchronize clocks
    drf->obj = SIDE3;   // no target in view

    // Reset sine wave.
    // Sect. 6.3 and (Eq. 2).

    drf->u   = 0;       // always reset sine
    drf->pmf = 0;       // and PMF
    drf->den = 1;       // 2^0
    drf->pow = 1;       // y^0

    MILD(drf->po);      // leave po wrappable
    RSET(drf->m);       // TODO: redundant?
    if (stb->k == COLLAPSE)
    {
    	// Particle dissolution.

        drf->k = FERMION;
        drf->a = drf->offE + 1;
        RSET(drf->pP);
    }
  }

  // End of expansion was hit?

  if (ISMILD(stb->o))
  {
      // Is cell bearing momentum?

	  if(!ZERO(stb->p))
	  {
        RSET(drf->po);  // reissue from here
        drf->f = LIGHT;
        drf->a = stb->offE + 1;
	  }
	  goto UPDATE;
  }

  // Is wavefront synchronized?
  // (see Ref. [14])

  if (stb->n * stb->n <= stb->syn)
    goto UPDATE;

  // Explore von Neumann neighborhood

  int org[3];
  Cell* nei;
  int dotMax = 0;
  Cell *dst = NULL;
  for (int dir = 0; dir < 6; dir++)
  {
    // Initialize new origin vector

    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Backwards?

    if (abs(org[i]) < abs(stb->o[i]))
      continue;

    // Test wrapping.
    // At this position, only the cell carrying
    // momentum is allowed to enter.

    if ((abs(org[0]) == SIDE_2 ||
         abs(org[1]) == SIDE_2 ||
         abs(org[2]) == SIDE_2) && ZERO(stb->p))
      continue;

    // Wrapping violation?

    if (abs(org[0]) == SIDE_2 + 1 ||
        abs(org[1]) == SIDE_2 + 1 ||
        abs(org[2]) == SIDE_2 + 1)
      continue;

    // Calculate branch.

    nei = drf->ws[dir];

    // Calculate alignment with spin.

    int dot = abs(DOT(org, stb->s));

    // Select momentum destination.
    // (Sect. 3.1)

    if (dot >= dotMax)
    {
      dotMax = dot;
      dst = nei;
    }

    // Test occupancy.

    if (nei->occ == 0)
    {
      // Flag as occupied

      nei->occ   = LIGHT;

      // Propagate information

      nei->n   = stb->n;
      nei->a   = stb->a;
      nei->ch  = stb->ch;
      nei->u   = stb->u;
      nei->pmf = stb->pmf;

      // Eq. 4

      nei->pow = stb->pow;
      nei->den = stb->den;

      nei->k   = stb->k;
      nei->obj = SIDE3; // disable affinity search
      MILD(nei->po);
      CP(nei->o, org);
      CP(nei->s, stb->s);
      CP(nei->pP, stb->pP);
      CP(nei->m, stb->m);
      RSET(nei->p);     // default null value

      // Update noise
      // (Sect. 3.3)

      nei->r = (nei->r << 3) ^ dir;

      // Set timing for spherical pattern (Sect. 3)

      nei->syn = LIGHT2 * MOD2(org);
    }
  }

  // Cell does not have momentum?

  if (ZERO(stb->p))
  {
    // No tracking information left

    drf->a = 0;
    SAT(drf->o);
  }
  else
  {
    // Propagate momentum

    CP(dst->p, stb->p);

    // Tracking information remains (a, o).
  }

  // Abandon this cell

  drf->k = EMPTY;
  RSET(drf->s);

  UPDATE: ;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Physical time

  int t = stb->n / LIGHT;

  // Bubble track decay (Eqn. 6):
  // Is there a bubble in this cell?
  // Last tick?

  if (BUSY(stb) && stb->n % LIGHT == 0 && !ZERO(stb->pP))
  {
    // Cell belongs to a particle empodion.

    // Vector p shrinks (Eq. 6).

    drf->pP[0] -= drf->pP[0] / (2 * t);
    drf->pP[1] -= drf->pP[1] / (2 * t);
    drf->pP[2] -= drf->pP[2] / (2 * t);

    // Disconnect empodion?
    // (Sec. 4.7.3)

    if (ZERO(drf->pP))
      drf->a = stb->offE + 1;  // default value

    // TODO: check pair homogenization
  }

  // Update lattice decay
  // A saturated o vector means lattice role

  else if (ISSAT(stb->o) && !ZERO(stb->pP) && t > 0)
  {
    // Lattice track decay (Eq. 5).

    drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
    drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
    drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);

    // Disconnect empodion?
    // (Sect. 4.7.3)

    if (ZERO(drf->pP))
      drf->a = stb->offE + 1; // default
  }

  // Find new skewed addresses for updates
  // (Sect. 4.2)

  int off;
  if (stb->offE % 2 == 0)
    off = (stb->offE + (t + 1)) % SIDE3;
  else
    off = (stb->offE - (t + 1)) % SIDE3;
  Cell *nxt = stb - stb->offE + off;
  Cell *lst = drf - drf->offE + off;

  // Superposing bubbles?

  if (stb->offL == nxt->offL && ZERO(stb->o) && ZERO(nxt->o))
  {
    // Check if different sectors
    // (Sec. 4.7.7)

    if (W1(stb) != W1(nxt))
    {
      // Super photon formation
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
        drf->k = FOWL;
        lst->k = FOWL;
      }
    }

    // Clump bubbles together to form particle pair fragments
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
        goto INTERACT;
      }

      // Calculate the common value for affinity
      // (Sect. 4.2)

      drf->a = stb->a | (nxt->a << ORDER);
      lst->a = drf->a;
    }

    // Combine pairs to form multi-pairs
    // (Sect. 5.2)

    else if (stb->k == nxt->k && stb->k > FERMION &&
             stb->a != nxt->a)
    {
      drf->a = nxt->a; // TODO: check mutiple case
                       // TODO: how to guarantee reciprocity?
      lst->a = drf->a;
    }
  }

  INTERACT: ;
}

/**
 * Detects interactions.
 */
void interact()
{
  // Not the last pass of wavefront?

  if (stb->n % LIGHT != 0) goto COPY;

  // Symmetry breaking

  if (stb->k == FOWL) goto REISSUE;

  // Find new addresses for interaction
  // (Sect. 4.2)

  int t = stb->n / LIGHT;	// Physical time
  int off;
  if (stb->offE % 2 == 0)
    off = (stb->offE + t) % SIDE3;
  else
    off = (stb->offE - t) % SIDE3;
  Cell *nxt = stb - stb->offE + off;
  Cell *lst = drf - drf->offE + off;

  // Internal process?

  if (stb->a == nxt->a && !ZERO(stb->p) && !ZERO(nxt->p))
  {
    // Self interference (Sect. 5.6.3)
    // (Sciarretta)

    int dif[3];
    SUB(dif, stb->o, nxt->o);

    // From the same region?
    // (Eq. 7, Eq. 8)

    if (DOT(nxt->o, stb->o) == 1 && MOD2(dif) == 0)
    {
      // Induce empodion (Sect. 5.6.3)

      if (nxt->k == FERMION && ISSAT(stb->o))
      {
        CP(lst->m, drf->pP);
        goto COPY;
      }

      // Reciprocity

      else if (ISSAT(nxt->o) && stb->k == FERMION)
      {
        CP(drf->m, nxt->pP);
        goto COPY;
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3)

      if (nxt->k == FERMION && ISSAT(stb->o))
      {
        CP(lst->o, drf->o);
        goto COPY;
      }

      // Reciprocity

      else if (ISSAT(nxt->o) && stb->k == FERMION)
      {
        CP(drf->o, nxt->o);
        goto COPY;
      }
    }
  }

  // Play pseudo dice to decide bubble mixing

  else if (stb->r < drf->pmf)
  {
    // Figure out the expected kind of interaction

    int code = (nxt->a == drf->a) | (!ZERO(nxt->p) << 1) |
	           (!ZERO(drf->p) << 2);

    // Test for same or different sectors

    if (W1(nxt) == W1(drf))
    {
      // Same-sector

      switch(code)
      {
        case 1:    // Soft interactions

          // Phase only interaction
          // (just a template for now)

          break;

        case 2:    // Rendez vous (different particles)
        case 4:    // Rendez vous (different particles)

          // Virtual photon capture
          // Part of electrical, magnetic or
          // interference interactions

          if (drf->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Draft is now a propeller

            drf->a = nxt->a;
            RSET(lst->m);
          }
          else if (lst->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Last is now a propeller

            lst->a = stb->a;
            RSET(drf->m);
          }

          // Static forces

          else if (BUSY(nxt) && drf->k == FERMION &&
                   !ZERO(nxt->p))
          {
            if (stb->r % 2 == 0)
            {
              // Electric radial force

              if (Q(nxt) == Q(stb))
              {
                // Repulsion.
                // drf is now a messenger
                // (Sect. 5.6.1)

                SUB(lst->m, drf->o, nxt->o);
              }
              else
              {
                // Attraction.
                // drf is now a messenger
                // (Sect. 5.6.1)

                SUB(lst->m, nxt->o, drf->o);
              }
            }
            if (nxt->r % 2 == 0)
            {
              // Magnetic lateral kick.
              // drf is now a messenger
              // (Sect. 5.6.2)

              CROSS(lst->m, lst->m, nxt->s);
            }
          }

          // fermion- x boson (local light-matter)
          // fermion+ x boson (local light-matter)

          else if (lst->k == FERMION && drf->k > FERMION)
          {
            // boson x fermion

            switch(drf->k)
            {
              case PHOTON:
                CP(drf->po, stb->p);
                CP(lst->po, nxt->p);
                break;

              case ZB:
              case WB:
                if (MAT(nxt))
                {
                  if (W0(nxt) == LEFT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    {
                      CP(drf->po, stb->p);
                      CP(lst->po, nxt->p);
                    }
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    {
                      CP(drf->po, stb->p);
                      CP(lst->po, nxt->p);
                    }
                  }
                }
                break;
            }
          }
          else if (nxt->k > FERMION && drf->k == FERMION)
          {
            // fermion x boson

            switch(nxt->k)
            {
              case PHOTON:
                CP(drf->po, stb->p);
                CP(lst->po, nxt->p);
                break;

              case ZB:
              case WB:
                if (MAT(nxt))
                {
                  if (W0(nxt) == LEFT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                       (!MAT(drf) && W0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      CP(drf->po, stb->p);
                      CP(lst->po, nxt->p);
                    }
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    {
                      CP(drf->po, stb->p);
                      CP(lst->po, nxt->p);
                    }
                  }
                }
                break;
            }
          }

          // boson x boson

          else if (lst->k > FERMION && drf->k > FERMION)
          {
            // Exchange charges and spins

            CP(drf->s, nxt->s);
            drf->ch &= (C_MASK | W0_MASK);
            drf->ch |= (nxt->ch & (C_MASK | W0_MASK));
            CP(drf->po, stb->p);

            // Reciprocity

            CP(lst->po, nxt->p);
            CP(lst->s, stb->s);
            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->po, nxt->o, drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

          CP(drf->po, stb->p);
          break;

        case 6:    // Collapse

          // Covers annihilation , light-matter
          // and scattering.

          drf->k = COLLAPSE;
          lst->k = COLLAPSE;
          CP(drf->po, stb->p);
          drf->obj = drf->a;
          lst->obj = lst->a;
          break;

        case 7:    // Internal collision

          // Cohesion
          // (Sect. 4.1)

          if (stb->k == FERMION && nxt->k == FERMION)
          {
            // Pole is the negation of p

            int pole[3];
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
            int pole[3];
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            int pole[3];
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
          }
          break;
      }
    }

    // Inter-sector interaction
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion

      if (nxt->k == FERMION && drf->k == SPHOTON)
      {
        int pole[3];
        CP(pole, drf->p);
        NEG(pole);
        CP(drf->po, pole);
      }
      else if (drf->k == FERMION && nxt->k == SPHOTON)
      {
        int pole[3];
        CP(pole, lst->p);
        NEG(pole);
        CP(lst->po, pole);
      }
    }
  }

  // Always emit flash from pole at each light pass

  REISSUE:

  if (!ZERO(stb->p))
  {
    drf->f = LIGHT;
    if (!ZERO(drf->o))
 	  RSET(drf->p);  // bump all affine cells
  }

  COPY: ;
}
