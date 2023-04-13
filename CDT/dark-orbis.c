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

  if (stb->emit)
  {
    // Common updates

    drf->n = 0;
    drf->syn  = 0;
    drf->obj  = SIDE3;

    // Eq. 4

    drf->den  = 1;      // 2^0	TODO: check if den always > 0
    drf->pow  = 1;      // y^0

    CP(drf->p, stb->p);	// patch 18/3
    RSET(drf->o);
    RSET(drf->pP);
    RSET(drf->m);
    RSET(drf->po);

    // Specific updates

    if (drf->emit == CONTACT)
    {
      // Pole is the contact point

      RSET(drf->po);
      RSET(drf->o);
    }
    else if (drf->emit == POLE)
    {
      // Pole is the negation of p

      int pole[3];
      CP(pole, drf->p);
      NEG(pole);
      CP(drf->po, pole);
      RSET(drf->o);
    }

    // Done

    drf->emit = NONE;
  }
  else
  {
    // Clock tick

    drf->n++;
  }

  // Lattice update

  stb->ch  = drf->ch;       // charges
  stb->a   = drf->a;        // affinity
  stb->n   = drf->n;        // ticks
  stb->syn = drf->syn;      // wf synch
  stb->u   = drf->u;        // sine
  stb->pmf = drf->pmf;      // sine PMF
  stb->f   = drf->f;        // flash
  stb->k   = drf->k;        // kind
  stb->r   = drf->r;        // random
  stb->obj = drf->obj;      // target
  CP(stb->p, drf->p);       // momentum
  CP(stb->s, drf->s);       // spin
  CP(stb->o, drf->o);       // bubble origin
  CP(stb->po, drf->po);     // pole
  CP(stb->fo, drf->fo);     // flash origin
  CP(stb->pP, drf->pP);     // decay
  CP(stb->m, drf->m);       // messenger

  // Eq. 4

  stb->pow = drf->pow;
  stb->den = drf->den;

  pthread_mutex_unlock(&mutex);
}

/**
 * Spreads flash.
 */
void flash()
{
  // Flash greedy expansion

  if (!stb->f)
    goto EXPAND;

  // Clean flash

  drf->f = false;

  // Test if pole was found

  if (!ZERO(stb->p) && ZERO(stb->po) && BUSY(stb) && !ZERO(stb->o))
  {
    // Pole found: create new seed

    drf->emit = IMMEDIATE;
    drf->obj = stb->a;  // non trivial target
    goto EXPAND;
  }

  // A collapse forces all affine bubbles to reissue

  else if (stb->a == stb->obj)
  {
    // Reissue

    drf->emit = IMMEDIATE;
    drf->obj = stb->a;  // non trivial target
    goto EXPAND;
  }

  // Explore von Neumann directions

  int org[3];
  Cell* nei;
  for (int dir = 0; dir < 6; dir ++)
  {
    CP(org, stb->fo);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

      // Propagate information

    nei = drf->ws[dir];
    if(!nei->f)
    {
      // Propagate flash

      nei->f = true;
      CP(nei->fo, org);

      // Propagate sought affinity for collapse

      nei->obj = stb->obj;

      if (!ZERO(stb->po))
      {
        // Shrink pole as flash approaches re-emission point

        nei->po[0] = stb->po[0] - (dir < 2);
        nei->po[1] = stb->po[1] - (dir % 2 == 1);
        nei->po[2] = stb->po[2] - (dir > 3);
      }
    }
  }

  EXPAND: ;
}

void flashxxxx()
{
  // Flash greedy expansion

  if (!stb->f)
    goto EXPAND;

  // Clean flash

  drf->f = false;

  // Test if pole was found

  if (!ZERO(stb->p) && ZERO(stb->po) && BUSY(stb) && !ZERO(stb->o))
  {
    // Pole found: create new seed

    drf->emit = IMMEDIATE;
    drf->obj = stb->a;  // non trivial target
    goto EXPAND;
  }

  // A collapse forces all affine bubbles to reissue

  else if (stb->a == stb->obj)
  {
    // Reissue

    drf->emit = IMMEDIATE;
    drf->obj = stb->a;  // non trivial target
    goto EXPAND;
  }

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

    if (isAllowed(dir, org))
    {
      // Propagate information

      nei = drf->ws[dir];

      // Propagate flash

      nei->f = true;
      CP(nei->fo, org);

      // Propagate sought affinity for collapse

      nei->obj = stb->obj;

      if (!ZERO(stb->po))
      {
        // Shrink pole as flash approaches re-emission point

        nei->po[0] = stb->po[0] - (dir < 2);
        nei->po[1] = stb->po[1] - (dir % 2 == 1);
        nei->po[2] = stb->po[2] - (dir > 3);
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
  // Reemission?

  if (drf->emit)
    goto UPDATE;

  // Empty?

  if (!BUSY(stb))
    goto UPDATE;

  // Wrapping?

  if (abs(stb->o[0]) == SIDE - 2 &&
      abs(stb->o[1]) == SIDE - 2 &&
      abs(stb->o[2]) == SIDE - 2)
  {
    assert(!ZERO(stb->p));
    drf->emit = IMMEDIATE;
    goto UPDATE;
  }

  // Is wavefront synchronized? (see Ref. [15])

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

    // No conflict?

    if (isAllowed(dir, org))
    {
      // Propagate information

      nei      = drf->ws[dir];
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
      CP(nei->o, org);
      CP(nei->s, stb->s);
      CP(nei->pP, stb->pP);
      CP(nei->m, stb->m);
      RSET(nei->p);

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
    }
  }
  if (!ZERO(stb->p))
  {
    // Propagate momentum

    CP(dst->p, stb->p);

    // Abandon the origin cell.
    // Tracking information remains.

    drf->k = LATTICE;
  }
  else
  {
    drf->k = EMPTY;
  }

  UPDATE: ;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Wrapping?

  if (drf->emit)
    goto INTERACT;

  // Physical time

  int t = stb->n / LIGHT;

  // Is there a bubble in this cell?
  // Last tick?

  if (BUSY(stb) && stb->n % LIGHT == 0)
  {
    // Bump sine generator
    // (Euler formula for sine)

    if (stb->obj == stb->a && t > 0)
      drf->u = stb->u * (t * t - MOD2(stb->o)) / (t * t);

    // Bubble track decay (Eqn. 6)

    if (!ZERO(drf->pP))
    {
      // Vector p shrinks

      drf->pP[0] -= drf->pP[0] / (2 * drf->n);
      drf->pP[1] -= drf->pP[1] / (2 * drf->n);
      drf->pP[2] -= drf->pP[2] / (2 * drf->n);
      if (ZERO(drf->pP))
      {
        // Sec. 4.7.3 - Disconnect empodion

        drf->a ^= stb->r;
      }
    }

    // Update the sine signal pmf (Eqn. 4)

    if (stb->den < SIDE / 2)  // eqv. infinity in the summation
    {
      drf->pow *= stb->pow * stb->pow;  // y^(2n)
      drf->den <<= 1;		// 2^n
      drf->pmf += (2 * t + 1) * drf->pow / drf->den;
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

      drf->a ^= stb->r;
    }
  }

  // Find new address for interaction
  // (Sect. 4.2)

  t++;					// look ahead
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

        drf->a ^= nxt->a;
        lst->a ^= stb->a;
      }
      else
      {
        // Symmetry breaking after singularity
        // (Sect. 4.6.7)

        drf->emit = POLE;
        lst->emit = POLE;
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

      drf->a = stb->a ^ nxt->a;
      lst->a = drf->a;
    }

    // Combine pairs to form multi-pairs
    // (Sect. )

    else if (stb->k == nxt->k && stb->k > FERMION &&
             stb->a != nxt->a)
    {
      drf->a ^= nxt->a;		// TODO: check mutiple case
    }
  }

  INTERACT: ;
}

/**
 * Detects interactions.
 */
void interact()
{
  // Wrapping?

  if (drf->emit)
    goto COPY;

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

      if (nxt->k == FERMION && stb->k == LATTICE)
      {
    	  CP(lst->m, drf->pP);
          goto COPY;
      }

      // Reciprocity

      else if (nxt->k == LATTICE && stb->k == FERMION)
      {
    	  CP(drf->m, nxt->pP);
          goto COPY;
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3)

      if (nxt->k == FERMION && stb->k == LATTICE)
      {
        CP(lst->o, drf->o);
        goto COPY;
      }

      // Reciprocity

      else if (nxt->k == LATTICE && stb->k == FERMION)
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

    // Initialize collapse target

    unsigned target = SIDE3;

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
                drf->emit = CONTACT;
                lst->emit = CONTACT;
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
                    	drf->emit = CONTACT;
                        lst->emit = CONTACT;
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
                    	drf->emit = CONTACT;
                        lst->emit = CONTACT;
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
            	  drf->emit = CONTACT;
                  lst->emit = CONTACT;
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

                    	drf->emit = CONTACT;
                        lst->emit = CONTACT;
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
                    	drf->emit = CONTACT;
                        lst->emit = CONTACT;
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
            drf->emit = CONTACT;

            // Reciprocity

            CP(lst->s, stb->s);
            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
            lst->emit = CONTACT;
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->po, nxt->o, drf->o);
          RSET(drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

        	drf->emit = CONTACT;
          break;

        case 6:    // Collapse

          // fermion+ x fermion- (annihilation) ?
          // fermion- x fermion+ (annihilation) ?

          if (CMPL(nxt->ch, drf->ch) && nxt->k == FERMION && stb->k == FERMION)
          {
            // Destroy particles, setting each 'a' property to a
            // new, pseudorandom value

            drf->a ^= nxt->r;
            drf->obj = drf->a;
            lst->obj = lst->a;
            target = drf->obj;  // copy for flash
            drf->k = FERMION;      // separate fragments
          }
          else if (nxt->k == FERMION && drf->k != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= nxt->r;
            drf->obj = drf->a;
            lst->obj = lst->a;
            target = drf->obj;  // copy for flash
            drf->k = FERMION;      // separate fragments
          }
          else if (nxt->k != FERMION && drf->k == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            lst->a ^= stb->r;
            drf->obj = drf->a;
            lst->obj = lst->a;
            target = lst->obj;  // copy for flash
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
            target = drf->obj; // copy for flash
          }

          // All options in this case reissue from cp

          drf->emit = CONTACT;
          drf->obj = nxt->a;   // non trivial target
          break;

        case 7:    // Internal collision

          // Cohesion
          // (Sect. 4.1)

          if (stb->k == FERMION && nxt->k == FERMION)
          {
        	drf->emit = POLE;
        	lst->emit = POLE;
          }
          else if (stb->k == WB && nxt->k == WB)
          {
            drf->emit = POLE;
            lst->emit = POLE;
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            drf->emit = POLE;
            lst->emit = POLE;
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
    	  drf->emit = CONTACT;
      else if (drf->k == FERMION && nxt->k == SPHOTON)
    	  drf->emit = CONTACT;
    }

    // Define collapse target

    drf->obj = target;
  }

  // Automatically release a flash

  drf->f = true;

  COPY: ;
}

