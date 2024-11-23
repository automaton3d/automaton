/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include "simulation.h"

namespace automaton
{
  // Grid constants
  const unsigned SIDE2 = SIDE * SIDE;
  const unsigned SIDE3 = SIDE2 * SIDE;
  const unsigned W_DIM = 3 * SIDE2;
  const unsigned BLOCK = SIDE3 * W_DIM;
  // Dynamics constants
  const unsigned CONVOL = 2;// W_DIM + 1;  // +1 needed for temp
  const unsigned COLLISION = CONVOL + 1;
  const unsigned W_DIFFUSION = COLLISION + SIDE;
  const unsigned XYZ_DIFFUSION = W_DIFFUSION + 3 * CENTER;
  const unsigned RELOC = XYZ_DIFFUSION + 3 * SIDE;
  const unsigned UPDATE = CONVOL + COLLISION + XYZ_DIFFUSION + RELOC;
  const unsigned DIAG = (unsigned) SIDE * sqrt(3);
  const unsigned LIGHT = (SIDE - 1) / 2;
  const unsigned RMAX = DIAG / 2;
  const unsigned RANGE = RMAX * LIGHT;
  const unsigned FRAME = UPDATE + LIGHT;
  const unsigned FMAX = RMAX / 2;

  Cell *lattice_current, *lattice_draft, *lattice_mirror;

  extern Entropy entropy;

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
    Cell *current = lattice_current,
         *draft   = lattice_draft,
         *mirror  = lattice_mirror,
         *nei;
    for (unsigned i = 0; i < BLOCK; i++, current++, draft++, mirror++)
    {
    	unsigned z = i / SIDE3;
      // Test the UPDATE x EXPANSION toggle bit
      if (current->ctrl)
      {
    	/********* UPDATE step ************/
        /********* CONVOLUTION substep ****/

    	// All the cells must confront each other in the w dimension.
    	// The collected data in this phase are hints for interaction detection.

        if (current->k < CONVOL)
        {
          // First clock tick?
          if (current->k == 0)
          {
            // Mirror the first version of
            // lattice_main in lattice_mirror
            *mirror = *draft;
            // Prepare to convolve charge
            draft->net_c0 = (draft->charge & C0_MASK) ? CENTER-1 : CENTER+1;
            draft->net_c1 = (draft->charge & C1_MASK) ? CENTER-1 : CENTER+1;
            draft->net_c2 = (draft->charge & C2_MASK) ? CENTER-1 : CENTER+1;
            draft->net_w0 = (draft->charge & W0_MASK) ? CENTER-1 : CENTER+1;
            draft->net_w1 = (draft->charge & W1_MASK) ? CENTER-1 : CENTER+1;
            draft->net_q = (draft->charge & Q_MASK) ? CENTER-1 : CENTER+1;
            draft->boson = true;
          }
          // Wavefronts clash?
          else if (current->wv && mirror->wv)
          {
//#define PRODUCTION
#ifdef PRODUCTION
            // Ok, we have two active wavefront cells.
            // Get pointers to the neighbors at the mirror lattice
            Cell *right = &lattice_mirror[(i + SIDE3) % BLOCK];
            Cell *left = &lattice_mirror[(i - SIDE3) % BLOCK];
            // Calculate the momentum flags in mirror.
            // CENTER is the unsigned equivalent of 0)
            bool p_current =
              current->p[0] != CENTER ||
			  current->p[1] != CENTER ||
			  current->p[2] != CENTER;
            bool p_mirror =
              mirror->p[0] != CENTER ||
              mirror->p[1] != CENTER ||
              mirror->p[2] != CENTER;
            bool p_left =
              left->p[0] != CENTER ||
              left->p[1] != CENTER ||
              left->p[2] != CENTER;
            bool p_right =
              right->p[0] != CENTER ||
              right->p[1] != CENTER ||
              right->p[2] != CENTER;
            // Separate the charges
            unsigned char c_current  = current->charge & COLOR_MASK;
            unsigned char c_mirror   = mirror->charge  & COLOR_MASK;
            unsigned char c_right    = right->charge   & COLOR_MASK;
            unsigned char c_left     = left->charge    & COLOR_MASK;

            unsigned char w0_current = current->charge & W0_MASK;
            unsigned char w0_mirror  = mirror->charge  & W0_MASK;
            unsigned char w0_right   = right->charge   & W0_MASK;
            unsigned char w0_left    = left->charge    & W0_MASK;

            unsigned char w1_current = current->charge & W1_MASK;
            unsigned char w1_mirror  = mirror->charge  & W1_MASK;
            unsigned char w1_right   = right->charge   & W1_MASK;
            unsigned char w1_left    = left->charge    & W1_MASK;

            // Compute the weak force
            bool wf_current = w0_current != w1_current;
            bool wf_mirror  = w0_mirror != w1_mirror;
            bool wf_right   = w0_right != w1_right;
            bool wf_left    = w0_left != w1_left;
            // Bubbles are superposing?
            // Radius > zero?
            if (mirror->pos[0] == current->pos[0] &&
                mirror->pos[1] == current->pos[1] &&
                mirror->pos[2] == current->pos[2] && current->t > LIGHT)
            {
              if (p_current)
              {
            	  // Current momentum non trivial.
                  // Belong to different sectors?
                  if (!current->wxw && w1_mirror != w1_current)
                  {
                    // Segregation Orbis <-> Umbra:
                    //  force immediate re-emission
                    draft->wxw = true;
                    draft->c[0] = (current->pos[0] + SIDE - CENTER) % SIDE;
                    draft->c[1] = (current->pos[1] + SIDE - CENTER) % SIDE;
                    draft->c[2] = (current->pos[2] + SIDE - CENTER) % SIDE;
                    draft->boson = false;		// invalidate boson check
                    draft->collapse = false;	// invalidate collapse check
               	    continue;
                  }
              }
              else
              {
            	  // Current momentum is trivial.
                  // Belong to different sectors?
                  if (!current->wxw && p_left && w1_mirror != w1_left)
                  {
                      // Segregation Orbis <-> Umbra:
                      //  force immediate re-emission
                      draft->wxw = true;
                      draft->c[0] = (current->pos[0] + SIDE - CENTER) % SIDE;
                      draft->c[1] = (current->pos[1] + SIDE - CENTER) % SIDE;
                      draft->c[2] = (current->pos[2] + SIDE - CENTER) % SIDE;
                      draft->boson = false;		// invalidate boson check
                      draft->collapse = false;	// invalidate collapse check
                 	    continue;
                  }
              }
              // Count charges in dimension W
              compileNetCharges(mirror, draft);
              // boson x non boson
              if (draft->aff != mirror->aff)
                draft->boson = false;  // There is a rogue: invalidate
              // After the full circular shift, the rogue will have
              // spoiled all cells along the way
            }
            // Boson average amplitude
            draft->A_bar.a += mirror->A.a;
            draft->A_bar.a >>= 1;
            // Intersector operations
            if (w1_mirror != w1_right)
            {
              draft->wxw = true;
              continue;
            }
            // Both non trivial momenta?
            if (p_mirror && p_current)
            {
              draft->collapse = true;  // hint enforce collapse
              draft->reloc = true;     // hint collision
              // Opposite charges? (disregard w1 in this comparison)
              if (((current->charge ^ mirror->charge) & 0x1F) == 0x1F)
              {
                // Hint annihilation
                draft->fxf = true;
              }
              else if (current->boson)
              {
                // Hint fermion x boson
                draft->fxb = true;
              }
            }
            // At least one has non trivial momentum
            else if (p_mirror || p_left)
            {
              // We are in case (a) (see paper)
              // Are both bosons?
              if (current->boson && mirror->boson)
              {
            	// Test boson cohesion:
            	//
                // Colored partners?
                if (c_mirror > 0 && c_mirror < 7 && c_current > 0 && c_current < 7)
                {
                  // Check resultant color neutrality
                  if (isColorNeutral(current->charge, mirror->charge))
                  {
                    // Hint gluon x gluon
                    draft->bxb = true;
                  }
                }
                // Both have weak force?
                else if (wf_mirror && wf_current)
                {
                  // Must have similar electric charges
                  if (((current->charge ^ mirror->charge) & Q_MASK) == 0)
                  {
                    // Hint W, Z boson
                    draft->bxb = true;
                  }
                }
              }
              // Assume now that none are bosons in case (a) or (c).
              // Same charge?
              else if (((current->charge ^ mirror->charge) & Q_MASK) == 0)
              {
                // Hint fermion cohesion
                draft->fxf = true;
              }
              // Do we have already a hint for interaction?
              if (draft->fxf || draft->fxb || draft->bxb)
              {
            	draft->reloc = true;  // keep or remove?
                // Calculate look ahead value of relocation to c.p.
                draft->c[0] = (current->pos[0] + SIDE - CENTER) % SIDE;
                draft->c[1] = (current->pos[1] + SIDE - CENTER) % SIDE;
                draft->c[2] = (current->pos[2] + SIDE - CENTER) % SIDE;
                // Is this bubble the slave?
                if (p_left && !draft->fxf)
                {
                  // Case (c):
                  // Calculate look ahead value for parallel transport
                  draft->m[0] = (mirror->pos[0] + SIDE - draft->pos[0]) % SIDE;
                  draft->m[1] = (mirror->pos[1] + SIDE - draft->pos[1]) % SIDE;
                  draft->m[2] = (mirror->pos[2] + SIDE - draft->pos[2]) % SIDE;
                }
              }
            }
#endif
          }
          // Execute a right circular shift in dimension W of the draft lattice
          // This shift will be completed during the swap_lattices() call
          draft = lattice_draft + (draft - lattice_draft + SIDE3) % BLOCK;
        }

        /******** COLLISION ********/

        // Collision window is just one tick
        else if (current->k == COLLISION - 1)
        {
#ifdef PRODUCTION
          // Change hints into decision
          bool fxb = current->fxb && current->boson;
          bool bxb = current->bxb && current->boson;
          bool fxf = current->fxf && !current->boson;
          bool wxw = current->wxw;
          if (wxw)
          {
            puts("\texecute segregation");
       	    // Execute segregation
            draft->reloc = true;
            // Segregation avoids collapse
            draft->collapse = false;
          }
          // This test covers annihilation, fermion cohesion and boson cohesion
          else if (fxf || bxb)
          {
            // Test threshold
            if (current->A.a > CENTER && current->ph.a > CENTER)
            {
       	      // Enforce relocation
              draft->reloc = true;
            }
          }
          // This test covers static forces and light-matter interaction
          else if (fxb)
          {
       	    // Enforce relocation
            draft->reloc = true;
          }
          else
          {
            // Disarm collapse, interaction did not complete
            draft->collapse = false;
          }
#else

          if (current->pole && current->t % LIGHT == 0 && rand() % 5000 == 0)
          {
//        	  printf("t=%u\n", current->t)
        	  /*;
          	draft->reloc = true;  // keep or remove?
            draft->c[0] = rand() % SIDE;
            draft->c[1] = rand() % SIDE;
            draft->c[2] = rand() % SIDE;
            draft->t = 1
            */;
          }
#endif
        }

        /******* W DIFFUSION ********/

        // If a collapse occurred as a result of the interaction, all
        // the related (same affinity) fragments must be notified.

        else if (current->k < W_DIFFUSION && current->k >= XYZ_DIFFUSION)
        {
#ifdef PRODUCTION
          // Probe neighbors
          for (int dir = 0; dir < 6; dir++)
          {
            nei = get_neighbor(i, dir);
            if (nei->collapse)
            {
              // Propagate collapse bit
              draft->collapse = true;
              draft->reloc = true;
            }
          }
#endif
        }

        /******* XYZ DIFFUSION *******/

        // For relocation to occur, all the cells in the layer must
        // receive information about the final destination.

        else if (current->k < XYZ_DIFFUSION && current->k >= COLLISION)
        {
          // Check von Neumann directions
          if (!current->reloc)
          {
            for (int dir = 0; dir < 6; dir++)
            {
              nei = get_neighbor(lattice_current, i, dir);
              if (nei->reloc)
              {
                draft->reloc = true;
                draft->c[0] = nei->c[0];
                draft->c[1] = nei->c[1];
                draft->c[2] = nei->c[2];
                draft->freq = nei->freq;
                draft->A.a = 0;
                break;
              }
              // Propagate phase to all cells
              else if (current->A.a == 0 && nei->A.a > 0)
              {
                draft->A.a = nei->A.a;
              }
            }
          }
        }
        /******** RELOCATION *******/

        // The actual relocation is done in three phases, along
        // the x, y, and z axes.
        else
        {
          // Now bubble relocation to c.p.
          nei = get_neighbor(lattice_current, i, NORTH);
          if (nei->c[0] > 0)
          {
        	relocate(draft, nei);
            draft->c[0]--;
            printf("%u,%u,%u: k=%u z=%u\n", draft->c[0], draft->c[1], draft->c[2], current->k, z); fflush(stdout);
          }
          nei = get_neighbor(lattice_current, i, WEST);
          if (nei->c[1] > 0)
          {
          	relocate(draft, nei);
            draft->c[1]--;
          }
          nei = get_neighbor(lattice_current, i, DOWN);
          if (nei->c[2] > 0)
          {
          	relocate(draft, nei);
            draft->c[2]--;
          }
#ifdef PRODUCTION
          // Execute bubble parallel transport
          else if (draft->m[0] > 0)
          {
            nei = get_neighbor(lattice_current, i, NORTH);
        	relocate(draft, nei);
            draft->reloc = false;
            draft->m[0]--;
          }
          else if (draft->m[1] > 0)
          {
            nei = get_neighbor(lattice_current, i, WEST);
        	relocate(draft, nei);
            draft->reloc = false;
            draft->m[1]--;
          }
          else if (draft->m[2] > 0)
          {
            nei = get_neighbor(lattice_current, i, DOWN);
        	relocate(draft, nei);
            draft->reloc = false;
            draft->m[2]--;
          }
#endif
          // Reset wavefront
          draft->wv = false;
          draft->A.a = 0;
        }
        // Check the end of the update step
        draft->k++;
        if (draft->k == UPDATE)
        {
          draft->k = 0;
          draft->ctrl = false;
        }
      }

      /******** EXPANSION *********/

      // Here, the wavefront accommodates in this light step.

      else
      {
        // Detect wavefront
        if (current->t == current->d)
          draft->wv = true;


        // Check neighbors with information
        for (int dir = 0; dir < 6; dir++)
        {
            nei = get_neighbor(lattice_current, i, dir);
            if (nei->freq > 0)
            {
            	draft->charge = nei->charge;
            }
        }



        // Detect new phase value
        if (current->angle == current->d)
          draft->A.a = current->sin;
        // Update phase angle
        draft->angle += current->freq;
        if (draft->angle >= RANGE)
          draft->angle -= RANGE;
        // Bump light timing
        draft->t++;
        // Check light step
        if (draft->t % LIGHT == 0)
        {
          draft->ctrl = true;
          // Decay electromagnetic phase
          draft->ph.a >>= 1;
        }
        // Test ultimate wrapping
        if (draft->t == RANGE)
        {
          draft->t = 0;
          draft->angle = 0;
        }
        // Decay empodions
        if ((draft->d ^ draft->sin) % LIGHT == 0)
        {
          draft->e = false;
          // Empodion is free, a receives default value
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
        draft->reloc    = false;
        draft->fxf      = false;
        draft->collapse = false;
        draft->m[0] = 0;
        draft->m[1] = 0;
        draft->m[2] = 0;
      }
    }
  }

  /*
   * CA basics.
   */
  void swap_lattices()
  {
    Cell *main_ptr = lattice_current;
    Cell *draft_ptr = lattice_draft;
    for (unsigned i = 0; i < BLOCK; i++)
      *main_ptr++ = *draft_ptr++;
  }

  /*
   * One step of the CA.
   */
  void simulation()
  {
    update_lattice();
    swap_lattices();

#define XXX
#ifdef XXX
    // Update entropy
    if (framework::timer % FRAME == 0)
	{
    	collectData();
	}
    if (framework::timer % (FRAME * ERA) == 0)
    {
    	double H = computeEntropy();
    	printf("H=%f\n", H);	// debug
    	Beep(750, 300);
    	entropy.add(H);
    }
#else
    // Update entropy
    if (framework::timer % FRAME == 0)
	{
    	collectData();
	}
    if (framework::timer % (FRAME * ERA) == 0)
    {
    	double H = computeEntropy();
    	assert(H >= 0);
    	printf("H=%f\n", H);	// debug
    }
#endif
  }
}
