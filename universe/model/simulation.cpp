/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */
#include "simulation.h"

namespace automaton
{
  using namespace std;

  // Grid constants
  unsigned EL;
  unsigned L2;
  unsigned L3;
  unsigned W_DIM;

  unsigned long BLOCK;
  // Dynamics constants and variables
  unsigned DIAG;
  unsigned RMAX;
  unsigned CONTRACT;
  unsigned CONVOL;
  unsigned DIFFUSION;
  unsigned RELOC;
  unsigned REISSUE;
  unsigned FRAME;

  unsigned SLOT1;
  unsigned SLOT2;
  unsigned SLOT3;
  unsigned SLOT4;
  unsigned SLOT5;
  unsigned SLOT6;
  unsigned SLOT7;

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

            /****** UPDATE COUNTERS ******/
            // Update tick counter
            draft.k = (curr.k + 1) % FRAME;
            // Update light counter
            if (draft.k == 0)
            {
              if (curr.a == W_DIM)
                draft.t = min(curr.t + 1, RMAX+1);
              else if (!curr.cB)  // Added: skip increment if cB is being consumed (allows t=0 to stick)
                draft.t = (curr.t + 1) % RMAX;
            }

            /*
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
            */
          }
        }
      }
    }
    /*
    if (lattice_curr[0][0][0][0].k == 0)
    {
      assert(sanityTest2());
      assert(sanityTest3());
    }
    */
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
      for (unsigned w = 0; w < W_DIM; ++w)
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
    int nw = (x[3] + disp[i][3] + W_DIM)  % W_DIM;
    return getCell(lattice_curr, nx, ny, nz, nw);
  }

  /**
   * Calculates the simulation parameters.
   */
  void calculateParameters(unsigned L, unsigned W)
  {
    // Grid constants
    EL        = L;
    W_DIM     = W; //     26//(3*L2+1)                 // An even number
    L2        = (EL*EL);
    L3        = L2 * EL;
    ORDER     = ((int)round(log2(EL)));   // Number of bits
    CENTER    = ((EL-1)/2);
    FCENTER   = (EL/2.0);
    BLOCK     = L3 * W_DIM;
    DIAG      = (unsigned) EL* sqrt(3);
    RMAX      = DIAG / 2;
    CONTRACT  = static_cast<int>(floor(sqrt(3.0) * CENTER));
    // Time windows
    CONVOL    = W_DIM;
    SLOT1     = CONVOL + 4 * (EL - 1);
    SLOT2     = SLOT1 + 3*(EL - 1);
    SLOT3     = SLOT2 + 2*W_DIM;
    SLOT4     = SLOT3 + 1;
    DIFFUSION = SLOT4 + (EL - 1);
    SLOT5     = DIFFUSION + (EL - 1);
    SLOT6     = SLOT5 + (EL - 1);
    SLOT7     = SLOT6 + (EL - 1);
    RELOC     = DIFFUSION + 3*(EL - 1);
    REISSUE   = RELOC + 1;
    FRAME     = REISSUE;
  }

}
