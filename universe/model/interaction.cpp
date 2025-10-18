/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
 */

#include "simulation.h"

#define SCENARIO 1

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_DIM;

  int count = 0;

  /**
   * Analyzes the cases when the wavefronts cross
   * during the convolution process.
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
    switch(SCENARIO)
    {
      case 1:
        return convolute1(curr, draft, mirror);
      case 2:
        return convolute2(curr, draft, mirror);
      case 3:
        return convolute3(curr, draft, mirror);
    }
    return false;
  }

  /**
   * Some properties must spread superluminally.
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  void diffuse(Cell& curr, Cell &draft, Cell &forward,
           Cell &north, Cell &west, Cell &down,
           Cell &south, Cell &east, Cell &up)
  {
    /****** SLOT I ******/
    if (curr.k < SLOT1)
    {
      // Propagate orphans outward (inclusive)
      if ((north.a == W_DIM && curr.d >= north.d) ||
        (west.a == W_DIM && curr.d >= west.d)   ||
        (down.a == W_DIM && curr.d >= down.d)   ||
        (south.a == W_DIM && curr.d >= south.d) ||
        (east.a == W_DIM && curr.d >= east.d)   ||
        (up.a == W_DIM && curr.d >= up.d))
      {
        draft.a = W_DIM;
      }
      // Propagate cB inward (inclusive)
      // This marks all cells inside the contraction sphere
      bool neighbor_cB = (north.cB && curr.d <= north.d) ||
                        (west.cB && curr.d <= west.d)   ||
                        (down.cB && curr.d <= down.d)   ||
                        (south.cB && curr.d <= south.d) ||
                        (east.cB && curr.d <= east.d)   ||
                        (up.cB && curr.d <= up.d);

      if (neighbor_cB)
      {
        draft.cB = true;
        // CRITICAL: Don't mark interior cells as orphans during contraction
        // Only the wavefront shell becomes orphan
        if (curr.a == W_DIM)
        {
          // Already orphan, keep it
        	assert(curr.t > 0);
          draft.a = W_DIM;
        }
        else if (curr.t == curr.d)
        {
          // On the wavefront being converted to orphan
          draft.a = W_DIM;
        }
        // else: interior cells stay as normal (a = w)
      }
      // Hunting using hB
      if (curr.d == curr.t)
      {
        if (north.hB)
        {
          draft.c[0] = (north.c[0] + 1) % EL;
          curr.sB = !draft.hB;
        }
        else if (west.hB)
        {
          draft.c[1] = (west.c[1] + 1) % EL;
          curr.sB = !draft.hB;
        }
        else if (down.hB)
        {
          draft.c[2] = (down.c[2] + 1) % EL;
          curr.sB = !draft.hB;
        }
        else if (south.hB)
        {
          draft.c[0] = (south.c[0] + 1) % EL;
          curr.sB = !draft.hB;
        }
        else if (east.hB)
        {
          draft.c[1] = (east.c[1] + 1) % EL;
          curr.sB = !draft.hB;
        }
        else if (up.hB)
        {
          draft.c[2] = (up.c[2] + 1) % EL;
          curr.sB = !draft.hB;
        }
      }
    }
    /****** SLOT II ******/
    else if (curr.k < SLOT2)
    {
      // Diffuse c vector in 3D
      if (!ZERO(north.c))
      {
        draft.c[0] = north.c[0];
        draft.c[1] = north.c[1];
        draft.c[2] = north.c[2];
        // Diffuse collapse
        if (north.kB)
          draft.kB = north.kB;
      }
      if (!ZERO(west.c))
      {
        draft.c[0] = west.c[0];
        draft.c[1] = west.c[1];
        draft.c[2] = west.c[2];
        // Diffuse collapse
        if (west.kB)
          draft.kB = west.kB;
      }
      if (!ZERO(down.c))
      {
        draft.c[0] = down.c[0];
        draft.c[1] = down.c[1];
        draft.c[2] = down.c[2];
        // Diffuse collapse
        if (down.kB)
          draft.kB = down.kB;
      }
      // Propagate frequency
      draft.f = max(down.f, max(west.f, max(north.f, max(south.f, max(east.f, up.f)))));
    }
    /****** SLOT III ******/
    else if (curr.k < SLOT3)
    {
      // Propagate kB in W dimension
      if (forward.kB && forward.a == curr.a)
      {
        // Calculate delta: difference between bubble centers
        // If forward.c points to forward's reissue point,
        // we need to adjust it relative to curr's position
        // The delta is the offset between where forward is centered
        // versus where curr is centered
        int delta_x = (curr.x[0] - forward.x[0] + EL) % EL;
        int delta_y = (curr.x[1] - forward.x[1] + EL) % EL;
        int delta_z = (curr.x[2] - forward.x[2] + EL) % EL;

        // Apply delta to translate forward's c vector to curr's coordinate system
        draft.c[0] = (forward.c[0] + delta_x) % EL;
        draft.c[1] = (forward.c[1] + delta_y) % EL;
        draft.c[2] = (forward.c[2] + delta_z) % EL;
        draft.kB = forward.kB;
        draft.cB = forward.cB;
      }
      draft.f = max(forward.f, curr.f);
    }
    /****** SLOT IV ******/
    else if (curr.k < SLOT4)
    {
      // Consume the contraction bit
      // FIXED: Only reset time for cells inside the contraction radius
      if (curr.cB)
      {
        // For orphan cells, keep their time advancing normally
        if (curr.a == W_DIM)
        {
          // Orphan: just clear the cB flag, don't reset time
          draft.cB = false;
        }
        else
        {
          // Normal cells inside contraction: reset to t=0 in next frame
          // This creates the fresh wavefront at the center
          draft.t = 0;  // Changed from RMAX - 1 to set t=0 immediately
          // Reset propagation status
          draft.cB = false;
          draft.kB = false;
          draft.hB = false;
          draft.bB = false;
        }
      }
    }
  }

  /**
   * Grid relocation.
   */
  void relocate(Cell& curr, Cell &draft, Cell &north, Cell &west, Cell &down)
  {
	// Save the 3D address
	unsigned x, y, z;
	x = curr.x[0];
	y = curr.x[1];
	z = curr.x[2];
    /****** SLOT V ******/
    if (curr.k < SLOT5)
    {
      if (north.c[0] > 0)
      {
        draft = north;
        draft.c[0]--;
      }
    }
    /****** SLOT VI ******/
    else if (curr.k < SLOT6)
    {
      if (west.c[1] > 0)
      {
        draft = west;
        draft.c[1]--;
      }
    }
    /****** SLOT VII ******/
    else if (curr.k < SLOT7)
    {
      if (down.c[2] > 0)
      {
        draft = down;
        draft.c[2]--;
      }
    }
    // Recover 3D address
	draft.x[0] = x;
	draft.x[1] = y;
	draft.x[2] = z;
  }

  /**
   * Prepares new wavefront.
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  void reissue(Cell& curr, Cell &draft, Cell &forward,
           Cell &north, Cell &west, Cell &down,
           Cell &south, Cell &east, Cell &up)
  {
    assert(!curr.cB);
    // Propagate normal affinity outward, overwriting normal or orphan
    if (curr.t == curr.d)
    {
      if (north.d == curr.d + 1)
      {
        draft.a = north.a; // Copy a from inner to outer cell
      }
      if (south.d == curr.d + 1)
      {
        draft.a = south.a; // Copy a from inner to outer cell
      }
      if (east.d == curr.d + 1)
      {
        draft.a = east.a; // Copy a from inner to outer cell
      }
      if (west.d == curr.d + 1)
      {
        draft.a = west.a; // Copy a from inner to outer cell
      }
      if (up.d == curr.d + 1)
      {
        draft.a = up.a; // Copy a from inner to outer cell
      }
      if (down.d == curr.d + 1)
      {
        draft.a = down.a; // Copy a from inner to outer cell
      }
    }
  }
}
