/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include "simulation.h"

namespace automaton
{
  const unsigned CONVOL = SIDE + 1;  // +1 needed for temp
  const unsigned COLLISION = CONVOL + 1;
  const unsigned W_DIFFUSION = COLLISION + SIDE;
  const unsigned XYZ_DIFFUSION = W_DIFFUSION + 3 * SIDE / 2;
  const unsigned RELOC = XYZ_DIFFUSION + 3 * SIDE;
  const unsigned UPDATE = CONVOL + COLLISION + XYZ_DIFFUSION + RELOC;
  const unsigned DIAG = (unsigned) SIDE * sqrt(3);
  const unsigned LIGHT = (SIDE - 1) / 2;
  const unsigned RMAX = DIAG / 2;
  const unsigned RANGE = RMAX * LIGHT;
  const unsigned FRAME = UPDATE + LIGHT;
  const unsigned FMAX = RMAX / 2;

  Cell *lattice_main, *lattice_draft, *lattice_mirror;

  /*
   * TODO list
   *
   * - super photon
   * - calculate common affinity
   *         // Compose the common value for affinity.

        	drf->a1 |= (nxt->a1 << ORDER);
        	lst->a1 |= (stb->a1 << ORDER);
   * - neutrino logic
   *
   */

  /*
   * Executes an one tick operation in every cell.
   * (serialized, not parallel ideally)
   */
  void update_lattice()
  {
    Cell *current = lattice_main,
         *draft   = lattice_draft,
         *mirror  = lattice_mirror,
         *nei;
    for (int i = 0; i < SIDE4; i++, current++, draft++, mirror++)
    {
      // Test the UPDATE x EXPANSION toggle bit
      if (current->ctrl)
      {
    	/********* UPDATE step ************/
        /********* CONVOLUTION substep ****/

    	// All the cells must confront each other in the w dimension.
    	// The collected data in this phase are hints for interaction detection.

        if (current->n < CONVOL)
        {
          // First clock tick?
          if (current->n == 0)
          {
            // Mirror the first version of
            // lattice_main in lattice_mirror
            *mirror = *draft;
            // Prepare to convolve charge
            draft->net_c0 = (draft->charge & C0_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->net_c1 = (draft->charge & C1_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->net_c2 = (draft->charge & C2_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->net_w0 = (draft->charge & W0_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->net_w1 = (draft->charge & W1_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->net_q = (draft->charge & Q_MASK) ? SIDE/2-1 : SIDE/2+1;
            draft->boson = true;
          }
          // Wavefronts clash?
          else if (current->wv && mirror->wv)
          {
        	// Get adjacent cell in the w dimension (circular shift)
            *draft = lattice_main[(i + SIDE3) % SIDE4];
            // Calculate the momentum flags.
            // SIDE/2 is the unsigned equivalent of 0
            bool p1 = mirror->p[0] != SIDE/2 ||
                      mirror->p[1] != SIDE/2 ||
                      mirror->p[2] != SIDE/2;
            bool p2 = draft->p[0] != SIDE/2 ||
                      draft->p[1] != SIDE/2 ||
                      draft->p[2] != SIDE/2;
            unsigned char c1  = mirror->charge & COLOR_MASK;
            unsigned char c2  = draft->charge & COLOR_MASK;
            unsigned char w01 = mirror->charge & W0_MASK;
            unsigned char w02 = draft->charge & W0_MASK;
            unsigned char w11 = mirror->charge & W1_MASK;
            unsigned char w12 = draft->charge & W1_MASK;
            // Compute the weak force
            bool wf1 = w01 != w11;
            bool wf2 = w02 != w12;
            // Check if partners are in different sectors (Orbis, Umbra)
            bool stranges = w11 != w12;
            // Bubbles are superposing?
            if (mirror->x == draft->x &&
                mirror->y == draft->y &&
                mirror->z == draft->z)
            {
              // Symmetry breaking (w_11 != w12)
              if (stranges)
              {
            	// Poles coincide?
                if (p1 && p2)
                {
            	  // Segregation Orbis <-> Umbra:
                  //   force immediate re-emission
                  draft->reloc = true;     // hint collision
                  // Hint relocation via fxf (can be confusing???? TODO)
                  draft->fxf = true;
            	  continue;
                }
              }
              // Compile blindly net charges
              if (mirror->charge & C0_MASK)
                draft->net_c0++;
              else
                draft->net_c0--;
              //
              if (mirror->charge & C1_MASK)
                draft->net_c1++;
              else
                draft->net_c1--;
              //
              if (mirror->charge & C2_MASK)
                draft->net_c2++;
              else
                draft->net_c2--;
              //
              if (mirror->charge & Q_MASK)
                draft->net_q++;
              else
                draft->net_q--;
              //
              if (mirror->charge & W0_MASK)
                draft->net_w0++;
              else
                draft->net_w0--;
              //
              if (mirror->charge & W1_MASK)
                draft->net_w1++;
              else
                draft->net_w1--;
              // boson x non boson
              if (draft->aff != mirror->aff)
                draft->boson = false;  // There is a rogue: invalidate
              // After the full circular shift, the rogue will have
              // spoiled all cells along the way
            }	// >>> end bubbles superposing
            // Intersector operations
            if (stranges)
            {
              // TODO
              // Not all stranges have segregated maybe
              assert(0); // investigate now!
              continue;
            }
            // Both non trivial momenta?
            if (p1 && p2)
            {
              draft->collapse = true;  // hint enforce collapse
              draft->reloc = true;     // hint collision
              // Opposite charges? (disregard w1 in this comparison)
              if (((current->charge ^ mirror->charge) & 0x1F) == 0x1F)
              {
                // Hint annihilation
                draft->fxf = true;
              }
              else if (draft->boson)
              {
                // Hint fermion x boson
                draft->fxb = true;
              }
            }
            // At least one has non trivial momentum
            else if (p1 || p2)
            {
              // We are in case (a) (see paper)
              // Are both bosons?
              if (current->boson && mirror->boson)
              {
            	// Test boson cohesion:
            	//
                // Colored partners?
                if (c1 > 0 && c1 < 7 && c2 > 0 && c2 < 7)
                {
                  // Check resultant color neutrality
                  if (isColorNeutral(current->charge, mirror->charge))
                  {
                    // Hint gluon x gluon
                    draft->bxb = true;
                  }
                }
                // Both have weak force?
                else if (wf1 && wf2)
                {
                  // Must have similar electric charges
                  if ((current->charge ^ mirror->charge) == 0)
                  {
                    // Hint W, Z boson
                    draft->bxb = true;
                  }
                }
              }
              // Assume now that none are bosons in case (a) or (c).
              // Same charge?
              else if (draft->charge == mirror->charge)
              {
                // Hint fermion cohesion
                draft->fxf = true;
              }
              // Do we have already a hint for interaction?
              if (draft->fxf || draft->fxb || draft->bxb)
              {
            	assert(draft->reloc); // remove after tests
            	draft->reloc = true;  // keep or remove?
                // Calculate look ahead value of relocation to c.p.
                draft->cx = (current->x + SIDE - SIDE / 2) % SIDE;
                draft->cy = (current->y + SIDE - SIDE / 2) % SIDE;
                draft->cz = (current->z + SIDE - SIDE / 2) % SIDE;
                // Is this bubble the slave?
                if (p1 && !draft->fxf)
                {
                  // Case (c):
                  // Calculate look ahead value for parallel transport
                  draft->dx = (mirror->x + SIDE - draft->x) % SIDE;
                  draft->dy = (mirror->y + SIDE - draft->y) % SIDE;
                  draft->dz = (mirror->z + SIDE - draft->z) % SIDE;
                }
              }
            }    // >>> end at least one has no trivial momentum
          }    // >>> end wavefront clash
        }

        /******** COLLISION ********/

        // Collision window is just one tick

        else if (current->n == COLLISION - 1)
        {
          // Change hints into decision
          bool fxb = current->fxb && current->boson;
          bool bxb = current->bxb && current->boson;
          bool fxf = current->fxf && !current->boson;
          if (fxb || fxf || bxb)
          {
       	    // Enforce relocation
            draft->reloc = true;
          }
          else
          {
        	// Disarm collapse, interaction did not complete
        	draft->collapse = false;
          }
        }

        /******* W DIFFUSION ********/

        // If a collapse occurred as a result of the interaction, all
        // the related (same affinity) fragments must be notified.

        else if (current->n < W_DIFFUSION && current->n >= XYZ_DIFFUSION)
        {
          // Probe neighbors
          for (int dir = 0; dir < 6; dir++)
          {
            nei = get_neighbor(i, dir);
            if (nei->collapse)
            {
              draft->collapse = true;
              draft->reloc = true;
            }
          }
        }

        /******* XYZ DIFFUSION *******/

        // For relocation to occur, all the cells in the layer must
        // receive information about the final destination.

        else if (current->n < XYZ_DIFFUSION && current->n >= COLLISION)
        {
          // Check von Neumann directions
          for (int dir = 0; dir < 6; dir++)
          {
            nei = get_neighbor(i, dir);
            if (nei->reloc)
            {
              draft->reloc = true;
              draft->cx = nei->cx;
              draft->cy = nei->cy;
              draft->cz = nei->cz;
              draft->freq = nei->freq;
              draft->amplitude = 0;
              break;
            }
            // Propagate phase to all cells
            else if (current->amplitude == 0 && nei->amplitude > 0)
            {
              draft->amplitude = nei->amplitude;
            }
          }
        }
        /******** RELOCATION *******/

        // The actual relocation is done in three phases, along
        // the x, y, and z axes.
        else
        {
          // The rest of the UPDATE step is used for
          // Execute bubble relocation to c.p.
          if (draft->cx > 0)
          {
            nei = get_neighbor(i, NORTH);
            *draft = *nei;
            draft->reloc = false;
            draft->cx--;
          }
          else if (draft->cy > 0)
          {
            nei = get_neighbor(i, WEST);
            *draft = *nei;
            draft->reloc = false;
            draft->cy--;
          }
          else if (draft->cz > 0)
          {
            nei = get_neighbor(i, DOWN);
            *draft = *nei;
            draft->reloc = false;
            draft->cz--;
          }
          // Execute bubble parallel transport
          else if (draft->dx > 0)
          {
            nei = get_neighbor(i, NORTH);
            *draft = *nei;
            draft->reloc = false;
            draft->dx--;
          }
          else if (draft->dy > 0)
          {
            nei = get_neighbor(i, WEST);
            *draft = *nei;
            draft->reloc = false;
            draft->dy--;
          }
          else if (draft->dz > 0)
          {
            nei = get_neighbor(i, DOWN);
            *draft = *nei;
            draft->reloc = false;
            draft->dz--;
          }
          // Reset wavefront
          draft->wv = false;
          draft->amplitude = 0;
        }
        // Check the end of the update step
        draft->n++;
        if (draft->n == UPDATE)
        {
          draft->n = 0;
          draft->ctrl = false;
        }
      }

      /******** EXPANSION *********/

      // Here, the wavefront accommodates in this light step.

      else
      {
        // Detect wavefront
        if (current->m == current->d)
          draft->wv = true;
        // Detect new phase value
        if (current->angle == current->d)
          draft->amplitude = current->sin;
        // Update phase angle
        draft->angle += current->freq;
        if (draft->angle >= RANGE)
          draft->angle -= RANGE;
        // Bump light timing
        draft->m++;
        // Check light step
        if (draft->m % LIGHT == 0)
          draft->ctrl = true;
        // Test ultimate wrapping
        if (draft->m == RANGE)
        {
          draft->m = 0;
          draft->angle = 0;
        }
        // Decay empodions
        if (draft->e)
        {
          // Empodion is free, aff receives default value
          draft->aff = draft->d ^ draft->sin;
        }
        // Prepare for next interaction
        draft->net_c0 = 0;
        draft->net_c1 = 0;
        draft->net_c2 = 0;
        draft->net_q = 0;
        draft->net_w0 = 0;
        draft->net_w1 = 0;
        draft->boson = 0;
        draft->fxf = false;
        draft->collapse = false;
        draft->dx = 0;
        draft->dy = 0;
        draft->dz = 0;
      }
    }
  }

  /*
   * CA basics.
   */
  void swap_lattices()
  {
    Cell *main_ptr = lattice_main;
    Cell *draft_ptr = lattice_draft;
      for (int i = 0; i < SIDE4; i++)
      *main_ptr++ = *draft_ptr++;
  }

  /*
   * One step of the CA.
   */
  void simulation()
  {
    update_lattice();
    swap_lattices();
  }
}
