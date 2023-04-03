///////////////////////////////
//                           //
// -- Universe's software -- //
//                           //
///////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
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
  stb->ch     = drf->ch;
  stb->a      = drf->a;
  stb->t      = drf->t;
  stb->syn    = drf->syn;
  stb->u      = drf->u;
  stb->pmf    = drf->pmf;

  // Eq. 4

  stb->pow    = drf->pow;
  stb->den    = drf->den;

  stb->f      = drf->f;
  stb->bmp    = drf->bmp;
  stb->flash  = drf->flash;
  stb->kind   = drf->kind;
  stb->seed   = drf->seed;
  stb->target = drf->target;
  stb->offset = drf->offset;
  CP(stb->p, drf->p);
  CP(stb->s, drf->s);
  CP(stb->o, drf->o);
  CP(stb->pole, drf->pole);
  CP(stb->dom, drf->dom);
  CP(stb->pP, drf->pP);
  CP(stb->msg, drf->msg);
  pthread_mutex_unlock(&mutex);
}

/**
 * Spreads flash.
 */
void flash()
{
  // Flash greedy expansion

  if (!stb->flash)
	  return;

  // Clean flash

  drf->flash = false;

  // Test if pole was found

  boolean isPole = false;
  if (!ZERO(stb->p) && ZERO(stb->pole) && stb->f > 0 && ZERO(stb->o))
  {
    // Pole found: create new seed

    RESET(drf->pole);
    RESET(drf->o);
    drf->target = stb->a;  // non trivial target
    isPole = true;
  }

  // A collapse forces all affine bubbles to reissue

  else if (stb->a == stb->target)
  {
    // Reissue

    RESET(drf->pole);
    RESET(drf->o);
    drf->target = stb->a;   // non trivial target
    isPole = true;
  }

  // Explore von Neumann directions

  int org[3];
  Cell* nei;
  for (int dir = 0; dir < 6; dir ++)
  {
    CP(org, stb->dom);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

	// Backwards?

	if (abs(org[i]) < abs(stb->dom[i]))
	  continue;

    // No conflict?

    if (isAllowed(dir, org))
    {
      // Propagate information

      nei = drf->wires[dir];

      // Propagate flash

      nei->flash = true;
      CP(nei->dom, org);

      // Propagate sought affinity for collapse

      nei->target = stb->target;

      if (!ZERO(stb->pole) && !isPole)
      {
        // Shrink pole as flash approaches re-emission point

        nei->pole[0] = stb->pole[0] - (dir < 2);
        nei->pole[1] = stb->pole[1] - (dir % 2 == 1);
        nei->pole[2] = stb->pole[2] - (dir > 3);
      }
    }
  }
}

/**
 * Expands wavefront.
 */
void expand()
{
  // Empty?

  if (stb->f == 0)
    return;

  drf->t++;

  if(abs(stb->o[0]) == SIDE - 2 && abs(stb->o[1]) == SIDE - 2 && abs(stb->o[2]) == SIDE - 2)
  {
    drf->t      = 0;
    drf->syn    = 0;
    drf->target = SIDE3;

    // Eq. 4

    drf->den    = 1;		// 2^0
    drf->pow    = 1;		// y^0

    CP(drf->p, stb->p);	// patch 18/3
    RESET(drf->o);

    // TODO: check these

    RESET(drf->pP);
    RESET(drf->msg);
    RESET(drf->pole);

    return;
  }

  // Is wavefront synchronized? (see Ref. [15])

  if (drf->t * drf->t <= stb->syn)
    return;

  // Explore von Neumann directions

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

      nei         = drf->wires[dir];
	  nei->t      = drf->t;  // copy the updated time
	  nei->a      = stb->a;
	  nei->ch     = stb->ch;
	  nei->u      = stb->u;
	  nei->pmf    = stb->pmf;

	  // Eq. 4

	  nei->pow    = stb->pow;
	  nei->den    = stb->den;

	  nei->f      = stb->f;
	  nei->bmp    = stb->bmp;
	  nei->kind   = stb->kind;
	  nei->target = stb->target;
	  nei->offset = stb->offset;
	  CP(nei->pole, stb->pole);
	  CP(nei->o, org);
	  CP(nei->s, stb->s);
	  CP(nei->pP, stb->pP);
	  CP(nei->msg, stb->msg);
	  RESET(nei->p);

	  int dot = abs(DOT(org, stb->s));
	  if (dot >= dotMax)
	  {
		  dotMax = dot;
		  dst = nei;
	  }

	  // Update noise

	  nei->seed = (nei->seed << 3) ^ dir;

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
  }
  drf->f = 0;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Is there a bubble in this cell?

  if (stb->f)
  {
    // Bump sine generator
    // (Euler formula for sine)

    if (stb->bmp && stb->t > 0)
    {
      drf->u = stb->u * (stb->t * stb->t - MOD2(stb->o)) / (stb->t * stb->t);
      drf->bmp--;
    }

    // Bubble track decay (Equation 6)

    if (!ZERO(drf->pP))
    {
      // Vector p shrinks

      drf->pP[0] -= drf->pP[0] / (2 * drf->t);
      drf->pP[1] -= drf->pP[1] / (2 * drf->t);
      drf->pP[2] -= drf->pP[2] / (2 * drf->t);
      if (ZERO(drf->pP))
      {
        // Sec. 4.7.3 - Disconnect empodion

        drf->a ^= stb->seed;
      }
    }

    // Update the sine signal pmf (Eqn. 4)

    if(stb->den < SIDE / 2)  // eqv. infinity in the summation
    {
      drf->pow *= stb->pow * stb->pow;  // y^(2n)
      drf->den <<= 1;		// 2^n
      drf->pmf += (2 * stb->t + 1) * drf->pow / drf->den;
    }
  }

  // Update lattice decay

  if (!ZERO(stb->pP))
  {
    // Lattice track decay (Eq. 6)

    drf->pP[0] -= drf->pP[0] * drf->pP[0] / (drf->t * drf->t);
    drf->pP[1] -= drf->pP[1] * drf->pP[1] / (drf->t * drf->t);
    drf->pP[2] -= drf->pP[2] * drf->pP[2] / (drf->t * drf->t);
    if(ZERO(drf->pP))
    {
      // Disconnect empodion
      // (Sect. 4.7.3)

      drf->a ^= stb->seed;
    }
  }

  // Find new address for interaction
  // (Sect. 4.2)

  int t = (stb->t % LIGHT) + 1;	// look ahead
  int offset = (t + stb->offset) % SIDE3;
  Cell *nxt = stb - stb->offset + offset;

  // Superposing bubbles?

  if (EQ(stb->o, nxt->o))
  {
    // Check if different w1
    // (Sec. 4.7.7)

    if (W1(stb) != W1(drf))
    {
      // Super photon formation
      // (Sect. 3.1)

      if ((C(stb)^C(nxt)) == C_MASK && W0(stb) == !W0(nxt) && Q(stb) == !Q(nxt))
      {
        drf->kind = SPHOTON;
        return;
      }

      // Symmetry breaking after singularity
      // (Sect. 4.6.7)

      CP(drf->pole, stb->o);
      RESET(drf->o);
      return;
    }

    // Clump bubbles together to form particle pair fragments
    // (Sect. 4.2)

    if (stb->a != nxt->a && stb->f == 1 && nxt->f == 1)
    {
      if ((C(stb)^C(nxt)) == C_MASK && W0(stb) == !W0(nxt) && Q(stb) == !Q(nxt))
        drf->kind = PHOTON;
      else if (C(stb) == C(nxt) && W0(stb) == !W0(nxt) && Q(stb) == !Q(nxt))
        drf->kind = GLUON;
      else if (C(stb) == ~C(nxt) && W0(stb) == W0(nxt) && Q(stb) == !Q(nxt))
        drf->kind = ZB;
      else if (C(stb) == ~C(nxt) && W0(stb) == W0(nxt) && Q(stb) == Q(nxt))
        drf->kind = WB;
      else if (C(stb) == C(nxt) && C(stb) != 0 && C(stb) != 7 && Q(stb) == Q(nxt) && Q(stb) == 0)
        drf->kind = UP;
      else if (stb->ch == nxt->ch && (stb->ch == 0 || stb->ch == 7))
        drf->kind = NEUTRINO;
      else
        return;

      // Calculate the common value for f and a
      // (Sect. 4.2)

      drf->a ^= stb->a;
      drf->f++;

      // Obs.: A pair is actually formed, since the other cell
      // performs this same action in turn.
    }

    // Combine pairs to form multi pairs
    // (Sect. )

    else if (stb->f % 2 == 0 && nxt->f %2 == 0 &&
      stb->a != nxt->a)
    {
      drf->f += stb->f;
      drf->a ^= stb->a;

      // Obs.: Things fit at the end as above for the first pair
    }
  }
}

/**
 * Detects interactions.
 */
void interact()
{
  // Not the last pass of wavefront?

  if (drf->t % LIGHT != 0)
	  return;

  // Internal process?

  if(stb->a == drf->a && !ZERO(stb->p) && !ZERO(drf->p))
  {
    // Self interference (Sect. 4.6.3)

    int dif[3];
    SUB(dif, drf->o, stb->o);
    if (DOT(stb->o, drf->o) == 1 && MOD2(dif) == 0)
    {
      // Trap empodions (Sect. 4.6.3)

      if (stb->f > 0 && drf->f > 0)
      {
    	  CP(drf->msg, stb->pP);
    	  return;
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3)

      if (stb->f > 0 && drf->f == 0)
      {
        CP(drf->o, stb->o);
        return;
      }
      if (stb->f == 0 && drf->f > 0)
      {
        CP(drf->o, stb->o);
        return;
      }
    }
  }

  // Play pseudo dice to decide bubble mixing

  else if (stb->seed < drf->pmf)
  {
    // Find new address for interaction
    // (Sect. 4.2)

    int t = stb->t % LIGHT;
    int offset = (t + stb->offset) % SIDE3;
    Cell *nxt = stb - stb->offset + offset;

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
          // (msg1 != 0, photon2, p2 aligned with msg1)
          // This serves both to electrical or magnetic interaction

          if (!ZERO(nxt->msg) && drf->kind == PHOTON && DOT(nxt->msg, drf->p) == 0)
          {
            // Recruit it

            drf->a = nxt->a;
            RESET(drf->msg);
          }

          // Static forces

          else if (nxt->f == 1 && drf->f == 1 && !ZERO(nxt->p))
          {
            // Electric radial force (default)
            // drf is now a messenger
            // Sect. 4.7.2

            if (Q(nxt) == Q(drf))
            {
              // Repulsion

              SUB(drf->msg, drf->o, nxt->o);
            }
            else
            {
              // Attraction

              SUB(drf->msg, nxt->o, drf->o);
            }
            if (nxt->seed % 2 == 1)
            {
              // Magnetic lateral kick
              // Sect. 4.7.2

              CROSS(drf->msg, drf->msg, drf->s);
            }
          }

          // fermion- x boson (local light-matter)
          // fermion+ x boson (local light-matter)

          else if (nxt->f == 1 && drf->f > 1)
          {
            // boson x fermion

            switch(drf->kind)
            {
              case PHOTON:

                // Reemit from cp

                RESET(drf->pole);
                RESET(drf->o);
                break;

              case ZB:
              case WB:
                if (MATTER(nxt))
                {
                  if (W0(nxt) == LEFT)
                  {
                    if ((MATTER(drf) && W0(drf) == LEFT) || (!MATTER(drf) && W0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MATTER(drf) && W0(drf) == LEFT) || (!MATTER(drf) && W0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                break;
            }
          }
          else if (nxt->f > 1 && drf->f == 1)
          {
            // fermion x boson

            switch(nxt->kind)
            {
              case PHOTON:

                // Reemit from cp

                RESET(drf->pole);
                RESET(drf->o);
                break;

              case ZB:
              case WB:
                if (MATTER(nxt))
                {
                  if (W0(nxt) == LEFT)
                  {
                    if ((MATTER(drf) && W0(drf) == LEFT) || (!MATTER(drf) && W0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MATTER(drf) && W0(drf) == LEFT) || (!MATTER(drf) && W0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                break;
            }
          }

          // boson x boson

          else if (nxt->f > 1 && drf->f > 1)
          {
            // Exchange charges and spins

            CP(drf->s, nxt->s);
            drf->ch |= (nxt->ch & C_MASK);
            drf->ch |= (nxt->ch & W0_MASK);

            // Reemit from cp

            RESET(drf->pole);
            RESET(drf->o);
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->pole, nxt->o, drf->o);
          RESET(drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

          RESET(drf->pole); // cp
          RESET(drf->o);
          break;

        case 6:    // Collapse

          // fermion+ x fermion- (annihilation) ?
          // fermion- x fermion+ (annihilation) ?

          if ((nxt->ch & 0x1f) == ~(drf->ch & 0x1f) && nxt->kind == FERMION)
          {
            // Destroy particles, setting each 'a' property to a
            // new, pseudorandom value

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target;  // copy for flash
            drf->kind = 0;         // undo pairing
          }
          else if (nxt->kind == FERMION && drf->kind != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target;    // copy for flash
            drf->kind = 0;           // undo pairing
          }
          else if (nxt->kind != FERMION && drf->kind == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target; // copy for flash
            drf->kind = 0;        // undo pairing
          }
          else if (nxt->kind == NEUTRINO && drf->kind == FERMION)
          {
        	  CP(drf->pole, nxt->o); // enough?!
          }
          else
          {
            // fermion+ x fermion+ (scattering)
            // fermion- x fermion- (scattering)

            drf->target = drf->a;
            target = drf->target; // copy for flash
          }

          // All options in this case reissue from cp

          RESET(drf->pole);
          RESET(drf->o);
          drf->target = nxt->a;   // non trivial target
          break;

        case 7:    // Internal collision

          // Reissue from cp

          RESET(drf->pole);
          RESET(drf->o);
          break;
      }
    }

    // Inter-sector interaction
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion

      if (nxt->f == 1 && drf->f > 1 && drf->kind == SPHOTON)
      {
        RESET(drf->pole);
        RESET(drf->o);
      }
      else if (nxt->f > 1 && drf->f == 1 && nxt->kind == SPHOTON)
      {
        RESET(drf->o);
        RESET(drf->pole);
      }
    }

    // Release a flash

    drf->flash = true;
    drf->target = target;  // collapse target
  }
}

