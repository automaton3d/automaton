/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include <thread>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <array>
#include "simulation.h"

namespace automaton
{
  using namespace std;

  // Grid constants
  unsigned EL;
  unsigned W_DIM;
  unsigned W_USED;
  unsigned L2;
  unsigned L3 = 0;
  unsigned long BLOCK = 0;
  unsigned DIAG = 0;
  unsigned RMAX = 0;
  unsigned CONTRACT = 0;
  unsigned UPDATE = 0;
  unsigned CONVOL = 0;
  unsigned SLOT1 = 0, SLOT2 = 0, SLOT3 = 0, SLOT4 = 0, SLOT5 = 0;
  unsigned SLOT6 = 0, SLOT7 = 0, SLOT8 = 0;
  unsigned DIFFUSION = 0;
  unsigned RELOC = 0;
  unsigned REISSUE = 0;
  unsigned FLOOD = 0;
  unsigned FRAME = 0;
  unsigned ORDER;
  unsigned CENTER;
  unsigned FCENTER;

  // The CA lattices
  std::vector<Cell> lattice_curr;
  std::vector<Cell> lattice_draft;
  std::vector<Cell> lattice_mirror;

  // Support for visualization delays

  bool convol_delay  = false;
  bool diffuse_delay = false;
  bool reloc_delay   = false;

  string lastAllocationError;

  // Layer tracking

  std::vector<std::array<unsigned, 3>> lcenters;

  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w)
  {
    lcenters[w][0] = x;
    lcenters[w][1] = y;
    lcenters[w][2] = z;
  }

  /*
   * Executes a one tick operation in every cell.
   */
  void update_lattice()
  {
    // Sweep each layer
    // Simulate a parallel execution in all cells
    for (unsigned w = 0; w < W_USED; ++w)
    {
   	  if (w == 0)
   	  {
    	const Cell& first = lattice_curr.front();
    	if (convol_delay && first.k < CONVOL)
    	{
    	  std::this_thread::sleep_for(std::chrono::milliseconds(120));
   	    }
    	else if (diffuse_delay && first.k >= CONVOL && first.k < DIFFUSION)
    	{
    	  std::this_thread::sleep_for(std::chrono::milliseconds(80));
    	}
    	else if (reloc_delay && first.k >= DIFFUSION && first.k < RELOC)
    	{
    	  std::this_thread::sleep_for(std::chrono::milliseconds(120));
    	}
      }
      // Sweep a 3D space
      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
          // Generate references to the working cells
            Cell &curr   = getCell(lattice_curr, x, y, z, w);
            Cell &draft  = getCell(lattice_draft, x, y, z, w);
            Cell &mirror = getCell(lattice_mirror, x, y, z, w);
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
            }
            /****** DIFFUSION ******/
            else if (curr.k < DIFFUSION)
            {
              diffuse(curr, draft, forward, north, west, down, south, east, up);
            }
            /****** RELOCATION ******/
            else if (curr.k < RELOC)
            {
              relocate(curr, draft, north, west, down);
            }
            /****** REISSUE ******/
            else if (curr.k < REISSUE)
            {
              reissue(curr, draft, forward, north, west, down, south, east, up);
            }
            /****** FLOOD ******/
            else if (curr.k < FLOOD)
            {
              flood(curr, draft, forward, north, west, down, south, east, up);
            }
            /****** UPDATE LAYER TRACKING ******/
            if (curr.d == 0)
            {
              trackCenter(x, y, z, w);
            }
            /****** UPDATE COUNTERS ******/
            // Update tick counter
            draft.k = (curr.k + 1) % FRAME;
            // Update light counter
            if (draft.k == 0)
            {
              if (curr.a == W_USED && curr.t <= RMAX )
              {
            	draft.t++;
              }
              else
              {
                draft.t = (curr.t + 1) % RMAX;
              }
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
    // Update the current lattice
    std::copy(
        lattice_draft.begin(),
        lattice_draft.begin() + BLOCK,
        lattice_curr.begin());
    // Take the first cell to represent the others
    Cell &repr = getCell(lattice_curr, 0, 0, 0, 0);
    if (repr.k == 0)
    {
      for (unsigned w = 0; w < W_USED; ++w)
      {
        for (unsigned x = 0; x < EL; ++x)
        {
          for (unsigned y = 0; y < EL; ++y)
          {
            for (unsigned z = 0; z < EL; ++z)
            {
              Cell &curr = getCell(lattice_curr, x, y, z, w);
              Cell &mirror = getCell(lattice_mirror, x, y, z, w);
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
    static int disp[8][4] =
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
    int nw = (x[3] + disp[i][3] + W_USED)  % W_USED;
    return getCell(lattice_curr, nx, ny, nz, nw);
  }

}
