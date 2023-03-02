///////////////////////////////
//                           //
// -- Universe's software -- //
//                           //
///////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "simulation.h"
#include "utils.h"

Tensor *lattice0, *lattice1;

// Current pointers

Tensor *stb, *drf;

extern pthread_mutex_t mutex;

int DOT(int *u, int *v)
{
	return u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
}

int MOD2(int *v)
{
	return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
}


/**
 * Copy data.
 */
void copy()
{
  pthread_mutex_lock(&mutex);
  stb->ch     = drf->ch;
  stb->a      = drf->a;
  stb->t      = drf->t;
  stb->tt     = drf->tt;
  stb->syn    = drf->syn;
  stb->u      = drf->u;
  stb->v      = drf->v;
  stb->f      = drf->f;
  stb->flash  = drf->flash;
  stb->kind   = drf->kind;
  stb->noise  = drf->noise;
  stb->target = drf->target;
  stb->offset = drf->offset;
  COPY(stb->p, drf->p);
  COPY(stb->s, drf->s);
  COPY(stb->o, drf->o);
  COPY(stb->pole, drf->pole);
  COPY(stb->p0, drf->p0);
  COPY(stb->prone, drf->prone);
  pthread_mutex_unlock(&mutex);
}

/**
 * Spreads flash.
 */
void flash()
{
  // Flash greedy expansion

  if (stb->flash == 0)
	  return;

  // Flash decay

  drf->flash--;

  // Test if pole was found

  boolean isPole = false;
  if (!ISNULL(stb->p) && ISNULL(stb->pole) && stb->f > 0 && ISNULL(stb->o))
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
  Tensor* nei;
  for (int dir = 0; dir < 6; dir++)
  {
    // Initialize new origin vector

    COPY(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Backwards?

    if(abs(org[i]) < abs(stb->o[i]))
      continue;

    // No conflict?

    if (isAllowed(dir, org))
    {
      // Get neighbor

      nei = drf->wires[dir];

      // Propagate flash flag

      nei->flash = true;

      // Propagate sought affinity for collapse

      nei->target = stb->target;

      // Propagate new radius

      COPY(nei->o, org);

      if (!ISNULL(stb->pole) && !isPole)
      {
        // Shrink pole as flash approaches re-emission point

        nei->pole[0] = stb->pole[0] - (dir < 2);
        nei->pole[1] = stb->pole[1] - (dir %2 == 1);
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

  // Wrapping?

  if (MOD2(stb->o) == 3 * SIDE2 / 4)
  {
    // Brand new

    drf->t      = 0;
    drf->tt     = 0;
    drf->syn    = 0;
    drf->target = SIDE3;
    RESET(drf->o);

    // TODO: check these

    RESET(drf->p0);
    RESET(drf->prone);
    RESET(drf->pole);

    //  DEBUG

    if (stb->a == 0)
    	drf->flash = (rand() % 10) == 2 ? 1 : 0;
    return;
  }

  // Increment lifetime

  drf->t++;

  // Is wavefront synchronized? (see Ref. [15])

  if (drf->t * drf->t <= stb->syn)
    return;

  // Explore von Neumann directions

  int org[3];
  Tensor* nei;
  int dotMax = 0;
  Tensor *dst = NULL;
  for (int dir = 0; dir < 6; dir++)
  {
    // Initialize new origin vector

	COPY(org, stb->o);
	int i = dir >> 1;
	org[i] += (dir % 2 == 0) ? +1 : -1;

	// Backwards?

	if(abs(org[i]) < abs(stb->o[i]))
	  continue;

	// No conflict?

	if (isAllowed(dir, org))
	{
	  // Propagate information

	  nei = drf->wires[dir];
	  nei->t      = drf->t;  // copy the updated time
	  nei->a      = stb->a;
	  nei->ch     = stb->ch;
	  nei->tt     = stb->tt;
	  nei->u      = stb->u;
	  nei->v      = stb->v;
	  nei->f      = stb->f;
	  nei->kind   = stb->kind;
	  nei->target = stb->target;
	  nei->offset = stb->offset;
	  COPY(nei->pole, stb->pole);
	  COPY(nei->o, org);
	  COPY(nei->s, stb->s);
	  COPY(nei->p0, stb->p0);
	  COPY(nei->prone, stb->prone);
	  RESET(nei->p);
	  if(!ISNULL(stb->p))
	  {
		  int dot = abs(DOT(org, stb->s));
		  if (dot >= dotMax)
		  {
			  dotMax = dot;
			  dst = nei;
		  }
	  }

	  // Update noise

	  nei->noise = (nei->noise << 3) ^ dir;

      // Set timing for spherical pattern (Section 3)

      nei->syn = LIGHT2 * MOD2(org);
    }
  }
  if (dst != NULL)
  {
    // Propagate momentum

    COPY(dst->p, stb->p);

    // Abandon the origin cell.
    // Tracking information remains.

    RESET(drf->p);
    //printf("if: %d,%d,%d\n", stb->o[0], stb->o[1], stb->o[2]);
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

    int v = stb->t * stb->t * SIDE2;
    drf->u = stb->u * (v - stb->t * stb->t);
    drf->f--;  // original value is preserved in the stable cell

    // Bubble track decay (Equation 1)

    if (drf->f == 0 && !ISNULL(drf->p))
    {
      // Vector p shrinks

      drf->p[0] -= drf->p[0] / (2 * drf->t);
      drf->p[1] -= drf->p[1] / (2 * drf->t);
      drf->p[2] -= drf->p[2] / (2 * drf->t);
      // draft->t--;
    }
  }

  // Update lattice decay

  if (stb->f == 0 && stb->tt > 0)
  {
    // Lattice track decay (Equation 2)

    drf->p[0] -= drf->p[0] * drf->p0[0] / (drf->tt * drf->tt);
    drf->p[1] -= drf->p[1] * drf->p0[1] / (drf->tt * drf->tt);
    drf->p[2] -= drf->p[2] * drf->p0[2] / (drf->tt * drf->tt);
    drf->tt--;
  }

  // Find new address for interaction

  int offset = (stb->t + stb->offset) % SIDE3;
  Tensor *nxt = (offset == SIDE3 - 1) ? stb - SIDE3 + 1 : stb + 1;
  assert(nxt != stb);

  // Superposing bubbles?

  if (ISEQUAL(stb->o, nxt->o))
  {
    // Clump bubbles together to form particle pair fragments

    if (stb->a != nxt->a && stb->f == 1 && nxt->f == 1)
    {
      if ((GETC(stb)^GETC(nxt)) == C_MASK && GETW0(stb) == !GETW0(nxt) && GETQ(stb) == !GETQ(nxt))
        drf->kind = PHOTON;
      else if (GETC(stb) == GETC(nxt) && GETW0(stb) == !GETW0(nxt) && GETQ(stb) == !GETQ(nxt))
        drf->kind = GLUON;
      else if (GETC(stb) == ~GETC(nxt) && GETW0(stb) == GETW0(nxt) && GETQ(stb) == !GETQ(nxt))
        drf->kind = ZB;
      else if (GETC(stb) == ~GETC(nxt) && GETW0(stb) == GETW0(nxt) && GETQ(stb) == GETQ(nxt))
        drf->kind = WB;
      else if (GETC(stb) == GETC(nxt) && GETC(stb) != 0 && GETC(stb) != 7 && GETQ(stb) == GETQ(nxt) && GETQ(stb) == 0)
        drf->kind = UP;
      else if (stb->ch == nxt->ch && (stb->ch == 0 || stb->ch == 7))
        drf->kind = NEUTRINO;
      else
        return;

      // Calculate the common value for f and a

      drf->a ^= stb->a;
      drf->f++;

      // A pair is actually formed, since the other cell performs
      // this same action in turn.
    }

    // Combine pairs to form multi pairs

    else if (stb->f % 2 == 0 && nxt->f %2 == 0 &&
      stb->a != nxt->a)
    {
      drf->f += stb->f;
      drf->a ^= stb->a;

      // Things fit at the end as above for the first pair
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

  // No interaction possible?

  if (!drf->kind)
    return;

  // Update the sine signal pmf (see Section

  int v = stb->t * stb->t * SIDE2;
  drf->v += ((2 * stb->t + 1) * v  * v) >> 1;

  // Figure out the expected kind of interaction

  int code = (stb->a == drf->a) | (!ISNULL(stb->p) << 1)|
                  (!ISNULL(drf->p) << 2);

  // Initialize collapse target

  unsigned target = SIDE3;

  // Play pseudo dices to decide bubble mixing

  if (stb->noise < drf->v)
  {
    // Test for same or different sectors

    if (GETW1(stb) == GETW1(drf))
    {
      // Same-sector

      switch(code)
      {
        case 1:    // Soft interactions

          if (stb->a == drf->a)
          {
            // Self interference

            int sub[3];
            SUB(sub, drf->o, stb->o);
            if (DOT(stb->o, drf->o) == 0 && abs(MOD2(sub)) < TOL)
            {
              // Sciarreta goes here
            }
          }
          else
          {
            // Phase only interaction
          }
          break;

        case 2:    // Rendez vous (different particles)
        case 4:    // Rendez vous (different particles)

          // Virtual photon capture
          // (prone1 != 0, photon2, p2 aligned with prone1)
          // This serves both to electrical or magnetic interaction

          if (!ISNULL(stb->prone) && drf->kind == PHOTON && DOT(stb->prone, drf->p) == 0)
          {
            // Recruit it

            drf->a = stb->a;
            RESET(drf->prone);
          }

          // fermion+ x fermion+ (partial scattering)
          // fermion- x fermion- (partial scattering)
          // repulsion

          else if (stb->f == 1 && drf->f == 1 && GETQ(stb) == GETQ(drf) && !ISNULL(stb->p))
          {
            if (stb->noise % 2 == 0)
            {
              // Electric radial force

              SUB(drf->prone, drf->o, stb->o);
            }
            else
            {
              // Magnetic lateral kick

              int dif[3];
              SUB(dif, drf->o, stb->o);
              CROSS(drf->prone, dif, drf->s);
            }
          }

          // fermion+ x fermion- (partial scattering)
          // fermion- x fermion+ (partial scattering)
          // attraction

          else if (stb->f == 1 && drf->f == 1 && GETQ(stb) != GETQ(drf) && !ISNULL(stb->p))
          {
            if (stb->noise % 2 == 0)
            {
              // Electric radial force

              SUB(drf->prone, stb->o, drf->o);
            }
            else
            {
              // Magnetic lateral kick

              int dif[3];
              SUB(dif, drf->o, stb->o);
              CROSS(drf->prone, dif, drf->s);
              NEG(drf->prone);
            }
          }

          // fermion- x boson (local light-matter)
          // fermion+ x boson (local light-matter)

          else if (stb->f == 1 && drf->f > 1)
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
                if (ISMATTER(stb))
                {
                  if (GETW0(stb) == LEFT)
                  {
                    if ((ISMATTER(drf) && GETW0(drf) == LEFT) || (!ISMATTER(drf) && GETW0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                else
                {
                  if (GETW0(stb) == RIGHT)
                  {
                    if ((ISMATTER(drf) && GETW0(drf) == LEFT) || (!ISMATTER(drf) && GETW0(drf) == RIGHT))
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
          else if (stb->f > 1 && drf->f == 1)
          {
            // fermion x boson

            switch(stb->kind)
            {
              case PHOTON:

                // Reemit from cp

                RESET(drf->pole);
                RESET(drf->o);
                break;

              case ZB:
              case WB:
                if (ISMATTER(stb))
                {
                  if (GETW0(stb) == LEFT)
                  {
                    if ((ISMATTER(drf) && GETW0(drf) == LEFT) || (!ISMATTER(drf) && GETW0(drf) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(drf->pole);
                      RESET(drf->o);
                    }
                  }
                }
                else
                {
                  if (GETW0(stb) == RIGHT)
                  {
                    if ((ISMATTER(drf) && GETW0(drf) == LEFT) || (!ISMATTER(drf) && GETW0(drf) == RIGHT))
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

          else if (stb->f > 1 && drf->f > 1)
          {
            // Exchange charges and spins

            COPY(drf->s, stb->s);
            drf->ch |= (stb->ch & C_MASK);
            drf->ch |= (stb->ch & W0_MASK);

            // Reemit from cp

            RESET(drf->pole);
            RESET(drf->o);
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->pole, stb->o, drf->o);
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

          if ((stb->ch & 0x1f) == ~(drf->ch & 0x1f) && stb->kind == FERMION)
          {
            // Destroy particles, setting each 'a' property to a
            // new, pseudorandom value

            drf->a ^= stb->noise;
            drf->target = drf->a;
            target = drf->target;  // copy for flash
            drf->kind = 0;         // undo pairing
          }
          else if (stb->kind == FERMION && drf->kind != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= stb->noise;
            drf->target = drf->a;
            target = drf->target;    // copy for flash
            drf->kind = 0;           // undo pairing
          }
          else if (stb->kind != FERMION && drf->kind == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= stb->noise;
            drf->target = drf->a;
            target = drf->target; // copy for flash
            drf->kind = 0;        // undo pairing
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
          drf->target = stb->a;   // non trivial target
          break;

        case 7:    // Internal collision

          // Reissue from cp

          RESET(drf->pole);
          RESET(drf->o);
          break;
      }
    }

    // Intersector interaction
    // (see Section 4.8.3)

    else if ((code >> 1) == 3)
    {
      // Exchange electrical charges if ions

      if (stb->ch == 0x03 && drf->ch == 0x28)
        drf->ch ^= Q_MASK;

      // Reissue

      RESET(drf->pole);
      RESET(drf->o);
    }
  }

  // Release a flash

  drf->flash = true;
  drf->target = target;  // collapse target
}
