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

Tensor *stbl, *draft;

extern pthread_mutex_t mutex;

/**
 * Copy data.
 */
void copy()
{
  pthread_mutex_lock(&mutex);
  stbl->ch     = draft->ch;
  stbl->a      = draft->a;
  stbl->t      = draft->t;
  stbl->tt     = draft->tt;
  stbl->syn    = draft->syn;
  stbl->u      = draft->u;
  stbl->v      = draft->v;
  stbl->f      = draft->f;
  stbl->flash  = draft->flash;
  stbl->kind   = draft->kind;
  stbl->noise  = draft->noise;
  stbl->target = draft->target;
  stbl->offset = draft->offset;
  COPY(stbl->p, draft->p);
  COPY(stbl->s, draft->s);
  COPY(stbl->o, draft->o);
  COPY(stbl->pole, draft->pole);

  COPY(stbl->p0, draft->p0);
  COPY(stbl->prone, draft->prone);
  pthread_mutex_unlock(&mutex);
}

/**
 * Spreads flash.
 */
void flash()
{
  // Flash greedy expansion

  if (stbl->flash == 0)
	  return;

  // Flash decay

  draft->flash = stbl->flash - 1;

  // Test if pole was found (momentum must be present)

  int f = stbl->f;
  if (!ISNULL(stbl->p) && ISNULL(stbl->pole) && stbl->f > 0 && ISNULL(stbl->o))
  {
    // Pole found: create new seed

    COPY(draft->p, stbl->p);
    draft->f = stbl->f;
    RESET(draft->pole);
    RESET(draft->o);
    draft->target = stbl->a;  // non trivial target
    f = 0;                    // warn that pole was found
  }

  // Collapse forces all affine bubbles to reissue

  if (stbl->a == stbl->target)
  {
    // Reissue

    draft->f = stbl->f;
    RESET(draft->pole);
    RESET(draft->o);
    draft->target = stbl->a;   // non trivial target
  }

  // Explore von Neumann directions

  int org[3];
  Tensor* neighbor;
  for (int dir = 0; dir < 6; dir++)
  {
    // Wrapping condition

    COPY(org, stbl->o);
    switch(dir)
    {
      case 0:
        if (org[0] == SIDE - 1 || org[0] < 0)
      	  continue;
      	org[0]++;
      	break;
      case 1:
        if (org[0] == -SIDE + 1 || org[0] > 0)
          continue;
      	org[0]--;
      	break;
      case 2:
        if (org[1] == SIDE - 1 || org[1] < 0)
          continue;
      	org[1]++;
      	break;
      case 3:
        if (org[1] == -SIDE + 1 || org[1] > 0)
          continue;
      	org[1]--;
      	break;
      case 4:
        if (org[2] == SIDE - 1 || org[2] < 0)
      	  continue;
      	org[2]++;
      	break;
      case 5:
        if (org[2] == -SIDE + 1 || org[2] > 0)
      	  continue;
        org[2]--;
        break;
    }
    neighbor = draft->wires[dir];

    // Flash is short-lived

    neighbor->flash = 1;

    // Propagate sought affinity for collapse

    neighbor->target = stbl->target;

    // Propagate particle presence

    neighbor->f = f;
    COPY(neighbor->o, org);

    // Propagate momentum

    if (f > 0)
    {
      COPY(neighbor->p, stbl->p);
    }
    else
    {
      RESET(neighbor->p);
    }

    if (!ISNULL(stbl->pole) && stbl->f > 0)
    {
      // Shrink pole as flash approaches re-emission point

   	  neighbor->pole[0] = stbl->pole[0] - (dir < 2);
      neighbor->pole[1] = stbl->pole[1] - (dir %2 == 1);
      neighbor->pole[2] = stbl->pole[2] - (dir > 3);
    }
  }
}

/**
 * Expands wavefront.
 */
void expand()
{
  // Test wrapping

  if(stbl->o[0] * stbl->o[0] + stbl->o[1] * stbl->o[1] + stbl->o[2] * stbl->o[2] == LIMIT)
  {
	//  if(ISNULL(stbl->p))
		//  return;

	  // Brand new

	  draft->t = 0;
	  draft->syn = 0;
	  RESET(draft->o);
	  RESET(draft->pole);
	  draft->p[0] = 1;
  }

  // Increment lifetime

  draft->t++;

  // Is wavefront synchronized? (see Ref. [15])

  if (draft->t * draft->t <= stbl->syn)
    return;
  if(stbl->f == 0)
	return;

  // Explore von Neumann directions

  int org[3];
  Tensor* neighbor;
  int dotMax = 0;
  Tensor *dst = NULL;
  for (int dir = 0; dir < 6; dir++)
  {
    // Initialize new origin vector

    COPY(org, stbl->o);

    // Test if branch is legal

    neighbor = draft->wires[dir];
    if (neighbor->f > 0)
    	continue;
    switch(dir)
    {
      case 0:
        if (org[0] == SIDE - 1 || org[0] < 0)
      	  continue;
      	org[0]++;
      	break;
      case 1:
        if (org[0] == -SIDE + 1 || org[0] > 0)
          continue;
      	org[0]--;
      	break;
      case 2:
        if (org[1] == SIDE - 1 || org[1] < 0)
          continue;
      	org[1]++;
      	break;
      case 3:
        if (org[1] == -SIDE + 1 || org[1] > 0)
          continue;
      	org[1]--;
      	break;
      case 4:
        if (org[2] == SIDE - 1 || org[2] < 0)
      	  continue;
      	org[2]++;
      	break;
      case 5:
        if (org[2] == -SIDE + 1 || org[2] > 0)
      	  continue;
        org[2]--;
        break;
    }

    // Check if is seed cell

    int dot = org[0] * stbl->s[0] + org[1] * stbl->s[1] + org[2] * stbl->s[2];
    if(dot > dotMax)
    {
    	dotMax = dot;
    	dst = neighbor;
    }

    // Propagate all information

    neighbor->t      = draft->t;  // copy the updated time
    neighbor->a      = stbl->a;
    neighbor->ch     = stbl->ch;
    neighbor->tt     = stbl->tt;
    neighbor->u      = stbl->u;
    neighbor->v      = stbl->v;
    neighbor->f      = stbl->f;
    neighbor->kind   = stbl->kind;
    neighbor->target = stbl->target;
    neighbor->offset = stbl->offset;
    COPY(neighbor->pole, stbl->pole);
    COPY(neighbor->o, org);
    COPY(neighbor->s, stbl->s);
    COPY(neighbor->p0, stbl->p0);
    COPY(neighbor->prone, stbl->prone);
    RESET(neighbor->p);

    // Update noise

    neighbor->noise = (neighbor->noise << 3) ^ dir;

    // Set timing for spherical pattern (Section 3)

    neighbor->syn = LIGHT2 * MOD2(org);
  }

  // Propagate momentum

  if(!ISNULL(stbl->p))
	    COPY(dst->p, stbl->p);

  // Abandon the origin cell.
  // Tracking information remains.

  draft->f = 0;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Is there a bubble in this cell?

  if (stbl->f)
  {
    // Bump sine generator
    // (Euler formula for sine)
/*
    int v = stbl->t * stbl->t * SIDE2;
    draft->u = stbl->u * (v - stbl->t * stbl->t);
    draft->f--;  // original value is preserved in the stable cell

    // Bubble track decay (Equation 1)

    if (draft->f == 0 && !ISNULL(draft->p))
//    if (draft->f == 0 && draft->t > 0)
    {
      // Vector p shrinks

      draft->p[0] -= draft->p[0] / (2 * draft->t);
      draft->p[1] -= draft->p[1] / (2 * draft->t);
      draft->p[2] -= draft->p[2] / (2 * draft->t);
//      draft->t--;
    }
    */
  }

  /*
  // Update lattice decay

  if (stbl->f == 0 && stbl->tt > 0)
  {
    // Lattice track decay (Equation 2)

    draft->p[0] -= draft->p[0] * draft->p0[0] / (draft->tt * draft->tt);
    draft->p[1] -= draft->p[1] * draft->p0[1] / (draft->tt * draft->tt);
    draft->p[2] -= draft->p[2] * draft->p0[2] / (draft->tt * draft->tt);
    draft->tt--;
  }

  // Find new address for interaction

  int x = ((stbl->offset & MASK) + stbl->o[0]) % SIDE ;
  int y = (((stbl->offset >> ORDER) & MASK) + stbl->o[1]) % SIDE;
  int z = ((stbl->offset >> ORDER >> ORDER) + stbl->o[2]) % SIDE;
  Tensor *dst = stbl + x + y * SIDE + z * SIDE2;

  // Superposing bubbles?

  if (ISEQUAL(stbl->o, dst->o))
  {
    // Clump bubbles together to form particle pair fragments

    if (stbl->a != dst->a && stbl->f == 1 && dst->f == 1)
    {
      if ((GETC(stbl)^GETC(dst)) == C_MASK && GETW0(stbl) == !GETW0(dst) && GETQ(stbl) == !GETQ(dst))
        draft->kind = PHOTON;
      else if (GETC(stbl) == GETC(dst) && GETW0(stbl) == !GETW0(dst) && GETQ(stbl) == !GETQ(dst))
        draft->kind = GLUON;
      else if (GETC(stbl) == ~GETC(dst) && GETW0(stbl) == GETW0(dst) && GETQ(stbl) == !GETQ(dst))
        draft->kind = ZB;
      else if (GETC(stbl) == ~GETC(dst) && GETW0(stbl) == GETW0(dst) && GETQ(stbl) == GETQ(dst))
        draft->kind = WB;
      else if (GETC(stbl) == GETC(dst) && GETC(stbl) != 0 && GETC(stbl) != 7 && GETQ(stbl) == GETQ(dst) && GETQ(stbl) == 0)
        draft->kind = UP;
      else if (stbl->ch == dst->ch && (stbl->ch == 0 || stbl->ch == 7))
        draft->kind = NEUTRINO;
      else
        return;

      // Calculate the common value for f and a

      draft->a ^= stbl->a;
      draft->f++;

      // A pair is actually formed, since the other cell performs
      // this same action in turn.
    }

    // Combine pairs to form multi pairs

    else if (stbl->f % 2 == 0 && dst->f %2 == 0 &&
      stbl->a != dst->a)
    {
      draft->f += stbl->f;
      draft->a ^= stbl->a;

      // Things fit at the end as above for the first pair
    }
  }
  */
}

/**
 * Detects interactions.
 */
void interact()
{
  // Not the last pass of wavefront?

  if (draft->t % LIGHT != 0)
    return;

  // No interaction possible?

  if (!draft->kind)
    return;

  // Update the sine signal pmf (see Section

  int v = stbl->t * stbl->t * SIDE2;
  draft->v += ((2 * stbl->t + 1) * v  * v) >> 1;

  // Figure out the expected kind of interaction

  int code = (stbl->a == draft->a) | (!ISNULL(stbl->p) << 1)|
                  (!ISNULL(draft->p) << 2);

  // Initialize collapse target

  unsigned target = 0;

  // Play pseudo dices to decide bubble mixing

  if (stbl->noise < draft->v)
  {
    // Test for same or different sectors

    if (GETW1(stbl) == GETW1(draft))
    {
      // Same-sector

      switch(code)
      {
        case 1:    // Soft interactions

          if (stbl->a == draft->a)
          {
            // Self interference

            int sub[3];
            SUB(sub, draft->o, stbl->o);
            if (DOT(stbl->o, draft->o) == 0 && abs(MOD2(sub)) < TOL)
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

          if (!ISNULL(stbl->prone) && draft->kind == PHOTON && DOT(stbl->prone, draft->p) == 0)
          {
            // Recruit it

            draft->a = stbl->a;
            RESET(draft->prone);
          }

          // fermion+ x fermion+ (partial scattering)
          // fermion- x fermion- (partial scattering)
          // repulsion

          else if (stbl->f == 1 && draft->f == 1 && GETQ(stbl) == GETQ(draft) && !ISNULL(stbl->p))
          {
            if (stbl->noise % 2 == 0)
            {
              // Electric radial force

              SUB(draft->prone, draft->o, stbl->o);
            }
            else
            {
              // Magnetic lateral kick

              int dif[3];
              SUB(dif, draft->o, stbl->o);
              CROSS(draft->prone, dif, draft->s);
            }
          }

          // fermion+ x fermion- (partial scattering)
          // fermion- x fermion+ (partial scattering)
          // attraction

          else if (stbl->f == 1 && draft->f == 1 && GETQ(stbl) != GETQ(draft) && !ISNULL(stbl->p))
          {
            if (stbl->noise % 2 == 0)
            {
              // Electric radial force

              SUB(draft->prone, stbl->o, draft->o);
            }
            else
            {
              // Magnetic lateral kick

              int dif[3];
              SUB(dif, draft->o, stbl->o);
              CROSS(draft->prone, dif, draft->s);
              NEG(draft->prone);
            }
          }

          // fermion- x boson (local light-matter)
          // fermion+ x boson (local light-matter)

          else if (stbl->f == 1 && draft->f > 1)
          {
            // boson x fermion

            switch(draft->kind)
            {
              case PHOTON:

                // Reemit from cp

                RESET(draft->pole);
                RESET(draft->o);
                break;

              case ZB:
              case WB:
                if (ISMATTER(stbl))
                {
                  if (GETW0(stbl) == LEFT)
                  {
                    if ((ISMATTER(draft) && GETW0(draft) == LEFT) || (!ISMATTER(draft) && GETW0(draft) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(draft->pole);
                      RESET(draft->o);
                    }
                  }
                }
                else
                {
                  if (GETW0(stbl) == RIGHT)
                  {
                    if ((ISMATTER(draft) && GETW0(draft) == LEFT) || (!ISMATTER(draft) && GETW0(draft) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(draft->pole);
                      RESET(draft->o);
                    }
                  }
                }
                break;
            }
          }
          else if (stbl->f > 1 && draft->f == 1)
          {
            // fermion x boson

            switch(stbl->kind)
            {
              case PHOTON:

                // Reemit from cp

                RESET(draft->pole);
                RESET(draft->o);
                break;

              case ZB:
              case WB:
                if (ISMATTER(stbl))
                {
                  if (GETW0(stbl) == LEFT)
                  {
                    if ((ISMATTER(draft) && GETW0(draft) == LEFT) || (!ISMATTER(draft) && GETW0(draft) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(draft->pole);
                      RESET(draft->o);
                    }
                  }
                }
                else
                {
                  if (GETW0(stbl) == RIGHT)
                  {
                    if ((ISMATTER(draft) && GETW0(draft) == LEFT) || (!ISMATTER(draft) && GETW0(draft) == RIGHT))
                    {
                      // Reemit from cp

                      RESET(draft->pole);
                      RESET(draft->o);
                    }
                  }
                }
                break;
            }
          }

          // boson x boson

          else if (stbl->f > 1 && draft->f > 1)
          {
            // Exchange charges and spins

            COPY(draft->s, stbl->s);
            draft->ch |= (stbl->ch & C_MASK);
            draft->ch |= (stbl->ch & W0_MASK);

            // Reemit from cp

            RESET(draft->pole);
            RESET(draft->o);
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(draft->pole, stbl->o, draft->o);
          RESET(draft->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

          RESET(draft->pole); // cp
          RESET(draft->o);
          break;

        case 6:    // Collapse

          // fermion+ x fermion- (annihilation) ?
          // fermion- x fermion+ (annihilation) ?

          if ((stbl->ch & 0x1f) == ~(draft->ch & 0x1f) && stbl->kind == FERMION)
          {
            // Destroy particles, setting each 'a' property to a
            // new, pseudorandom value

            draft->a ^= stbl->noise;
            draft->target = draft->a;
            target = draft->target;  // copy for flash
            draft->kind = 0;         // undo pairing
          }
          else if (stbl->kind == FERMION && draft->kind != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            draft->a ^= stbl->noise;
            draft->target = draft->a;
            target = draft->target;    // copy for flash
            draft->kind = 0;           // undo pairing
          }
          else if (stbl->kind != FERMION && draft->kind == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            draft->a ^= stbl->noise;
            draft->target = draft->a;
            target = draft->target; // copy for flash
            draft->kind = 0;        // undo pairing
          }
          else
          {
            // fermion+ x fermion+ (scattering)
            // fermion- x fermion- (scattering)

            draft->target = draft->a;
            target = draft->target; // copy for flash
          }

          // All options in this case reissue from cp

          RESET(draft->pole);
          RESET(draft->o);
          draft->target = stbl->a;   // non trivial target
          break;

        case 7:    // Internal collision

          // Reissue from cp

          RESET(draft->pole);
          RESET(draft->o);
          break;
      }
    }

    // Intersector interaction
    // (see Section 4.8.3)

    else if ((code >> 1) == 3)
    {
      // Exchange electrical charges if ions

      if (stbl->ch == 0x03 && draft->ch == 0x28)
        draft->ch ^= Q_MASK;

      // Reissue

      RESET(draft->pole);
      RESET(draft->o);
    }
  }

  // Release a flash

  draft->flash = 1;        // this low value vanishes quickly
  draft->target = target;  // collapse target
}
