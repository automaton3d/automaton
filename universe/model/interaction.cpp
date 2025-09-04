/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
 *
 * If necessary, calculates the relocation offset.
 *
 */

#include "simulation.h"

namespace automaton
{

  extern bool reloc_x[W_DIM], reloc_y[W_DIM], reloc_z[W_DIM];

  /*
   * Analizes the cases when the wavefronts cross
   * during the convolution process.
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
	if (curr.t == curr.d && mirror.t == mirror.d)
	{
	  // Test superposition
	  if (curr.f == curr.t && mirror.f == mirror.t)
	  {
	    // Overlapping:
		// Fully symmetric?
		if (curr.ch == (~mirror.ch & CHARGE_MASK))
		{
		  draft.f += curr.t;
		  draft.hB &= curr.phiB;
		}
	  }
	  else
	  {
        // Distinct:
        // Annihilation
		if (!curr.kB && !mirror.kB && curr.W1() == mirror.W1() && curr.Q() == !mirror.Q() && curr.W0() == !mirror.W0() && curr.COLOR() == mirror.ANTICOLOR())
		{
          draft.kB = true;
		}
		else if (curr.ch == mirror.ch)
		{
		  draft.a = min(curr.a, mirror.a);
		}
	  }
	}
    return false;
  }

  /*
   * Complete.
   */
  void diffuse(Cell& curr, Cell &draft, Cell &mirror)
  {
	// Diffuse the collapse flag in W dimension
	Cell &forward = curr.getNeighbor(FORWARD);
	Cell &north   = curr.getNeighbor(NORTH);
	Cell &west    = curr.getNeighbor(WEST);
	Cell &down    = curr.getNeighbor(DOWN);
	if (forward.kB && forward.a == curr.a)
		draft.kB = true;
	// Diffuse the collapse flag in 3D space
	else if (north.kB)
		draft.kB = true;
	else if (west.kB)
		draft.kB = true;
	else if (down.kB)
		draft.kB = true;
	// Diffuse the frequency variable in 3D space
	draft.f = max(down.f, max(west.f, max(north.f, curr.f)));
	// Diffuse the relocation vector c in dimension W
	if (forward.kB && forward.a == curr.a)
	{
	  draft.c[0] = forward.c[0];
	  draft.c[1] = forward.c[1];
	  draft.c[2] = forward.c[2];
	}
	// Diffuse the relocation vector c in 3D space
	else if (north.kB)
	{
	  draft.c[0] = north.c[0];
	  draft.c[1] = north.c[1];
	  draft.c[2] = north.c[2];
	}
	else if (west.kB)
	{
	  draft.c[0] = west.c[0];
	  draft.c[1] = west.c[1];
	  draft.c[2] = west.c[2];
	}
	else if (down.kB)
	{
	  draft.c[0] = down.c[0];
	  draft.c[1] = down.c[1];
	  draft.c[2] = down.c[2];
	}
  }

  /*
   * Not Complete.
   */
  void relocate(Cell& curr, Cell &draft, Cell &mirror)
  {
	  Cell &north = curr.getNeighbor(NORTH);
	  Cell &west  = curr.getNeighbor(WEST);
	  Cell &down  = curr.getNeighbor(DOWN);
      if (north.c[0] > 0)
      {
    	reloc_x[curr.x[3]];
        draft.c[0] = curr.c[0] - 1;
      }
      else if (west.c[1] > 0)
      {
      	reloc_y[curr.x[3]];
        draft.c[1] = curr.c[1] - 1;
      }
      else if (down.c[2] > 0)
      {
      	reloc_z[curr.x[3]];
        draft.c[2] = curr.c[2] - 1;
      }
  }

  void transport(Cell& curr, Cell &draft, Cell &mirror)
  {
	  /*
    // Is the last tick of Convolution?
    if (curr.k == CONVOL - 1)
    {
      // Copy back the modified data
      draft.m[0] = mirror.m[0];
      draft.m[1] = mirror.m[1];
      draft.m[2] = mirror.m[2];
      draft.c[0] = mirror.c[0];
      draft.c[1] = mirror.c[1];
      draft.c[2] = mirror.c[2];
    }
    else
    {
      // Candidate to be a master?
      if (curr.pole && ZERO(curr.c))
      {
        // Test the conditions to be a slave in mirror
        if (!mirror.pole &&	curr.aff == mirror.aff && ZERO(mirror.c))
        {
          // There is a slave in mirror.
          // Hint interaction type (a)
          draft.fxb = true;
          // Master look-ahead relocate his own pole
          draft.c[0] = curr.pos[0];
          draft.c[1] = curr.pos[1];
          draft.c[2] = curr.pos[2];
          // Mark the slave with a look-ahead transport
          mirror.m[0] = curr.pos[0];
          mirror.m[1] = curr.pos[1];
          mirror.m[2] = curr.pos[2];
          // Slave must relocate to his own pole
          // Safe: there is no race condition
          mirror.c[0] = mirror.pos[0];
          mirror.c[1] = mirror.pos[1];
          mirror.c[2] = mirror.pos[2];
        }
      }
    }
    */
  }

}
