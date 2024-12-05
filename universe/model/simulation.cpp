/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */
#include "simulation.h"

namespace automaton
{
  using namespace std;

  extern EntropyCalculator entropyCalc;

  // Grid constants
  const unsigned SIDE3 = SIDE2 * SIDE;
  const unsigned BLOCK = SIDE3 * W_DIM;

  // Dynamics constants
  const unsigned DIAG        = (unsigned) SIDE * sqrt(3);
  const unsigned RMAX        = DIAG / 2;
  const unsigned FMAX        = RMAX / 2;
  const unsigned CONVOL      = W_DIM + 1;  // +1 needed for temp
  const unsigned COLLISION   = CONVOL + 1;
  const unsigned W_DIFFUSION = COLLISION + W_DIM;
  const unsigned X_DIFFUSION = W_DIFFUSION + (SIDE - 1);
  const unsigned Y_DIFFUSION = X_DIFFUSION + (SIDE - 1);
  const unsigned Z_DIFFUSION = Y_DIFFUSION + (SIDE - 1);
  const unsigned X_RELOC     = Z_DIFFUSION + (SIDE - 1);
  const unsigned Y_RELOC     = X_RELOC + (SIDE - 1);
  const unsigned Z_RELOC     = Y_RELOC + (SIDE - 1);
  const unsigned UPDATE      = Z_RELOC;
  const unsigned LIGHT       = (SIDE - 1) / 2;
  const unsigned FRAME       = UPDATE + LIGHT;
  const unsigned RANGE       = RMAX * LIGHT;

  Cell lattice_curr   [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_draft  [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_mirror [SIDE][SIDE][SIDE][W_DIM];

  /*
   * Executes a one tick operation in every cell.
   */
  void update_lattice()
  {
    // Sweep each layer
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            Cell &curr = lattice_curr[x][y][z][w];
            Cell &draft   = lattice_draft[x][y][z][w];
            Cell &mirror  = lattice_mirror[x][y][z][w];
            /****** CONVOLUTION ******/
            if (curr.k < CONVOL)
            {
              if (curr.k == 0)
              {
                mirror = curr;  // Copy current to mirror on first tick
              }
              else if (curr.wv && mirror.wv)
              {
                // Handle wavefront clash if needed
              }
            }
            /****** COLLISION ******/
            else if (curr.k < COLLISION)
            {
              // This condition may be true in more than one wavefront cell at the same time
              if (curr.pole && curr.wv && curr.t > 2*LIGHT && rand() % 3 == 0)
              {
                draft.t = 0;
                draft.c[rand() % 3] = (rand() % 2) ? 1 : SIDE - 1;
              }
            }
            /****** W DIFFUSION ******/
            else if (curr.k < W_DIFFUSION)
            {
              // Handle W Diffusion if needed
              draft.wv = false;
            }
            /****** X DIFFUSION ******/
            else if (curr.k < X_DIFFUSION)
            {
              // This will happen SIDE-1 times in the X DIFFUSION window
              draft.c[0] = max(curr.c[0], lattice_curr[(x == 0) ? SIDE-1 : x-1][y][z][w].c[0]);
            }
            /****** Y DIFFUSION ******/
            else if (curr.k < Y_DIFFUSION)
            {
              // This will happen SIDE-1 times in the Y DIFFUSION window
              draft.c[1] = max(curr.c[1], lattice_curr[x][(y == 0) ? SIDE-1 : y-1][z][w].c[1]);
            }
            /****** Z DIFFUSION ******/
            else if (curr.k < Z_DIFFUSION)
            {
              // This will happen SIDE-1 times in the Z DIFFUSION window
              draft.c[2] = max(curr.c[2], lattice_curr[x][y][(z == 0) ? SIDE-1 : z-1][w].c[2]);
            }
            /****** X RELOCATION ******/
            else if (curr.k < X_RELOC)
            {
              // This test is evaluated SIDE-1 times in the X RELOCATION window
              if (curr.c[0] > 0)
              {
                // Calculate the source position in the x-dimension with wrapping
                unsigned source_x = (x == 0) ? SIDE - 1 : x - 1;
                // Copy data from the source position to the current cell in the draft lattice
                lattice_draft[x][y][z][w] = lattice_curr[source_x][y][z][w];
                draft.c[0] = curr.c[0] - 1;
              }
            }
            /****** Y RELOCATION ******/
            else if (curr.k < Y_RELOC)
            {
              // This test is evaluated SIDE-1 times in the Y RELOCATION window
              if (curr.c[1] > 0)
              {
                // Calculate the source position in the x-dimension with wrapping
                unsigned source_y = (y == 0) ? SIDE - 1 : y - 1;
                // Copy data from the source position to the current cell in the draft lattice
                lattice_draft[x][y][z][w] = lattice_curr[x][source_y][z][w];
                draft.c[1] = curr.c[1] - 1;
              }
            }
            /****** Z RELOCATION ******/
            else if (curr.k < Z_RELOC)
            {
              // This test is evaluated SIDE-1 times in the Z RELOCATION window
              if (curr.c[2] > 0)
              {
                // Calculate the source position in the x-dimension with wrapping
                unsigned source_z = (z == 0) ? SIDE - 1 : z - 1;
                // Copy data from the source position to the current cell in the draft lattice
                lattice_draft[x][y][z][w] = lattice_curr[x][y][source_z][w];
                draft.c[2] = curr.c[2] - 1;
              }
              //draft.wv = false;
              //draft.A.a = 0;
            }
            /****** EXPANSION ******/
            /*    (Light pass)     */
            else
            {
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
                draft.aff = draft.d ^ draft.sin;
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
                  draft.t = 0;
                }
              }
              // Prepare everything for a new FRAME
              draft.net_c0 = draft.net_c1 = draft.net_c2 = 0;
              draft.net_q = draft.net_w0 = draft.net_w1 = 0;
              draft.boson = false;
              draft.fxf = false;
              draft.collapse = false;
              fill(begin(draft.c), end(draft.c), 0);
              fill(begin(draft.m), end(draft.m), 0);
            }
            // Update the superluminal timer
            draft.k = (curr.k + 1) % FRAME;
          }
        }
      }
    }
  }

  /*
   * Swaps lattices after an update.
   */
  void swap_lattices()
  {
    // Counter k is always identical in all cells,
    // take the first cell to represent the others.
    Cell &repr = lattice_curr[0][0][0][0];
    if (repr.k > 0 && repr.k < CONVOL)
      shiftW();
    //
    std::copy(
        &lattice_draft[0][0][0][0],
        &lattice_draft[0][0][0][0] + BLOCK,
        &lattice_curr[0][0][0][0]);
  }

  /*
   * One step of the CA.
   */
  void simulation()
  {
    // Run one step of the simulation
    update_lattice();
    swap_lattices();
    // Update entropy object
    entropyCalc.updateEntropy();
  }

}
