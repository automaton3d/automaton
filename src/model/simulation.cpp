/*
 * simulation.cpp
 * Implements the main functionality of the FSM (Finite State Machine).
 * Core is 100% integer - no floats in simulation logic.
 * Floating-point operations are only used in auxiliary geometry functions.
 */

#define _USE_MATH_DEFINES
#include <cmath>
#include <thread>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <array>

#include "model/simulation.h"
#include "model/geometry.h"
#include "config.h"

#ifdef USE_CUDA
extern void cudaSimulationStepWrapper();
extern bool isCudaEnabled();
extern void updateMirrorOnGPU();
#endif

namespace automaton
{
  using namespace std;

  // Grid constants
  unsigned EL;               // Edge length of cube [0, EL-1]
  unsigned W_DIM;            // W dimension size
  unsigned W_USED;           // Number of active W layers
  unsigned L2;               // EL * EL
  unsigned L3 = 0;           // EL * EL * EL
  unsigned long BLOCK = 0;   // Total cells = L3 * W_USED
  unsigned DIAG = 0;         // Diagonal length
  unsigned RMAX = 0;         // Maximum radius (sphere radius)
  unsigned CONTRACT = 0;     // Contraction threshold
  unsigned UPDATE = 0;       // Update counter
  unsigned CONVOL = 0;       // Convolution phase duration

  // Slot timing constants (phase boundaries)
  unsigned GSLOT_X = 0, GSLOT_Y = 0, GSLOT_Z = 0;

  unsigned SLOT1 = 0, SLOT2 = 0, SLOT3 = 0, SLOT4 = 0, SLOT5 = 0;
  unsigned SLOT6 = 0, SLOT7 = 0, SLOT8 = 0;

  unsigned DIFFUSION = 0;    // Start of diffusion phase
  unsigned RELOC = 0;        // Start of relocation phase
  unsigned REISSUE = 0;      // Start of reissue phase
  unsigned FLOOD = 0;        // Start of flood phase
  unsigned FRAME = 0;        // Total frame length (one full cycle)

  unsigned ORDER;            // Log2(EL)
  unsigned CENTER;           // Center coordinate (EL-1)/2
  unsigned FCENTER;          // Floating-point center (EL/2.0)

  // The CA lattices
  std::vector<Cell> lattice_curr;    // Current state
  std::vector<Cell> lattice_draft;   // Next state (being computed)
  std::vector<Cell> lattice_mirror;  // Mirror state for convolution

  string lastAllocationError;        // Last memory allocation error message

  // Layer tracking - stores center position for each W layer
  std::vector<std::array<unsigned, 3>> lcenters;

  /**
   * Tracks the center of a bubble for a given layer.
   * Called when a cell with d==0 is found (wavefront center).
   * This allows bubble centers to move during simulation.
   * 
   * @param x,y,z Coordinates of the center
   * @param w Layer index
   */
  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w)
  {
      // Update bubble center for this layer to where d==0 is
      // This enables bubble movement during simulation
      lcenters[w][0] = x;
      lcenters[w][1] = y;
      lcenters[w][2] = z;
  }

  /**
   * Executes one simulation tick on CPU.
   * 100% integer operations - no floating point.
   * Processes all cells in all layers using von Neumann neighborhood
   * with antipodal spherical wrapping.
   */
  void update_lattice_cpu()
  {
    for (unsigned w = 0; w < W_USED; ++w)
    {
      // Debug delays (for visualization)
      if (w == 0)
      {
        const Cell& first = lattice_curr.front();

        if (convol_delay && first.k < CONVOL)
        {
          std::this_thread::sleep_for(
              std::chrono::milliseconds(120));
        }
        else if (diffuse_delay &&
                 first.k >= CONVOL &&
                 first.k < DIFFUSION)
        {
          std::this_thread::sleep_for(
              std::chrono::milliseconds(80));
        }
        else if (reloc_delay &&
                 first.k >= DIFFUSION &&
                 first.k < RELOC)
        {
          std::this_thread::sleep_for(
              std::chrono::milliseconds(120));
        }
      }

      // Sweep entire 3D domain - ALL cells are processed
      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
            // Work with all cells - no filtering!
            Cell &curr   = getCell(lattice_curr,   x, y, z, w);
            Cell &draft  = getCell(lattice_draft,  x, y, z, w);
            Cell &mirror = getCell(lattice_mirror, x, y, z, w);

            draft = curr;

            // Neighbors with antipodal wrapping (auxiliary function)
            Cell &forward = curr.getNeighbor(FORWARD);
            Cell &north   = curr.getNeighbor(NORTH);
            Cell &west    = curr.getNeighbor(WEST);
            Cell &down    = curr.getNeighbor(DOWN);
            Cell &south   = curr.getNeighbor(SOUTH);
            Cell &east    = curr.getNeighbor(EAST);
            Cell &up      = curr.getNeighbor(UP);

            /****** CONVOLUTION PHASE - Wavefront interaction ******/
            if (curr.k < CONVOL)
            {
              convolute(curr, draft, mirror);
            }

            /****** DIFFUSION PHASE - Propagate state through lattice ******/
            else if (curr.k < DIFFUSION)
            {
              diffuse(curr, draft,
                      forward,
                      north,
                      west,
                      down,
                      south,
                      east,
                      up);
            }

            /****** RELOCATION PHASE - Physical bubble movement ******/
            else if (curr.k < RELOC)
            {
              relocate(curr, draft,
                       north,
                       west,
                       down);
            }

            /****** REISSUE PHASE - Wavefront regeneration ******/
            else if (curr.k < REISSUE)
            {
              reissue(curr, draft,
                      forward,
                      north,
                      west,
                      down,
                      south,
                      east,
                      up);
            }

            /****** FLOOD PHASE - Time equalization ******/
            else if (curr.k < FLOOD)
            {
              flood(curr, draft,
                    forward,
                    north,
                    west,
                    down,
                    south,
                    east,
                    up);
            }

            /****** TRACK CENTER - Update bubble center position ******/
            if (curr.d == 0)
            {
              trackCenter(x, y, z, w);
            }

            /****** UPDATE COUNTERS - Increment time counters ******/

            // Tick counter (k) - cycles through FRAME
            draft.k = (curr.k + 1) % FRAME;

            // Light counter (t) - wavefront phase
            if (draft.k == 0)
            {
              if (curr.a == W_USED && curr.t <= RMAX)
              {
                draft.t = curr.t + 1;  // Orphan continues expanding
              }
              else
              {
                draft.t = (curr.t + 1) % (RMAX + 1);  // Normal oscillation
              }
            }
          }
        }
      }
    }
  }

  /**
   * Main lattice update dispatcher.
   * Routes to CPU or CUDA implementation based on configuration.
   */
  void update_lattice()
  {
#ifdef USE_CUDA
    if (isCudaEnabled())
    {
      cudaSimulationStepWrapper();
      return;
    }
#endif

    update_lattice_cpu();
  }

  /**
   * Swaps lattices after update (CPU version).
   * Copies draft to current, updates mirror state.
   * 
   * @return true if a new light frame started (k == 0)
   */
  bool swap_lattices_cpu()
  {
    bool newLightFrame = false;

    // Copy draft to current (commit next state)
    std::copy(
        lattice_draft.begin(),
        lattice_draft.begin() + BLOCK,
        lattice_curr.begin());

    Cell &repr =
        getCell(lattice_curr, 0, 0, 0, 0);

    // New frame starts when k wraps to 0
    if (repr.k == 0)
    {
      // Update mirror lattice with current state
      for (unsigned w = 0; w < W_USED; ++w)
      {
        for (unsigned x = 0; x < EL; ++x)
        {
          for (unsigned y = 0; y < EL; ++y)
          {
            for (unsigned z = 0; z < EL; ++z)
            {
              Cell &curr =
                  getCell(lattice_curr,
                          x, y, z, w);

              Cell &mirror =
                  getCell(lattice_mirror,
                          x, y, z, w);

              mirror = curr;

              // Reset convolution field to current time
              mirror.f = mirror.t;
            }
          }
        }
      }

      newLightFrame = true;
    }

    // Shift mirror during convolution phase
    if (repr.k < CONVOL)
    {
      shiftMirror();
    }

    return newLightFrame;
  }

  /**
   * Lattice swap dispatcher.
   * Routes to CPU or CUDA implementation.
   * 
   * @return true if a new light frame started
   */
  bool swap_lattices()
  {
#ifdef USE_CUDA
    if (isCudaEnabled())
    {
      Cell &repr =
          getCell(lattice_curr,
                  0, 0, 0, 0);

      return (repr.k == 0);
    }
#endif

    return swap_lattices_cpu();
  }

  /**
   * One complete simulation step.
   * Updates lattice and swaps buffers.
   * 
   * @return true if a new light frame started
   */
  bool simulation()
  {
    update_lattice();
    return swap_lattices();
  }


  /**
   * Finds neighbor in one of eight von Neumann directions.
   * Uses corrected spherical antipodal wrapping geometry adapted per layer.
   * 
   * Directions:
   *   0: FORWARD  (x+1)
   *   1: BACKWARD (x-1)
   *   2: NORTH    (y+1)
   *   3: SOUTH    (y-1)
   *   4: UP       (z+1)
   *   5: DOWN     (z-1)
   *   6: W+1      (w+1)
   *   7: W-1      (w-1)
   * 
   * @param i Direction index (0-7)
   * @return Reference to neighbor cell
   */
  Cell &Cell::getNeighbor(int i)
  {
    static int disp[8][4] =
    {
        {+1,  0,  0,  0},  // FORWARD  (x+1)
        {-1,  0,  0,  0},  // BACKWARD (x-1)
        { 0, +1,  0,  0},  // NORTH    (y+1)
        { 0, -1,  0,  0},  // SOUTH    (y-1)
        { 0,  0, +1,  0},  // UP       (z+1)
        { 0,  0, -1,  0},  // DOWN     (z-1)
        { 0,  0,  0, +1},  // W+1
        { 0,  0,  0, -1}   // W-1
    };

    int nx = (int)x[0] + disp[i][0];
    int ny = (int)x[1] + disp[i][1];
    int nz = (int)x[2] + disp[i][2];
    int nw = (int)x[3] + disp[i][3];

    // Apply antipodal wrapping ONLY for spatial coordinates (x, y, z)
    spherical_wrap(nx, ny, nz);  // Only 3 arguments!

    // Additional integer bounds guarantee for spatial coordinates
    if (nx < 0) nx = 0;
    if (nx >= (int)EL) nx = (int)EL - 1;
    if (ny < 0) ny = 0;
    if (ny >= (int)EL) ny = (int)EL - 1;
    if (nz < 0) nz = 0;
    if (nz >= (int)EL) nz = (int)EL - 1;

    // Periodic wrapping for W dimension (separate from spatial)
    nw = nw % (int)W_USED;
    if (nw < 0) nw += (int)W_USED;

    // Safety assertions - should never fail after correct wrapping
    assert(nx >= 0 && nx < (int)EL);
    assert(ny >= 0 && ny < (int)EL);
    assert(nz >= 0 && nz < (int)EL);
    assert(nw >= 0 && nw < (int)W_USED);

    return getCell(lattice_curr, (unsigned)nx, (unsigned)ny, (unsigned)nz, (unsigned)nw);
  }

}