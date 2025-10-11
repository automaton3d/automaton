/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */
#include "simulation.h"

namespace automaton
{
  using namespace std;

  // Grid constants
  const unsigned L3 = L2 * EL;
  const unsigned long BLOCK = L3 * W_DIM;

  // Dynamics constants and variables
  const unsigned DIAG      = (unsigned) EL* sqrt(3);
  const unsigned RMAX      = DIAG / 2;
  const unsigned CONTRACT  = static_cast<int>(floor(sqrt(3.0) * CENTER));
  const unsigned CONVOL    = W_DIM;
  const unsigned SLOT4 = SLOT3 + 4*(EL - 1);
  const unsigned DIFFUSION = SLOT4 + (EL - 1);
  const unsigned RELOC     = DIFFUSION + 3*(EL - 1);
  const unsigned REISSUE   = RELOC + 1;
  const unsigned FRAME     = REISSUE;

  const unsigned SLOT1 = CONVOL + 4 * (EL - 1);
  const unsigned SLOT2 = SLOT1 + 3*(EL - 1);
  const unsigned SLOT3 = SLOT2 + 2*W_DIM;
  const unsigned SLOT5 = DIFFUSION + (EL - 1);
  const unsigned SLOT6 = SLOT5 + (EL - 1);
  const unsigned SLOT7 = SLOT6 + (EL - 1);

  // The CA lattices
  Cell lattice_curr   [EL][EL][EL][W_DIM];
  Cell lattice_draft  [EL][EL][EL][W_DIM];
  Cell lattice_mirror [EL][EL][EL][W_DIM];

  // Support for visualization delays

  bool convol_delay  = false;
  bool diffuse_delay = false;
  bool reloc_delay   = false;

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
            draft = curr;
            // Calculate neighbors
            Cell &forward = curr.getNeighbor(FORWARD);
            Cell &north   = curr.getNeighbor(NORTH);
            Cell &west    = curr.getNeighbor(WEST);
            Cell &down    = curr.getNeighbor(DOWN);
            Cell &south   = curr.getNeighbor(SOUTH);
            Cell &east    = curr.getNeighbor(EAST);
            Cell &up      = curr.getNeighbor(UP);
            /****** CONVOLUTION ******/
            if (curr.k < CONVOL)
            {
              convolute(curr, draft, mirror);
              if (convol_delay)
                std::this_thread::sleep_for(std::chrono::nanoseconds(500));
            }
            /****** DIFFUSION ******/
            else if (curr.k < DIFFUSION)
            {
              diffuse(curr, draft, forward, north, west, down, south, east, up);
              if (diffuse_delay)
                std::this_thread::sleep_for(std::chrono::nanoseconds(500));
            }
            /****** RELOCATION ******/
            else if (curr.k < RELOC)
            {
          	  relocate(curr, draft, north, west, down);
              if (reloc_delay)
                std::this_thread::sleep_for(std::chrono::nanoseconds(500));
            }
            /****** REISSUE ******/
            else if (curr.k < REISSUE)
            {
              reissue(curr, draft, forward, north, west, down, south, east, up);
            }
            else
            {
              // Catastrophic error
              assert(false && "zebra!");
            }
            /****** UPDATE COUNTERS ******/
            // Update tick counter
            draft.k = (curr.k + 1) % FRAME;
            // Update light counter
            if (draft.k == 0)
            {
              if (curr.a == W_DIM)
                draft.t = min(curr.t + 1, RMAX+1);
              else
                draft.t = (curr.t + 1) % RMAX;
            }
          }
        }
      }
    }
    if (lattice_curr[0][0][0][0].k == 0)
    {
      assert(sanityTest2());
      assert(sanityTest3());
    }
  }

  /**
   * Swaps lattices after an update.
   * (Counter k is always identical in all cells)
   */
  void swap_lattices()
  {
    // Update the current lattice
    std::copy(
          &lattice_draft[0][0][0][0],
          &lattice_draft[0][0][0][0] + BLOCK,
          &lattice_curr[0][0][0][0]);
    // Take the first cell to represent the others
    Cell &repr = lattice_curr[0][0][0][0];
    if (repr.k == 0)
    {
      for (unsigned w = 0; w < W_DIM; ++w)
      {
        for (unsigned x = 0; x < EL; ++x)
        {
          for (unsigned y = 0; y < EL; ++y)
          {
            for (unsigned z = 0; z < EL; ++z)
            {
              Cell &curr = lattice_curr[x][y][z][w];
              Cell &mirror = lattice_mirror[x][y][z][w];
              mirror = curr;
              // Reset f for the next convolution
              mirror.f = mirror.t;
            }
          }
        }
      }
    }
    if (repr.k < CONVOL)
    {
      shiftMirror();
    }
  }

  //extern int count;

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

