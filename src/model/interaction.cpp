/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
 * Core simulation logic - 100% integer operations for deterministic behavior.
 */

#include <cassert>
#include <algorithm>
#include "model/simulation.h"
#include "globals.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;

  int count = 0;

  bool ctrl = true; // debug: fire-once flag for convolution triggers

  /**
   * Converts absolute coordinate to centered coordinate system.
   * Used for geometric calculations where origin is at sphere center.
   * 
   * @param x Absolute coordinate [0, EL-1]
   * @return Centered coordinate [-EL/2, EL/2]
   */
  inline int centered_coord(int x)
  {
    return x - (int)(EL / 2);
  }

  /**
   * Periodic toroidal wrapping - used ONLY for relocation phase.
   * For spherical topology, spatial coordinates use antipodal wrapping instead.
   * This function is kept for legacy relocation logic.
   * 
   * @param x,y,z,w Coordinates to wrap (modified in place)
   */
  inline void periodic_wrap(int& x, int& y, int& z, int& w)
  {
    if (x < 0) x += EL; else if (x >= (int)EL) x -= EL;
    if (y < 0) y += EL; else if (y >= (int)EL) y -= EL;
    if (z < 0) z += EL; else if (z >= (int)EL) z -= EL;
    if (w < 0) w += W_USED; else if (w >= (int)W_USED) w -= W_USED;
  }

  /**
   * Analyzes the cases when the wavefronts cross
   * during the convolution process.
   * Routes to scenario-specific convolution rules.
   *
   * @param curr Current cell state
   * @param draft Destination cell state (to be written)
   * @param mirror Mirrored cell state (from previous frame)
   * @return true if any sanity test failed
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
    switch(gConfig.simulation.scenario)
    {
      case 0:
        return convolute0(curr, draft, mirror);  // Free wave propagation test
      case 1:
        return convolute1(curr, draft, mirror);  // Relocation test
      case 2:
        return convolute2(curr, draft, mirror);  // Orphan mechanism test
      case 3:
        return convolute3(curr, draft, mirror);  // Contraction test
      case 4:
        return convolute4(curr, draft, mirror);  // Hunting test
      case 5:
        return convolute5(curr, draft, mirror);  // Reissue test
      case 6:
        return convolute6(curr, draft, mirror);  // Dispersion test
      case 7:
        return convolute7(curr, draft, mirror);  // Full simulation
      default:
      {
        exit(1);
      }
    }
    return false;
  }

  /**
   * Diffusion phase (SLOTS I-V).
   * Propagates orphan state, hunting vectors, contraction flags,
   * and affinity through the lattice.
   * 
   * @param curr Current cell
   * @param draft Destination cell
   * @param forward, north, west, down, south, east, up Neighbor cells
   */
  void diffuse(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up)
  {
      /****** SLOT I - Initial orphan propagation ******/
      if (curr.k < SLOT1)
      {
        /*--- Orphan propagation: cells become orphan if any neighbor with higher d is orphan ---*/
        if ((north.a == W_USED && curr.d >= north.d) ||
            (west.a  == W_USED && curr.d >= west.d)  ||
            (down.a  == W_USED && curr.d >= down.d)  ||
            (south.a == W_USED && curr.d >= south.d) ||
            (east.a  == W_USED && curr.d >= east.d)  ||
            (up.a    == W_USED && curr.d >= up.d))
        {
          draft.a = W_USED;
        }
      }

      /****** SLOT II - Hunting and orphan propagation ******/
      if (curr.k < SLOT2)
      {
        /*--- Orphan propagation (same as SLOT I) ---*/
        if ((north.a == W_USED && curr.d >= north.d) ||
            (west.a  == W_USED && curr.d >= west.d)  ||
            (down.a  == W_USED && curr.d >= down.d)  ||
            (south.a == W_USED && curr.d >= south.d) ||
            (east.a  == W_USED && curr.d >= east.d)  ||
            (up.a    == W_USED && curr.d >= up.d))
        {
          draft.a = W_USED;
        }

        /*--- Hunting using hB flag: propagates hunter vector outward ---*/
        if (curr.d == effective_t(curr.t))
        {
          if (north.hB)
          {
            draft.c[0] = north.c[0] + 1;
            curr.sB = !draft.hB;
          }
          else if (west.hB)
          {
            draft.c[1] = west.c[1] + 1;
            curr.sB = !draft.hB;
          }
          else if (down.hB)
          {
            draft.c[2] = down.c[2] + 1;
            curr.sB = !draft.hB;
          }
          else if (south.hB)
          {
            draft.c[1] = south.c[1] + 1;
            curr.sB = !draft.hB;
          }
          else if (east.hB)
          {
            draft.c[0] = east.c[0] + 1;
            curr.sB = !draft.hB;
          }
          else if (up.hB)
          {
            draft.c[2] = up.c[2] + 1;
            curr.sB = !draft.hB;
          }
        }
      }

      /****** SLOT III - Affinity and contraction propagation ******/
      else if (curr.k < SLOT3)
      {
        /*--- Propagate c[] vector to ALL cells in layer 0 ---*/
        if (curr.x[3] == 0)
        {
          if (!ZERO(north.c))
          {
            draft.c[0] = north.c[0];
            draft.c[1] = north.c[1];
            draft.c[2] = north.c[2];

            if (north.kB)
              draft.kB = north.kB;
          }
          else if (!ZERO(south.c))
          {
            draft.c[0] = south.c[0];
            draft.c[1] = south.c[1];
            draft.c[2] = south.c[2];

            if (south.kB)
              draft.kB = south.kB;
          }
          else if (!ZERO(east.c))
          {
            draft.c[0] = east.c[0];
            draft.c[1] = east.c[1];
            draft.c[2] = east.c[2];

            if (east.kB)
              draft.kB = east.kB;
          }
          else if (!ZERO(west.c))
          {
            draft.c[0] = west.c[0];
            draft.c[1] = west.c[1];
            draft.c[2] = west.c[2];

            if (west.kB)
              draft.kB = west.kB;
          }
          else if (!ZERO(up.c))
          {
            draft.c[0] = up.c[0];
            draft.c[1] = up.c[1];
            draft.c[2] = up.c[2];

            if (up.kB)
              draft.kB = up.kB;
          }
          else if (!ZERO(down.c))
          {
            draft.c[0] = down.c[0];
            draft.c[1] = down.c[1];
            draft.c[2] = down.c[2];

            if (down.kB)
              draft.kB = down.kB;
          }
        }

        /*--- Propagate maximum f (convolution field) ---*/
        draft.f = std::max(
            down.f,
            std::max(
                west.f,
                std::max(
                    north.f,
                    std::max(
                        south.f,
                        std::max(east.f, up.f)))));

        /*--- Propagate contraction flag cB inward (toward lower d) ---*/
        if (!curr.cB)
        {
          if (north.cB && north.d > curr.d)
          {
            draft.cB = true;
            if (north.a != W_USED)
              draft.a = north.a;
          }
          else if (south.cB && south.d > curr.d)
          {
            draft.cB = true;
            if (south.a != W_USED)
              draft.a = south.a;
          }
          else if (east.cB && east.d > curr.d)
          {
            draft.cB = true;
            if (east.a != W_USED)
              draft.a = east.a;
          }
          else if (west.cB && west.d > curr.d)
          {
            draft.cB = true;
            if (west.a != W_USED)
              draft.a = west.a;
          }
          else if (down.cB && down.d > curr.d)
          {
            draft.cB = true;
            if (down.a != W_USED)
              draft.a = down.a;
          }
          else if (up.cB && up.d > curr.d)
          {
            draft.cB = true;
            if (up.a != W_USED)
              draft.a = up.a;
          }
        }
      }

      /****** SLOT IV - Inertial transport ******/
      else if (curr.k < SLOT4)
      {
        /*--- Transport c[] vector based on geometric delta ---*/
        if (forward.kB && forward.a == curr.a)
        {
          // Centered coordinates
          int cx = centered_coord((int)curr.x[0]);
          int cy = centered_coord((int)curr.x[1]);
          int cz = centered_coord((int)curr.x[2]);

          int fx = centered_coord((int)forward.x[0]);
          int fy = centered_coord((int)forward.x[1]);
          int fz = centered_coord((int)forward.x[2]);

          // Real geometric delta (non-toroidal)
          int delta_x = cx - fx;
          int delta_y = cy - fy;
          int delta_z = cz - fz;

          // Transport without toroidal assumption
          int ncx = (int)forward.c[0] + delta_x;
          int ncy = (int)forward.c[1] + delta_y;
          int ncz = (int)forward.c[2] + delta_z;

          // Clamp to valid range
          ncx = std::max(0, std::min((int)EL - 1, ncx));
          ncy = std::max(0, std::min((int)EL - 1, ncy));
          ncz = std::max(0, std::min((int)EL - 1, ncz));

          draft.c[0] = ncx;
          draft.c[1] = ncy;
          draft.c[2] = ncz;

          draft.kB = forward.kB;
          draft.cB = forward.cB;
        }

        /*--- Propagate maximum convolution field ---*/
        draft.f = std::max(forward.f, curr.f);
      }

      /****** SLOT V - Orphan reinitialization ******/
      else if (curr.k < SLOT5)
      {
        if (curr.a == W_USED)
        {
          if (curr.d < curr.t)
          {
            draft.a = curr.x[3];  // Reset affinity to layer index
          }
        }
      }
  }

  /**
   * Grid relocation phase (SLOTS VI-VIII).
   * Physically moves bubbles by decrementing c[] vector components.
   * 
   * @param curr Current cell
   * @param draft Destination cell
   * @param north, west, down Neighbor cells (direction-specific transport)
   */
  void relocate(Cell& curr, Cell &draft,
                Cell &north, Cell &west, Cell &down)
  {
    // Save the 3D address (will be restored at the end)
    unsigned x = curr.x[0];
    unsigned y = curr.x[1];
    unsigned z = curr.x[2];

    /****** SLOT VI - Transport along X axis (east-west) ******/
    if (curr.k < SLOT6)
    {
      if (north.c[0] > 0)
      {
        draft = north;
        draft.c[0]--;
      }
    }

    /****** SLOT VII - Transport along Y axis (north-south) ******/
    else if (curr.k < SLOT7)
    {
      if (west.c[1] > 0)
      {
        draft = west;
        draft.c[1]--;
      }
    }

    /****** SLOT VIII - Transport along Z axis (up-down) ******/
    else if (curr.k < SLOT8)
    {
      if (down.c[2] > 0)
      {
        draft = down;
        draft.c[2]--;
      }
    }

    // Restore original 3D address (position doesn't change, only content)
    draft.x[0] = x;
    draft.x[1] = y;
    draft.x[2] = z;
  }

  /**
   * Reissue phase (wavefront regeneration).
   * Prepares new wavefront after relocation or contraction.
   * Resets propagation flags and propagates affinity outward.
   * 
   * @param curr Current cell
   * @param draft Destination cell
   * @param forward, north, west, down, south, east, up Neighbor cells
   */
  void reissue(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up)
  {
      // Reset propagation status flags
      draft.kB = false;  // Kill bit (annihilation)
      draft.hB = false;  // Hunting flag
      draft.bB = false;  // Blob flag

      // Propagate normal affinity outward along wavefront
      if (curr.d == effective_t(curr.t))
      {
          if (north.d == curr.d + 1)
            draft.a = north.a;

          if (south.d == curr.d + 1)
            draft.a = south.a;

          if (east.d == curr.d + 1)
            draft.a = east.a;

          if (west.d == curr.d + 1)
            draft.a = west.a;

          if (up.d == curr.d + 1)
            draft.a = up.a;

          if (down.d == curr.d + 1)
            draft.a = down.a;
      }

      // Handle contraction flag (cB)
      if (curr.cB)
      {
        // Consume contraction flag (one-shot propagation)
        draft.cB = false;

        // When contraction reaches center (d < 2), reset time counter
        if (curr.a != W_USED && curr.d < 2)
        {
          draft.t = 0;  // Restart wavefront expansion
        }
      }
  }

  /**
   * Flood phase (time equalization).
   * Propagates minimum t value across connected regions.
   * Ensures wavefront synchronization.
   * 
   * @param curr Current cell
   * @param draft Destination cell
   * @param forward, north, west, down, south, east, up Neighbor cells
   */
  void flood(Cell& curr, Cell &draft, Cell &forward,
             Cell &north, Cell &west, Cell &down,
             Cell &south, Cell &east, Cell &up)
  {
    if (curr.a != W_USED)
    {
      // Take minimum t from all neighbors (wavefront synchronization)
      draft.t = std::min({
          north.t,
          south.t,
          east.t,
          west.t,
          down.t,
          up.t
      });
    }
  }

} // namespace automaton