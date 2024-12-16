/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */
#include "simul.h"

namespace automaton_back
{
  using namespace std;

//  extern EntropyCalculator entropyCalc;

  // Grid constants
  const unsigned SIDE3 = SIDE2 * SIDE;
  const unsigned BLOCK = SIDE3 * W_DIM;

  // Dynamics constants
  const unsigned DIAG        = (unsigned) SIDE * sqrt(3);
  const unsigned RMAX        = DIAG / 2;
  const unsigned FMAX        = RMAX / 2;
  const unsigned CONVOL      = W_DIM + 1 + 1;
  const unsigned COLLISION   = CONVOL + 1;
  const unsigned W_DIFFUSION = COLLISION + W_DIM - 1;
  const unsigned X_DIFFUSION = W_DIFFUSION + (SIDE - 1);
  const unsigned Y_DIFFUSION = X_DIFFUSION + (SIDE - 1);
  const unsigned Z_DIFFUSION = Y_DIFFUSION + (SIDE - 1);
  const unsigned RELOCATION  = Z_DIFFUSION + 3*(SIDE - 1);
  const unsigned TRANSPORT   = RELOCATION + 3*(SIDE - 1);
  const unsigned UPDATE      = TRANSPORT;
  const unsigned LIGHT       = (SIDE - 1) / 2;
  const unsigned FRAME       = UPDATE + LIGHT;
  const unsigned RANGE       = RMAX * LIGHT;

  // The CA lattices
  Cell lattice_curr   [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_draft  [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_mirror [SIDE][SIDE][SIDE][W_DIM];

  /*
   * Executes a one tick operation in every cell.
   */
  void update_lattice()
  {
    // Sweep each layer
    // Simulate a parallel execution in all cells
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Sweep a 3D space
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            Cell &curr   = lattice_curr[x][y][z][w];
            Cell &draft  = lattice_draft[x][y][z][w];
            Cell &mirror = lattice_mirror[x][y][z][w];

            /****** CONVOLUTION ******/
            if (curr.k < CONVOL)
            {
              if (curr.k == 0)
              {
                // Copy current to mirror on first tick
                mirror = curr;
                // Prepare to convolve charge
                draft.net_c0 = (curr.charge & C0_MASK) ? CENTER-1 : CENTER+1;
                draft.net_c1 = (curr.charge & C1_MASK) ? CENTER-1 : CENTER+1;
                draft.net_c2 = (curr.charge & C2_MASK) ? CENTER-1 : CENTER+1;
                draft.net_w0 = (curr.charge & W0_MASK) ? CENTER-1 : CENTER+1;
                draft.net_w1 = (curr.charge & W1_MASK) ? CENTER-1 : CENTER+1;
                draft.net_q  = (curr.charge & Q_MASK)  ? CENTER-1 : CENTER+1;
                draft.boson = true;
              }
              else if (curr.wv && mirror.wv)
              {
                // Handle wavefront clash
            	//          unsigned w_left = (w == 0) ? W_DIM-1 : w-1;
            	//        unsigned w_right = (w == W_DIM-1) ? 0 : w+1;
            	//         if (convolute(curr, draft, mirror, lattice_mirror[x][y][z][w_left],
            	//            lattice_mirror[x][y][z][w_right]))
           	    //         continue;
//                seggregation(curr, draft, mirror);
              }
              /*
              else if (curr.k == CONVOL - 1)
              {
                // Is a slave?
                if (!curr.pole)
                {
                  // Copy back relocation
                  copy(begin(mirror.c), end(mirror.c), std::begin(draft.c));
                  // Copy back parallel transport
                  copy(begin(mirror.m), end(mirror.m), std::begin(draft.m));
                }
              }
              */
            }
            /****** COLLISION ******/
            else if (curr.k < COLLISION)
            {
//              executeInteraction(curr, draft);
              // Time is reset at the pattern central cell
              if (0&&curr.fxf && curr.pos[0] == CENTER && curr.pos[1] == CENTER && curr.pos[2] == CENTER)
              {
                // TODO: angle etc.
                draft.t = 0;
                printf("t=0 in w=%u\n", w);fflush(stdout);
              }
            }
            /****** W DIFFUSION ******/
            else if (curr.k < W_DIFFUSION)
            {
            	/*
              Cell &nei = lattice_curr[x][y][z][(w == 0) ? SIDE-1 : w-1];
              if (nei.collapse)
              {
                // Propagate the relocation offset
                copy(begin(nei.c), end(nei.c), std::begin(draft.c));
                // Propagate collapse bit
                draft.collapse = true;
              }
              */
              // Lit down the wavefront flag
              draft.wv = false;
            }
            /****** X DIFFUSION ******/
            else if (curr.k < X_DIFFUSION)
            {
              // This will happen SIDE-1 times in the X DIFFUSION window:
              // Retrieve the neighbor address
              Cell &nei = lattice_curr[(x == 0) ? SIDE-1 : x-1][y][z][w];
              // The greatest value prevails
              // This accounts for concomitant relocations
              draft.c[0] = max(curr.c[0], nei.c[0]);
              // The spatial spread of collapse bit is needed for fermion collapse
              // Cells from orphans do not propagate collapse
              if (nei.collapse && curr.aff != W_DIM)
                draft.collapse = true;
            }
            /****** Y DIFFUSION ******/
            else if (curr.k < Y_DIFFUSION)
            {
              // This will happen SIDE-1 times in the Y DIFFUSION window:
              // Retrieve the neighbor address
              Cell &nei = lattice_curr[x][(y == 0) ? SIDE-1 : y-1][z][w];
              // The greatest value prevails
              // This accounts for concomitant relocations
              draft.c[1] = max(curr.c[1], nei.c[1]);
              // The spatial spread of collapse bit is needed for fermion collapse.
              // Cells from orphans do not propagate collapse
              if (nei.collapse && curr.aff != W_DIM)
                draft.collapse = true;
            }
            /****** Z DIFFUSION ******/
            else if (curr.k < Z_DIFFUSION)
            {
              // This will happen SIDE-1 times in the Z DIFFUSION window:
              // Retrieve the neighbor address
              Cell &nei = lattice_curr[x][y][(z == 0) ? SIDE-1 : z-1][w];
              // The greatest value prevails
              // This accounts for concomitant relocations
              draft.c[2] = max(curr.c[2], nei.c[2]);
              // The spatial spread of collapse bit is needed for fermion collapse
              // Cells from orphans do not propagate collapse
              if (nei.collapse && curr.aff != W_DIM)
                draft.collapse = true;
            }
            /****** RELOCATION ******/
            else if (curr.k < RELOCATION)
            {
              // This test is evaluated 3*(SIDE-1) times in the RELOCATION window
              if (curr.c[0] > 0)
              {
                draft = lattice_curr[(x + SIDE - 1) % SIDE][y][z][w];
                draft.c[0]--;

              }
              else if (curr.c[1] > 0)
              {
                draft = lattice_curr[x][(y + SIDE - 1) % SIDE][z][w];
                draft.c[1]--;
              }
              else if (curr.c[2] > 0)
              {
                draft = lattice_curr[x][y][(z + SIDE - 1) % SIDE][w];
                draft.c[2]--;
              }
              //draft.A.a = 0;
            }
            /****** PARALLEL TRANSPORT ******/
            else if (curr.k < TRANSPORT)
            {
            	/*
              // Execute bubble parallel transport
              if (curr.c[0] > 0)
              {
                unsigned src_x = (x == 0) ? SIDE - 1 : x - 1;
                lattice_draft[x][y][z][w] = lattice_curr[src_x][y][z][w];
                draft.c[0] = curr.c[0] - 1;
              }
              else if (curr.c[1] > 0)
              {
                unsigned src_y = (y == 0) ? SIDE - 1 : y - 1;
                lattice_draft[x][y][z][w] = lattice_curr[x][src_y][z][w];
                draft.c[1] = curr.c[1] - 1;
              }
              else if (curr.c[2] > 0)
              {
                unsigned src_z = (z == 0) ? SIDE - 1 : z - 1;
                lattice_draft[x][y][z][w] = lattice_curr[x][y][src_z][w];
                draft.c[2] = curr.c[2] - 1;
              }
              */
            }
            /***********************/
            /*     EXPANSION       */
            /*    (Light pass)     */
            /***********************/
            else
            {
              // Collapse test
              if (curr.collapse)
              {
                // Dissolve the particle
                draft.aff = w;
                draft.collapse = false;
              }
              // Awake the wavefront
              if (curr.t == curr.d)
                draft.wv = true;
              // Check and update from x-dimension neighbors with wrapping
              unsigned x_left  = (x == 0) ? SIDE-1 : x-1;
              unsigned x_right = (x == SIDE-1) ? 0 : x+1;
              if (lattice_curr[x_left][y][z][w].freq > 0)
                  draft.freq = lattice_curr[x_left][y][z][w].freq;
              if (lattice_curr[x_right][y][z][w].freq > 0)
                  draft.freq = lattice_curr[x_right][y][z][w].freq;
              // Check and update from y-dimension neighbors with wrapping
              unsigned y_up   = (y == 0) ? SIDE-1 : y-1;
              unsigned y_down = (y == SIDE-1) ? 0 : y+1;
              if (lattice_curr[x][y_up][z][w].freq > 0)
                draft.freq = lattice_curr[x][y_up][z][w].freq;
              if (lattice_curr[x][y_down][z][w].freq > 0)
                draft.freq = lattice_curr[x][y_down][z][w].freq;
              // Check and update from z-dimension neighbors with wrapping
              unsigned z_back = (z == 0) ? SIDE-1 : z-1;
              unsigned z_front = (z == SIDE-1) ? 0 : z+1;
              if (lattice_curr[x][y][z_back][w].freq > 0)
                draft.freq = lattice_curr[x][y][z_back][w].freq;
              if (lattice_curr[x][y][z_front][w].freq > 0)
                draft.freq = lattice_curr[x][y][z_front][w].freq;
              // Retrieve sine information
              if (curr.angle == curr.d)
                draft.A.a = curr.sin;
              // Compute angle for sine
              draft.angle += curr.freq;
              // Wrap the angle
              if (draft.angle >= RANGE)
                draft.angle -= RANGE;
              // Empodion decay
              if ((curr.d ^ curr.sin) % LIGHT == 0)
              {
                draft.e = false;
                // Affinity assumes its default value
                draft.aff = w;
              }
              // Increment the clock
              draft.t = curr.t + 1;
              // Light pass completed?
              if (draft.t % LIGHT == 0)
              {
                draft.ph.a >>= 1;
                // Test time wrapping
                if (draft.t == RANGE)
                {
                  // TODO: angle etc.
                  draft.t = 0;
                }
              }
              // Prepare everything for a new FRAME
              draft.net_c0 = draft.net_c1 = draft.net_c2 = 0;
              draft.net_q = draft.net_w0 = draft.net_w1 = 0;
              draft.boson = false;
              draft.fxf = false;
              fill(begin(draft.c), end(draft.c), 0);
              fill(begin(draft.m), end(draft.m), 0);
            }
            // Update the superluminal timer
            draft.k = curr.k + 1;
            if (draft.k == FRAME)
           	  draft.k = 0;
          }
        }
      }
    }
  }


}
