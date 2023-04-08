///////////////////////////////
//                           //
// -- Universe's software -- //
//                           //
///////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "simulation.h"
#include "utils.h"

Cell *latt0, *latt1;

// Current pointers

Cell *stb, *drf;

/**
 * Copy data.
 */
void copy()
{
  // Clock tick

  if(stb->reemit)
    drf->n = 0;
  else
    drf->n++;
  drf->reemit = NONE;

  // Lattice update

  stb->ch     = drf->ch;
  stb->a      = drf->a;
  stb->n      = drf->n;
  stb->syn    = drf->syn;
  stb->u      = drf->u;
  stb->pmf    = drf->pmf;
  stb->f      = drf->f;
  stb->bmp    = drf->bmp;
  stb->flash  = drf->flash;
  stb->kind   = drf->kind;
  stb->seed   = drf->seed;
  stb->target = drf->target;
  CP(stb->p, drf->p);
  CP(stb->s, drf->s);
  CP(stb->o, drf->o);
  CP(stb->pole, drf->pole);
  CP(stb->dom, drf->dom);
  CP(stb->pP, drf->pP);
  CP(stb->msg, drf->msg);

  // Eq. 4

  stb->pow    = drf->pow;
  stb->den    = drf->den;
}

/**
 * Spreads flash.
 */
void flash()
{
  // Flash greedy expansion

  if (!stb->flash)
    goto EXPAND;

  // Clean flash

  drf->flash = false;

  // Test if pole was found

  if (!ZERO(stb->p) && ZERO(stb->pole) && stb->f > 0 && ZERO(stb->o))
  {
    // Pole found: create new seed

    drf->reemit = CONTACT;
    drf->target = stb->a;  // non trivial target
    goto EXPAND;
  }

  // A collapse forces all affine bubbles to reissue

  else if (stb->a == stb->target)
  {
    // Reissue

    drf->reemit = CONTACT;
    drf->target = stb->a;  // non trivial target
    goto EXPAND;
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

      if (!ZERO(stb->pole))
      {
        // Shrink pole as flash approaches re-emission point

        nei->pole[0] = stb->pole[0] - (dir < 2);
        nei->pole[1] = stb->pole[1] - (dir % 2 == 1);
        nei->pole[2] = stb->pole[2] - (dir > 3);
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

  if (drf->reemit)
    goto UPDATE;

  // Empty?

  if (stb->f == 0)
    goto UPDATE;

  // Wrapping?

  if (abs(stb->o[0]) == SIDE - 2 &&
      abs(stb->o[1]) == SIDE - 2 &&
      abs(stb->o[2]) == SIDE - 2)
  {
    drf->reemit = IMMEDIATE;
    drf->syn    = 0;
    drf->target = SIDE3;

    // Eq. 4

    drf->den    = 1;		// 2^0	TODO: check if den always > 0
    drf->pow    = 1;		// y^0

    CP(drf->p, stb->p);	// patch 18/3
    RESET(drf->o);

    // TODO: check these

    RESET(drf->pP);
    RESET(drf->msg);
    RESET(drf->pole);

    goto UPDATE;
  }

  // Is wavefront synchronized? (see Ref. [15])

  if (drf->n * drf->n <= stb->syn)
    goto UPDATE;

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
	  nei->n      = stb->n;
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
  UPDATE: ;
}

/**
 * Updates before interaction.
 */
void update()
{
  // Wrapping?

  if(drf->reemit)
    goto INTERACT;

  // Physical time

  int t = stb->n / LIGHT;

  // Is there a bubble in this cell?
  // Last tick?

  if (stb->f && stb->n % LIGHT == 0)
  {
    // Bump sine generator
    // (Euler formula for sine)

    if (stb->bmp && t > 0)
    {
      drf->u = stb->u * (t * t - MOD2(stb->o)) / (t * t);
      drf->bmp--;
    }

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

        drf->a ^= stb->seed;
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

      drf->a ^= stb->seed;
    }
  }

  // Find new address for interaction
  // (Sect. 4.2)

  t++;					// look ahead
  int offset;
  if(stb->offset % 2 == 0)
	  offset = (stb->offset + t) % SIDE3;
  else
	  offset = (stb->offset - t) % SIDE3;
  Cell *nxt = stb - stb->offset + offset;

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
        drf->kind = SPHOTON;

        // Calculate the common value for f and a
        // (Sect. 4.2)

        drf->a ^= nxt->a;
        drf->f++;
        goto INTERACT;
      }

      // Symmetry breaking after singularity
      // (Sect. 4.6.7)

      drf->reemit = POLE;
      goto INTERACT;
    }

    // Clump bubbles together to form particle pair fragments
    // (Sect. 4.2)

    if (stb->a != nxt->a && stb->f == 1 && nxt->f == 1)
    {
      if (MAT(stb) != MAT(nxt) && W0(stb) == !W0(nxt) &&
          Q(stb) == !Q(nxt))
        drf->kind = GLUON;
      else if (C(stb) == C(nxt) && C(stb) != 0 && C(stb) != 7 &&
               Q(stb) == Q(nxt) && Q(stb) == W1(stb))
        drf->kind = UP;
      else if (C(stb) == _C(nxt) && W0(stb) == !W0(nxt) &&
               Q(stb) == !Q(nxt))
        drf->kind = PHOTON;
      else if (C(stb) == C(nxt) && (C(stb) == 0 || C(stb) == 7) &&
               Q(stb) != Q(nxt) && W0(stb) == W0(nxt) &&
               W0(stb) != W1(stb))
        drf->kind = NEUTRINO;
      else if (C(stb) == _C(nxt) &&
               (W0(stb) != W1(stb) || W0(nxt) != W1(stb)) && Q(stb) == !Q(nxt))
        drf->kind = ZB;
      else if (C(stb) == _C(nxt) && W0(stb) == W0(nxt) &&
               Q(stb) == Q(nxt) && W0(stb) != W1(stb))
        drf->kind = WB;
      else
        goto INTERACT;

      // Calculate the common value for f and a
      // (Sect. 4.2)

      drf->a ^= stb->a;
      drf->f++;

      // Obs.: A pair is actually formed, since the other cell
      // performs this same action in turn.
    }

    // Combine pairs to form multi pairs
    // (Sect. )

    else if (stb->f > 0 && nxt->f > 0 && stb->f % 2 == 0 &&
             nxt->f %2 == 0 && stb->a != nxt->a)
    {
      drf->f += stb->f;
      drf->a ^= stb->a;		// TODO: check mutiple case

      // Obs.: Things fit at the end as above for the first pair
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

  if(drf->reemit)
    goto REEMISSION;

  // Not the last pass of wavefront?

  if (drf->n % LIGHT != 0)
    goto COPY;

  // Find new address for interaction
  // (Sect. 4.2)

  int t = stb->n / LIGHT;	// Physical time
  int offset;
  if(stb->offset % 2 == 0)
	  offset = (stb->offset + t) % SIDE3;
  else
	  offset = (stb->offset - t) % SIDE3;
  Cell *nxt = stb - stb->offset + offset;

  // Internal process?

  if (stb->a == nxt->a && !ZERO(stb->p) && !ZERO(nxt->p))
  {
    // Self interference (Sect. 4.6.3)
    // (Sciarretta)

    int dif[3];
    SUB(dif, drf->o, nxt->o);
    if (DOT(nxt->o, drf->o) == 1 && MOD2(dif) == 0)
    {
      // Trap empodions (Sect. 4.6.3)

      if (nxt->f > 0 && drf->f > 0)
      {
    	  CP(drf->msg, stb->pP);
          goto COPY;
      }
    }
    else
    {
      // Exchange particle x lattice (Sect. 4.6.3)

      if (nxt->f > 0 && drf->f == 0)
      {
        CP(drf->o, nxt->o);
        goto COPY;
      }
      if (nxt->f == 0 && drf->f > 0)
      {
        CP(drf->o, nxt->o);
        goto COPY;
      }
    }
  }

  // Play pseudo dice to decide bubble mixing

  else if (stb->seed < drf->pmf)
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
          // (msg1 != 0, photon2, p2 aligned with msg1)
          // This serves both to electrical or magnetic interaction

          if (!ZERO(nxt->msg) && drf->kind == PHOTON &&
              DOT(nxt->msg, drf->p) == 0)
          {
            // Recruit it. Drfat is now a propeller

            drf->a = nxt->a;
            RESET(drf->msg);
          }

          // Static forces

          else if (nxt->f == 1 && drf->f == 1 && !ZERO(nxt->p))
          {
            if (nxt->seed % 2 == 1)
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
            }
            else
            {
              // Magnetic lateral kick
              // (Sect. 4.7.2)

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
                drf->reemit = CONTACT;
                break;

              case ZB:
              case WB:
                if (MAT(nxt))
                {
                  if (W0(nxt) == LEFT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    	drf->reemit = CONTACT;
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    	drf->reemit = CONTACT;
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
            	  drf->reemit = CONTACT;
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

                    	drf->reemit = CONTACT;
                    }
                  }
                }
                else
                {
                  if (W0(nxt) == RIGHT)
                  {
                    if ((MAT(drf) && W0(drf) == LEFT) ||
                        (!MAT(drf) && W0(drf) == RIGHT))
                    	drf->reemit = CONTACT;
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
            drf->ch &= (C_MASK | W0_MASK);
            drf->ch |= (nxt->ch & (C_MASK | W0_MASK));
            drf->reemit = CONTACT;
          }
          break;

        case 3:    // Inertia

          // Calculate parallel transported pole

          SUB(drf->pole, nxt->o, drf->o);
          RESET(drf->o);
          break;

        case 5:    // Inertia

          // Counterpart in inertia is trivial

        	drf->reemit = CONTACT;
          break;

        case 6:    // Collapse

          // fermion+ x fermion- (annihilation) ?
          // fermion- x fermion+ (annihilation) ?

          if (CMPL(nxt->ch, drf->ch) && nxt->kind == FERMION)
          {
            // Destroy particles, setting each 'a' property to a
            // new, pseudorandom value

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target;  // copy for flash
            drf->kind = 0;         // undo pairing
            drf->f = 1;            // separate fragments
          }
          else if (nxt->kind == FERMION && drf->kind != FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target;  // copy for flash
            drf->kind = 0;         // undo pairing
            drf->f = 1;            // separate fragments
          }
          else if (nxt->kind != FERMION && drf->kind == FERMION)
          {
            // fermion- x boson (light-matter)
            // fermion+ x boson (light-matter)

            drf->a ^= nxt->seed;
            drf->target = drf->a;
            target = drf->target;  // copy for flash
            drf->kind = 0;         // undo pairing
            drf->f = 1;            // separate fragments
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

          drf->reemit = CONTACT;
          drf->target = nxt->a;   // non trivial target
          break;

        case 7:    // Internal collision

        	// Cohesion
        	// (Sect. 4.1)

        	if(nxt->kind == FERMION || nxt->kind == WB || nxt->kind == ZB)
        		drf->reemit = POLE;
          break;
      }
    }

    // Inter-sector interaction
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion

      if (nxt->f == 1 && drf->kind == SPHOTON)
    	  drf->reemit = CONTACT;
      else if (drf->f == 1 && nxt->kind == SPHOTON)
    	  drf->reemit = CONTACT;
    }

    REEMISSION:

    // Prepare re-emission

    if(drf->reemit == CONTACT)
    {
      // Pole is the contact point

      RESET(drf->pole);
      RESET(drf->o);
    }
    else if(drf->reemit == POLE)
    {
      // Pole is the negation of p

      int pole[3];
      CP(pole, drf->p);
      NEG(pole);
      CP(drf->pole, pole);
      RESET(drf->o);
    }

    // Define collapse target

    drf->target = target;
  }

  // Automatically release a flash

  drf->flash = true;

  COPY: ;
}

