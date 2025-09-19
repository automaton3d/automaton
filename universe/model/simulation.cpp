/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */
#include "simulation.h"

namespace automaton
{
  using namespace std;

 // extern EntropyCalculator entropyCalc;

  // Grid constants
  const unsigned L3 = L2 * EL;
  const unsigned long BLOCK = L3 * W_DIM;

  // Dynamics constants and variables
  const unsigned DIAG      = (unsigned) EL* sqrt(3);
  const unsigned RMAX      = DIAG / 2;
  const unsigned CONVOL    = W_DIM;
  const unsigned DIFFUSION = CONVOL + 3 * (EL - 1) + (W_DIM - 1);
  const unsigned RELOC     = DIFFUSION + 3*(EL - 1);
  const unsigned FRAME     = RELOC;

  // The CA lattices
  Cell lattice_curr   [EL][EL][EL][W_DIM];
  Cell lattice_draft  [EL][EL][EL][W_DIM];
  Cell lattice_mirror [EL][EL][EL][W_DIM];

  bool reloc_x[W_DIM], reloc_y[W_DIM], reloc_z[W_DIM];

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
      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
        	// Generate references to the working cells
            Cell &curr   = lattice_curr[x][y][z][w];
            Cell &draft  = lattice_draft[x][y][z][w];
            Cell &mirror = lattice_mirror[x][y][z][w];
            /****** CONVOLUTION ******/
            if (curr.k < CONVOL)
            {
              convolute(curr, draft, mirror);
            }
            /****** DIFFUSION ******/
            else if (curr.k < DIFFUSION)
            {
              diffuse(curr, draft, mirror);
            }
            /****** RELOCATION ******/
            else if (curr.k < RELOC)
            {
              relocate(curr, draft, mirror);
            }
            // Update tick counter
            draft.k = (curr.k + 1) % FRAME;
            if (draft.k == 0)
            {
              draft.t = (curr.t + 1) % RMAX;
            }
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
    // Take the first cell to represent the others.
    Cell &repr = lattice_curr[0][0][0][0];
	// Execute pending shifts in draft
	if (repr.k == 0)
	{
      // non spatial shifts
      shiftW();
	}
	// Complete relocation
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
    if (repr.k == 0)
    {
      // First tick: update mirror
      std::copy(
          &lattice_curr[0][0][0][0],
          &lattice_curr[0][0][0][0] + BLOCK,
          &lattice_mirror[0][0][0][0]);
      // Update the f variable
      for (unsigned w = 0; w < W_DIM; ++w)
        for (unsigned x = 0; x < EL; ++x)
          for (unsigned y = 0; y < EL; ++y)
            for (unsigned z = 0; z < EL; ++z)
            {
              Cell &mirror = lattice_mirror[x][y][z][w];
              mirror.f = mirror.t;
            }
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
  }

  /*
   * Finds neighbor in one of eight von Neumann directions.
   */
  Cell &Cell::getNeighbor(int i)
  {
    // Displacements for 8 von Neumann directions (4D)
    static const int disp[8][4] =
    {
      {+1,  0,  0,  0}, // +x
      {-1,  0,  0,  0}, // -x
      { 0, +1,  0,  0}, // +y
      { 0, -1,  0,  0}, // -y
      { 0,  0, +1,  0}, // +z
      { 0,  0, -1,  0}, // -z
      { 0,  0,  0, +1}, // +w
      { 0,  0,  0, -1}  // -w
    };

    int nx = (x[0] + disp[i][0] + EL) % EL;
    int ny = (x[1] + disp[i][1] + EL) % EL;
    int nz = (x[2] + disp[i][2] + EL) % EL;
    int nw = (x[3] + disp[i][3] + W_DIM)  % W_DIM;
    return lattice_curr[nx][ny][nz][nw];
  }

}

