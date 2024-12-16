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
  const unsigned long BLOCK = SIDE3 * W_DIM;

  // Dynamics constants and variables
  const unsigned DIAG        = (unsigned) SIDE * sqrt(3);
  const unsigned RMAX        = DIAG / 2;
  const unsigned FMAX        = RMAX / 2;
  const unsigned CONVOL      = W_DIM;
  const unsigned COLLISION   = CONVOL + 1;
  const unsigned DIFFUSION   = COLLISION + W_DIM - 1;
  const unsigned RELOCATION  = DIFFUSION + (SIDE - 1);
  const unsigned TRANSPORT   = RELOCATION + 3*(SIDE - 1);
  const unsigned UPDATE      = TRANSPORT;
  const unsigned LIGHT       = (unsigned) (sqrt(3) * (SIDE - 1) / 2) + 1;
  const unsigned FRAME       = UPDATE + LIGHT;

  unsigned RANGE;

  bool reloc = false;

  // The CA lattices
  Cell lattice_curr   [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_draft  [SIDE][SIDE][SIDE][W_DIM];
  Cell lattice_mirror [SIDE][SIDE][SIDE][W_DIM];

  bool reloc_x[W_DIM], reloc_y[W_DIM], reloc_z[W_DIM];
  bool reloc_w = false;

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
        	// Generate references to the working cells
            Cell &curr   = lattice_curr[x][y][z][w];
            Cell &draft  = lattice_draft[x][y][z][w];
            Cell &mirror = lattice_mirror[x][y][z][w];
            /****** CONVOLUTION ******/
            if (curr.k < CONVOL)
            {
              // First tick used for initializations before
              // the first w shift
              if (curr.k == 0)
              {
                // Prepare to convolve charge
                draft.net_c0 = (curr.charge & C0_MASK) ? CENTER-1 : CENTER+1;
                draft.net_c1 = (curr.charge & C1_MASK) ? CENTER-1 : CENTER+1;
                draft.net_c2 = (curr.charge & C2_MASK) ? CENTER-1 : CENTER+1;
                draft.net_w0 = (curr.charge & W0_MASK) ? CENTER-1 : CENTER+1;
                draft.net_w1 = (curr.charge & W1_MASK) ? CENTER-1 : CENTER+1;
                draft.net_q  = (curr.charge & Q_MASK)  ? CENTER-1 : CENTER+1;
                draft.boson = true;
              }
              // Interaction only involves active cells.
              else if (curr.wv && mirror.wv)
              {
                // unsigned w_left = (w == 0) ? W_DIM-1 : w-1;
                // unsigned w_right = (w == W_DIM-1) ? 0 : w+1;
                // if (convolute(curr, draft, mirror, lattice_mirror[x][y][z][w_left],
                // lattice_mirror[x][y][z][w_right]))
                seggregation(curr, draft, mirror);
              }
              // Warn the swap routine to do a w shift
              reloc_w = true;
            }
            /****** COLLISION ******/
            else if (curr.k < COLLISION)
            {
              // Is it a slave?
              // (the convolution step modifies the draft for
              // masters and mirror for slaves, so we must get
              // what was stored stored in mirror)
              if (!ZERO(mirror.c))
              {
                // Copy back relocation
                copy(begin(mirror.c), end(mirror.c), std::begin(draft.c));
                // Copy back parallel transport
                copy(begin(mirror.m), end(mirror.m), std::begin(draft.m));
              }
              else
              {
            	  // Template
              }
            }
            /****** DIFFUSION ******/
            else if (curr.k < DIFFUSION)
            {
              draft.wv = false;
            }
            /****** RELOCATION ******/
            else if (curr.k < RELOCATION)
            {
              if (curr.c[0] > 0)
              {
            	reloc_x[w] = true;
                draft.c[0] = curr.c[0] - 1;
              }
              if (curr.c[1] > 0)
              {
              	reloc_y[w] = true;
                draft.c[1] = curr.c[1] - 1;
              }
              if (curr.c[2] > 0)
              {
              	reloc_z[w] = true;
                draft.c[2] = curr.c[2] - 1;
              }
            }
            /****** PARALLEL TRANSPORT ******/
            else if (curr.k < TRANSPORT)
            {
            }
            /***********************/
            /*     EXPANSION       */
            /*    (Light pass)     */
            /***********************/
            else
            {
              // Propagate time
              unsigned t = curr.t;
              unsigned t0;
              t0 = lattice_curr[(x + 1) % SIDE][y][z][w].t;
              t = min(t0, t);
              t0 = lattice_curr[(x + SIDE - 1) % SIDE][y][z][w].t;
              t = min(t0, t);
              t0 = lattice_curr[x][(y + 1) % SIDE][z][w].t;
              t = min(t0, t);
              t0 = lattice_curr[x][(y + SIDE - 1) % SIDE][z][w].t;
              t = min(t0, t);
              t0 = lattice_curr[x][y][(z + 1) % SIDE][w].t;
              t = min(t0, t);
              t0 = lattice_curr[x][y][(z + SIDE - 1) % SIDE][w].t;
              t = min(t0, t);
              // Awake the wavefront
              if (t == curr.d)
                draft.wv = true;
              // Increment the clock
              draft.t = t + 1;
              // Light pass completed?
              if (draft.t % LIGHT == 0)
              {
                draft.ph.a >>= 1;
              }
              // Test time wrapping
              if (draft.t == RANGE)
              {
                // TODO: angle etc.
                draft.t = 0;
              }
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

  /**
   * Swaps lattices after an update.
   * (Counter k is always identical in all cells)
   */
  void swap_lattices()
  {
	// Execute pending shifts in draft
	if (reloc_w)
	{
      // non spatial shifts
      shiftW();
	}
	else
	{
      // Spatial shifts
	  for (unsigned w = 0; w < W_DIM; w++)
      {
        if (reloc_x[w])
          shiftX(w);
        if (reloc_y[w])
          shiftY(w);
        if (reloc_z[w])
          shiftZ(w);
      }
	}
    // Update the current lattice
    std::copy(
        &lattice_draft[0][0][0][0],
        &lattice_draft[0][0][0][0] + BLOCK,
        &lattice_curr[0][0][0][0]);
    // Take the first cell to represent the others.
    Cell &repr = lattice_curr[0][0][0][0];
    if (repr.k == 0)
    {
      // First tick: update mirror
      std::copy(
          &lattice_curr[0][0][0][0],
          &lattice_curr[0][0][0][0] + BLOCK,
          &lattice_mirror[0][0][0][0]);
    }
  }

  /*
   * One step of the CA.
   */
  void simulation()
  {
    // Run one step of the simulation
    update_lattice();
    swap_lattices();
    assert(sanityTest5());
    // Update entropy object
    entropyCalc.updateEntropy();
  }

}
