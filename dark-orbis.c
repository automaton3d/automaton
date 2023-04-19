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

  // Reissue?

  if (ZERO(drf->o))
  {
    drf->n   = 1;     // synchronize clocks
    drf->syn = 0;
    drf->obj = SIDE3; // unreachable

    // Sine wave
    // Eq. 4

    drf->den = 1;  // 2^0	TODO: check if den always > 0
    drf->pow = 1;  // y^0

    drf->u   = 0;  // always reset sine
    drf->pmf = 0;  // and PMF

    RSET(drf->o);
    RSET(drf->pP);
    RSET(drf->m);
    SAT(drf->po);	// saturate po (unreachable)
  }
  else
  {
    // Clock tick

    drf->n++;
  }

  // Lattice update

  stb->ch  = drf->ch;   // charges
  stb->a   = drf->a;    // affinity
  stb->n   = drf->n;    // ticks
  stb->syn = drf->syn;  // wf synch
  stb->u   = drf->u;    // sine
  stb->pmf = drf->pmf;  // sine PMF
  stb->f   = drf->f;    // flash
  stb->k   = drf->k;    // kind
  stb->r   = drf->r;    // random
  stb->obj = drf->obj;  // target
  CP(stb->p, drf->p);   // momentum
  CP(stb->s, drf->s);   // spin
  CP(stb->o, drf->o);   // bubble origin
  CP(stb->po, drf->po); // pole
  CP(stb->fo, drf->fo); // flash origin
  CP(stb->pP, drf->pP); // decay
  CP(stb->m, drf->m);   // messenger

  // Eq. 4

  stb->pow = drf->pow;
  stb->den = drf->den;

  pthread_mutex_unlock(&mutex);
}

void flash()
{
  // Flash greedy expansion

  if (!stb->f)
    goto EXPAND;

  // Clean flash

  drf->f = false;

  // Test if pole was found
  // stb->po is the pole inherited from the beginning of the flash.

  if (BUSY(stb) && ZERO(stb->po))
  {
    // Pole found: create new seed

    SAT(drf->po);
    RSET(drf->o);
  }

  // Physical time

  int t = stb->n / LIGHT;

  // Find new skewed addresses for update
  // (Sect. 4.2)

  int off;
  if (stb->off % 2 == 0)
    off = (stb->off + t) % SIDE3;
  else
    off = (stb->off - t) % SIDE3;
  Cell *nxt = stb - stb->off + off;
  Cell *lst = drf - drf->off + off;

  // A collapse forces all affine bubbles to reissue
  // nxt->a is the skewed cell affinity that the flash is exploring
  // stb->obj is the affinity inherited from the beginning of the flash
  // It only matters non trivial p.

  if (nxt->a == stb->obj && !ZERO(nxt->p))
  {
    // Reissue this particular bubble
    // collapse happens at c.p.

    RSET(lst->po);     // reissue from c.p.
    lst->obj = lst->a; // default
    RSET(lst->o);      // hallmark of reissue
  }
  else
  {
    // Bump sine generator
    // (Euler formula for sine)

    int tn = stb->n / LIGHT;  // skewed time
    if (stb->obj == stb->a && t > 0)
    {
      lst->u = nxt->u * (tn * tn - MOD2(stb->o)) / (tn * tn);
    }

    // Update the sine signal pmf (Eqn. 4)
    // Only the skewed cell is affected

    if (nxt->den < SIDE / 2) // eqv. infinity in the summation
    {
      lst->pow *= nxt->pow * nxt->pow; // y^(2n)
      lst->den <<= 1;                  // 2^n
      lst->pmf += (2 * tn + 1) * lst->pow / lst->den;
    }
  }

  // Spread the flash
  // Explore von Neumann directions

  int org[3];
  Cell* nei;
  for (int dir = 0; dir < 6; dir ++)
  {
    CP(org, stb->fo);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Backwards?

    if (abs(org[i]) < abs(stb->fo[i]))
      continue;

    // No conflict?

    if (tree3d(dir, org))
    {
      // Propagate information

      nei      = drf->ws[dir]; // branch address
      nei->f   = true;         // flash wavefront
      nei->obj = stb->obj;     // collapse targets
      nei->k   = stb->k;       // collapse type
      CP(nei->fo, org);        // update origin
      CP(nei->po, stb->po);    // value before shrink
      CP(nei->p, stb->p);      // bear momentum

      // Shrink pole as flash approaches re-emission point

      switch(dir)
      {
        case 0:
          if(nei->po[0] < 0)
            nei->po[0]++;
          break;
        case 1:
    	  if(nei->po[0] > 0)
            nei->po[0]--;
          break;
      case 2:
          if(nei->po[1] < 0)
            nei->po[1]++;
          break;
      case 3:
          if(nei->po[1] > 0)
            nei->po[1]--;
          break;
      case 4:
          if(nei->po[2] < 0)
            nei->po[2]++;
          break;
      case 5:
          if(nei->po[2] > 0)
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

  if (!BUSY(stb))
    goto UPDATE;

  // Wrapping?

  if(abs(stb->o[0]) == SIDE_2 && abs(stb->o[1]) == SIDE_2 && abs(stb->o[2]) == SIDE_2)
  {
    if(!ZERO(stb->p))
    {
      // Reissue from here

  	  RSET(drf->po);
      RSET(drf->o);
    }

    // Limit of expansion was hit

    goto UPDATE;
  }

  // Is wavefront synchronized?
  // (see Ref. [15])

  if (drf->n * drf->n <= stb->syn)
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

    // Ultimate cell?

    if (abs(org[0]) + abs(org[1]) + abs(org[2]) == 3 * SIDE_2)
    {
    	Cell *c = huntMomentum(latt0);
    	assert(c != NULL);

    	if(ZERO(stb->p))
    	{
    		continue;
    	}
    }

    // No conflict?

    if (tree3d(dir, org))
    {
      // Calculated branch

      nei = drf->ws[dir];

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
      nei->obj = stb->obj;
      CP(nei->po, stb->po);
      assert(!ISSAT(org));
      CP(nei->o, org);
      CP(nei->s, stb->s);
      CP(nei->pP, stb->pP);
      CP(nei->m, stb->m);
      RSET(nei->p);

      // Select momentum destination
      // (Sect. 3.1)

      int dot = abs(DOT(org, stb->s));
      if (dot >= dotMax)
      {
        dotMax = dot;
        dst = nei;
      }

      // Update noise

      nei->r = (nei->r << 3) ^ dir;

      // Set timing for spherical pattern (Section 3)

      nei->syn = LIGHT2 * MOD2(org);

      // Test wrapping

      if ((abs(org[0]) == SIDE_2 || abs(org[1]) == SIDE_2 || abs(org[2]) == SIDE_2) && !ZERO(stb->p))
      {
    	  dst = nei;
    	  break;
      }
    }
  }
  if (!ZERO(stb->p))
  {
    addCell(dst);	// debug
    assert(dst != NULL);

    // Propagate momentum

    assert(ZERO(dst->p));

    CP(dst->p, stb->p);

    // Abandon the origin cell.
    // Tracking information remains.

    SAT(drf->o);  // cell is now LATTICE
  }

  // Bubble left this cell

  drf->k = EMPTY;
  drf->a = 0;

  UPDATE: ;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Physical time

  int t = stb->n / LIGHT;

  // Is there a bubble in this cell?
  // Last tick?

  if (BUSY(stb) && stb->n % LIGHT == 0)
  {
    // Bubble track decay (Eqn. 6)

    if (!ZERO(drf->pP))
    {
      // Vector p shrinks

      drf->pP[0] -= drf->pP[0] / (2 * drf->n);
      drf->pP[1] -= drf->pP[1] / (2 * drf->n);
      drf->pP[2] -= drf->pP[2] / (2 * drf->n);

      // Disconnect empodion?
      // (Sec. 4.7.3)

      if (ZERO(drf->pP))
        drf->a = stb->off + 1;  // default value
    }
  }

  // Update lattice decay

  if (!ZERO(stb->pP) && t > 0)
  {
    // Lattice track decay (Eq. 6)

    drf->pP[0] -= drf->pP[0] * drf->pP[0] / (t * t);
    drf->pP[1] -= drf->pP[1] * drf->pP[1] / (t * t);
    drf->pP[2] -= drf->pP[2] * drf->pP[2] / (t * t);
    if (ZERO(drf->pP))
    {
      // Disconnect empodion
      // (Sect. 4.7.3)

      drf->a = stb->off + 1;
    }
  }

  // Find new skewed addresses for updates
  // (Sect. 4.2)

  t++;					// look ahead TODO
  int off;
  if (stb->off % 2 == 0)
    off = (stb->off + t) % SIDE3;
  else
    off = (stb->off - t) % SIDE3;
  Cell *nxt = stb - stb->off + off;
  Cell *lst = drf - drf->off + off;

  // Superposing bubbles?

  if (EQ(stb->o, nxt->o))
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

        // Calculate the common value for a
        // (Sect. 4.2)

        drf->a |= (nxt->a << ORDER);
        lst->a |= (stb->a << ORDER);
      }
      else
      {
        // Symmetry breaking after singularity
        // (Sect. 4.6.7)

        CP(drf->po, stb->p);
        CP(lst->po, nxt->p);
        RSET(drf->o);
        RSET(lst->o);
      }
    }

    // Clump bubbles together to form particle pair fragments
    // (Sect. 4.2)

    else if (stb->a != nxt->a && stb->k == FERMION && nxt->k == FERMION)
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
      else if (C(stb) == C(nxt) && (C(stb) == 0 || C(stb) == 7) &&
               Q(stb) != Q(nxt) && W0(stb) == W0(nxt) &&
               W0(stb) != W1(stb))
      {
        drf->k = NEUTRINO;
        lst->k = NEUTRINO;
      }
      else if (C(stb) == _C(nxt) &&
               (W0(stb) != W1(stb) || W0(nxt) != W1(stb)) && Q(stb) == !Q(nxt))
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
      drf->a = nxt->a;		// TODO: check mutiple case TODO: how to guarantee reciprocity?
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

  if (stb->n % LIGHT != 0)
    goto COPY;

  // Find new addresses for interaction
  // (Sect. 4.2)

  int t = stb->n / LIGHT;	// Physical time
  int off;
  if (stb->off % 2 == 0)
    off = (stb->off + t) % SIDE3;
  else
    off = (stb->off - t) % SIDE3;
  Cell *nxt = stb - stb->off + off;
  Cell *lst = drf - drf->off + off;

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

          else if (BUSY(nxt) && drf->k == FERMION && !ZERO(nxt->p))
          {
            if (stb->r % 2 == 0)
            {
              // Electric radial force

              if (Q(nxt) == Q(stb))
              {
                // Repulsion
                // drf is now a messenger
                // (Sect. 5.6.1)

                SUB(lst->m, drf->o, nxt->o);
              }
              else
              {
                // Attraction
                // drf is now a messenger
                // (Sect. 5.6.1)

                SUB(lst->m, nxt->o, drf->o);
              }
            }
            if (nxt->r % 2 == 0)
            {
              // Magnetic lateral kick
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
                RSET(drf->o);	// drf->emit = CONTACT;
                RSET(lst->o);	// lst->emit = CONTACT;
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
                      RSET(drf->o);	// drf->emit = CONTACT;
                      RSET(lst->o);	// lst->emit = CONTACT;
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
                      RSET(drf->o);	// drf->emit = CONTACT;
                      RSET(lst->o);	// lst->emit = CONTACT;
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
                RSET(drf->o);	// drf->emit = CONTACT;
                RSET(lst->o);	// lst->emit = CONTACT;
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
                      RSET(drf->o);	// drf->emit = CONTACT;
                      RSET(lst->o);	// lst->emit = CONTACT;
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
                      RSET(drf->o);	// drf->emit = CONTACT;
                      RSET(lst->o);	// lst->emit = CONTACT;
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
            RSET(drf->o);

            // Reciprocity

            CP(lst->po, nxt->p);
            CP(lst->s, stb->s);
            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
            RSET(lst->o);
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->po, nxt->o, drf->o);
          RSET(drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

          CP(drf->po, stb->p);
          RSET(drf->o);
          break;

        case 6:    // Collapse

          // fermion+ x fermion- (annihilation) ?
          // fermion- x fermion+ (annihilation) ?

          if (CMPL(nxt->ch, drf->ch) && nxt->k == FERMION && stb->k == FERMION)
          {
            // Destroy particles, setting each 'a' property to
            // its default value

            drf->obj = drf->a;
            drf->a = drf->off + 1;
            lst->obj = lst->a;
            lst->a = lst->off + 1;
          }
          else if (nxt->k == FERMION && drf->k != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->obj = drf->a;
            drf->a = drf->off + 1;
            lst->obj = lst->a;
            lst->a = lst->off + 1;
          }
          else if (nxt->k != FERMION && drf->k == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            lst->a = stb->a | (nxt->a << ORDER);
            drf->obj = drf->a;
            lst->obj = lst->a;
            lst->k = FERMION;      // separate fragments
          }
          else if (nxt->k == NEUTRINO && drf->k == FERMION)
          {
            CP(drf->po, nxt->o); // enough?!
          }
          else if (stb->k == NEUTRINO && lst->k == FERMION)
          {
            CP(lst->po, stb->o);
          }
          else
          {
            // fermion+ x fermion+ (scattering)
            // fermion- x fermion- (scattering)

            drf->obj = drf->a;
            lst->obj = lst->a;
          }

          // All options in this case reissue from cp

          CP(drf->po, stb->p);
          RSET(drf->o);
          drf->obj = nxt->a;   // non trivial target
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
            RSET(drf->o);
            //
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
            RSET(lst->o);
          }
          else if (stb->k == WB && nxt->k == WB)
          {
            int pole[3];
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
            RSET(lst->o);
            RSET(drf->o);
            RSET(lst->o);
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            int pole[3];
            CP(pole, lst->p);
            NEG(pole);
            CP(lst->po, pole);
            RSET(lst->o);
            RSET(drf->o);
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
        RSET(drf->o);
      }
      else if (drf->k == FERMION && nxt->k == SPHOTON)
      {
        int pole[3];
        CP(pole, lst->p);
        NEG(pole);
        CP(lst->po, pole);
        RSET(drf->o);
      }
    }
  }

  // Always emit flash from pole at each light pass

  if(t % LIGHT == 0 && !ZERO(stb->p))
  {
    drf->f = true;
    RSET(drf->fo);
    if(!ZERO(drf->o))
 	  RSET(drf->p);  // bump all affine cells
  }

  COPY: ;
}
