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
    // Cells awaken?
    if (curr.t == curr.d && mirror.t == mirror.d)
    {
      // Test superposition
      if (curr.x[0] == mirror.x[0] && curr.x[1] == mirror.x[1] && curr.x[2] == mirror.x[2])
      {
        // Test single pair
        if (curr.f == curr.t && mirror.f == mirror.t && !curr.bB && !mirror.bB)
        {
          // Fully symmetric?
          if (curr.ch == (~mirror.ch & CHARGE_MASK))
          {
            // Graviton
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
            draft.bB = true;
          }
          else if ((curr.Q() ^ mirror.Q()) && (curr.W1() == mirror.W1()) &&
        		  (curr.W0() ^ mirror.W0()) && (curr.C2() == mirror.C2()) &&
				  (curr.C1() == mirror.C1()) && (curr.C0() == mirror.C0()))
          {
            // Photon
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
            draft.bB = true;
          }
          else if ((curr.ch == 0 && mirror.ch == 0) || (curr.ch == 63 && mirror.ch == 63))
          {
            // Neutrino
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
          }
          else if ((!curr.Q() && !mirror.Q()) && (!curr.W1() && !mirror.W1()) && (curr.W0() && mirror.W0()) && (curr.COLOR() == mirror.COLOR()) && (curr.COLOR() != 0 && curr.COLOR() != 7))
          {
            // Boson W-
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
          }
          else if ((curr.Q() && mirror.Q()) && (curr.W1() && mirror.W1()) && (!curr.W0() && !mirror.W0()) && (curr.COLOR() == mirror.COLOR()) && (curr.COLOR() != 0 && curr.COLOR() != 7))
          {
            // Boson W+
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
          }
          else if ((curr.Q() == !mirror.Q()) && (curr.W1() && mirror.W1()) && (!curr.W0() && !mirror.W0()) && (curr.COLOR() == mirror.COLOR()) && (curr.COLOR() != 0 && curr.COLOR() != 7))
          {
            // Boson Z
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
          }
        }
        // Blob formation
        else if(curr.f != curr.t && mirror.f != mirror.t && curr.bB)
        {
          draft.f += curr.f + mirror.f;
          draft.s2B &= curr.phiB;
          draft.a = min(curr.a, mirror.a);
        }
      }
      else
      {
        // Distinct:
    	if (curr.W1() == mirror.W1())
    	{
    	  // Same sector
          // Annihilation?
          if (!curr.kB && !mirror.kB && curr.Q() == !mirror.Q() &&
        	   curr.W0() == !mirror.W0() && curr.COLOR() == mirror.ANTICOLOR() &&
               curr.f == curr.t && mirror.f == mirror.t)
          {
            // Reissue from C.P.
            draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
            draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
            draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
            draft.kB = true;
            draft.a = curr.x[3];
          }
          // Fermion cohesion?
          else if (curr.ch == mirror.ch && curr.f == curr.t &&
                   mirror.f == mirror.t)
          {
            if (curr.c[3] > mirror.c[3])
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
              // Entangle
              draft.a = min(curr.a, mirror.a);
            }
            else
            {
              // Reissue from sB
        	  draft.hB = true;
              // Entangle
              draft.a = min(curr.a, mirror.a);
            }
          }
          // Inertia TODO:
          // Same affinity?
          else if (curr.a == mirror.a)
          {
            if (curr.pB && !mirror.pB)
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
            }
            // Parallel transport?
            else if (!curr.pB && mirror.pB)
            {
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
              // TODO draft.c[0] = (x1-x2) mod L Cuma???????
            }
          }
          // Strong interaction
          else if (neutralColor(curr, mirror))
          {
            // Gluon x gluon
            if (curr.f > curr.t && mirror.f > mirror.t)
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
            }
            // Quark x gluon
            else if (curr.f == curr.t && mirror.f > mirror.t)
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
            }
          }
          // Electroweak interaction:
          // Harmonic?
          else if (curr.phiB && mirror.phiB)
          {
            // Weak interaction
            if (neutralWeak(curr, mirror))
            {
              if ((curr.pB && !mirror.pB) || (curr.sB && mirror.sB))
              {
                // Reissue from C.P.
                draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
                draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
                draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
                // Collapse
                draft.kB = true;
              }
            }
            // Electric interaction
            else if (curr.pB)
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
              if (mirror.pB)
              {
                // Collapse
                draft.kB = true;
              }
            }
            // Magnetic interaction
            else if (curr.sB)
            {
              // Reissue from C.P.
              draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
              draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
              draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
              if (mirror.sB)
              {
                // Collapse
                draft.kB = true;
              }
            }
          }
        }
      }
	}
	else
	{
	  // Different sectors:
	  // Global annihilation
      // Electroweak interaction:
      // Harmonic?
      if (curr.phiB && mirror.phiB)
      {
        // Weak interaction
        if (neutralWeak(curr, mirror))
        {
          if ((curr.pB && !mirror.pB) || (curr.sB && mirror.sB))
          {
            // Reissue from C.P.
            draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
            draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
            draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
            // Collapse
            draft.kB = true;
          }
        }
        // Electric interaction
        else if (curr.pB)
        {
          // Reissue from C.P.
          draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
          draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
          draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
          if (mirror.pB)
          {
            // Collapse
            draft.kB = true;
          }
        }
        // Magnetic interaction
        else if (curr.sB)
        {
          // Reissue from C.P.
          draft.c[0] = EL + (curr.c[0] - mirror.c[0]) % EL;
          draft.c[1] = EL + (curr.c[1] - mirror.c[1]) % EL;
          draft.c[2] = EL + (curr.c[2] - mirror.c[2]) % EL;
          if (mirror.sB)
          {
            // Collapse
            draft.kB = true;
          }
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

    if (curr.d == curr.t)
    {
      // Propagate hunting
      if (north.kB)
      {
        draft.c[0] = (north.c[0] + 1) % EL;
        if (curr.d > 0)
      	  draft.hB = true;
      }
      else if (west.kB)
      {
        draft.c[1] = (west.c[0] + 1) % EL;
        if (curr.d > 0)
      	  draft.hB = true;
      }
      else if (down.kB)
      {
        draft.c[2] = (down.c[0] + 1) % EL;
        if (curr.d > 0)
      	  draft.hB = true;
      }
    }

    if (forward.kB && forward.a == curr.a)
      draft.kB = true;
    // Diffuse the collapse flag in 3D space
    else if (north.kB)
    {
      draft.kB = true;
      if (north.a == north.x[3])
      {
    	draft.a = curr.x[3];
      }
    }
    else if (west.kB)
    {
      draft.kB = true;
      if (west.a == west.x[3])
      {
    	draft.a = curr.x[3];
      }
    }
    else if (down.kB)
    {
      draft.kB = true;
      if (west.a == west.x[3])
      {
    	draft.a = curr.x[3];
      }
    }
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
    // Diffuse the time variable
    for (int dir = 0; dir < 6; dir++)
    {
      Cell &nei = curr.getNeighbor(dir);
      if (curr.k == FRAME - 1 && curr.t > nei.t)
      {
        draft.t = nei.t;
      }
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
    // Relocation via offset
    if (north.c[0] > 0)
    {
      reloc_x[curr.x[3]] = true;
      draft.c[0] = curr.c[0] - 1;
    }
    else if (west.c[1] > 0)
    {
      reloc_y[curr.x[3]] = true;
      draft.c[1] = curr.c[1] - 1;
    }
    else if (down.c[2] > 0)
    {
      reloc_z[curr.x[3]] = true;
      draft.c[2] = curr.c[2] - 1;
    }
  }

  bool neutralColor(Cell &a, Cell &b)
  {
    int color_a = a.ch & 0x07;
    int color_b = b.ch & 0x07;
    return (color_a ^ color_b) == 0x07;
  }

  bool neutralWeak(Cell &a, Cell &b)
  {
    int weak_a = (a.ch >> 3) & 0x03;
    int weak_b = (b.ch >> 3) & 0x03;
    return (weak_a ^ weak_b) == 0x03;
  }

}
