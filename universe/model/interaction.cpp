/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
 */

#include "simulation.h"

namespace automaton
{

  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.t == curr.d)
    {
      if (curr.t == RMAX / 2 && curr.pB)
      {
    	draft.c[0] = curr.x[0];  // This is the pBit location
        draft.c[1] = curr.x[1];
        draft.c[2] = curr.x[2];
        draft.cB = true;
      }
    }
    return false;
  }

  /**
   * Analizes the cases when the wavefronts cross
   * during the convolution process.
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  bool convolute2(Cell& curr, Cell &draft, Cell &mirror)
  {
    // Cells awaken?
    if (curr.t == curr.d && mirror.t == mirror.d)
    {
      // Test superposition
      if (curr.x[0] == mirror.x[0] && curr.x[1] == mirror.x[1] &&
        curr.x[2] == mirror.x[2])
      {
        // Test dispersion
        if (curr.W1() != mirror.W1() && curr.t == RMAX / 2 &&
           !curr.cB && curr.a != W_DIM)
        {
          // Who has the pB true interacts once
          if (curr.pB && !mirror.pB)
          {
            // Reissue from pBit point.
            draft.c[0] = curr.x[0];  // This is the pBit location
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            // Trigger a contraction
            draft.cB = true;
          }
          // Who has pB false interacts with the last pB true
          if (!curr.pB && mirror.pB)
          {
            // Reissue from sB
            draft.hB = true;
            draft.cB = true;
          }
        }
        // Test single pair
        else if (curr.f == curr.t && mirror.f == mirror.t)
        {
          /*
        // Different sectors?
        if (curr.W1() != mirror.W1())
        {
          // Momentum
          if (curr.pB && mirror.pB)  // Nor correct!!!! never happens
          {
            if (curr.t > 0)
              printf("pB=pB t=%d\n", curr.t);
            // Graviton
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = min(curr.a, mirror.a);
          }
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
        else if ((curr.ch == 0 && mirror.ch == 0) || (curr.ch == 63 &&
              mirror.ch == 63))
        {
          // Neutrino
          draft.f += curr.t;
          draft.s2B &= curr.phiB;
          draft.a = min(curr.a, mirror.a);
        }
        else if ((!curr.Q() && !mirror.Q()) && (!curr.W1() &&
              !mirror.W1()) && (curr.W0() && mirror.W0()) &&
        (curr.COLOR() == mirror.COLOR()) &&
                  (curr.COLOR() != 0 && curr.COLOR() != 7))
        {
          // Boson W-
          draft.f += curr.t;
          draft.s2B &= curr.phiB;
          draft.a = min(curr.a, mirror.a);
          draft.bB = true;
        }
        else if ((curr.Q() && mirror.Q()) && (curr.W1() && mirror.W1()) && (!curr.W0() &&
                 !mirror.W0()) && (curr.COLOR() == mirror.COLOR()) &&
                 (curr.COLOR() != 0 && curr.COLOR() != 7))
        {
          // Boson W+
          draft.f += curr.t;
          draft.s2B &= curr.phiB;
          draft.a = min(curr.a, mirror.a);
          draft.bB = true;
        }
        else if ((curr.Q() != mirror.Q()) && (curr.W1() &&
              mirror.W1()) && (!curr.W0() && !mirror.W0()) &&
         (curr.COLOR() == mirror.COLOR()) &&
                 (curr.COLOR() != 0 && curr.COLOR() != 7))
        {
          // Boson Z
          draft.f += curr.t;
          draft.s2B &= curr.phiB;
          draft.a = min(curr.a, mirror.a);
          draft.bB = true;
        }
        */
      }
      /*
      // Blob formation
      else if(curr.f != curr.t && mirror.f != mirror.t && curr.bB)
      {
        draft.f += curr.f + mirror.f;
        draft.s2B &= curr.phiB;
        draft.a = min(curr.a, mirror.a);
      }
      */
    }
    /*
    else
    {
      // Distinct:
      if (curr.W1() == mirror.W1())
      {
        // Same sector
        // Annihilation?
        if (!curr.kB && !mirror.kB && curr.Q() != mirror.Q() &&
             curr.W0() != mirror.W0() &&
       curr.COLOR() == mirror.ANTICOLOR() &&
             curr.f == curr.t && mirror.f == mirror.t)
        {
          // Reissue from C.P.
          draft.c[0] = curr.x[0];
          draft.c[1] = curr.x[1];
          draft.c[2] = curr.x[2];
          draft.kB = true;
          draft.a = curr.x[3];  // TODO: check if propagates
        }
        // Fermion cohesion?
        else if (curr.ch == mirror.ch && curr.f == curr.t &&
                 mirror.f == mirror.t)
        {
          if (curr.c[3] > mirror.c[3])
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
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
            draft.c[0] = curr.c[0];
            draft.c[1] = curr.c[1];
            draft.c[2] = curr.c[2];
          }
          // Parallel transport?
          else if (!curr.pB && mirror.pB)
          {
            draft.c[0] = EL + (curr.x[0] - mirror.x[0]) % EL;
            draft.c[1] = EL + (curr.x[1] - mirror.x[1]) % EL;
            draft.c[2] = EL + (curr.x[2] - mirror.x[2]) % EL;
          }
        }
        // Strong interaction
        else if (neutralColor(curr, mirror))
        {
          // Gluon x gluon
          if (curr.f > curr.t && mirror.f > mirror.t)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            // Exchange colors
            draft.ch = (curr.ch & ~COLOR_MASK) | (mirror.ch & COLOR_MASK);
          }
          // Quark x gluon
          else if (curr.f == curr.t && mirror.f > mirror.t)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            // Exchange colors
            draft.ch = (curr.ch & ~COLOR_MASK) | (mirror.ch & COLOR_MASK);
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
              draft.c[0] = curr.x[0];  // TODO: checar
              draft.c[1] = curr.x[1];
              draft.c[2] = curr.x[2];
              // Collapse
              draft.kB = true;
            }
          }
          // Electric interaction
          else if (curr.pB)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            if (mirror.pB)
            {
              // Collapse
              draft.kB = true;
            }
            else
            {
              // Properties exchange
              draft.a = mirror.a;
              draft.t = mirror.t;
              draft.c[0] = mirror.x[0];
              draft.c[1] = mirror.x[1];
              draft.c[2] = mirror.x[2];
              // TODO: The values of $a$ and $t$ spread to all cells in the layer
              // during this relocation.
            }
          }
          // Magnetic interaction
          else if (curr.sB)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            if (mirror.sB)
            {
              // Collapse
              draft.kB = true;
            }
            else
            {
              // Properties exchange
              draft.a = mirror.a;
              draft.t = mirror.t;
              draft.c[0] = mirror.x[0];
              draft.c[1] = mirror.x[1];
              draft.c[2] = mirror.x[2];
              // TODO: The values of $a$ and $t$ spread to all cells in the layer
              // during this relocation.
            }
          }
        }
      }
    }
    */
  }
    /*
    else
    {
      // Different sectors:
      // Singularization
      if (curr.ch == ((~mirror.ch) & CHARGE_MASK))
      {
        // Reissue from C.P.
        draft.c[0] = curr.x[0];
        draft.c[1] = curr.x[1];
        draft.c[2] = curr.x[2];
        draft.a = curr.x[3];
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
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            // Collapse
            draft.kB = true;
          }
        }
        // Electric interaction
        else if (curr.pB)
        {
          // Reissue from C.P.
          draft.c[0] = curr.x[0];
          draft.c[1] = curr.x[1];
          draft.c[2] = curr.x[2];
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
          draft.c[0] = curr.x[0];
          draft.c[1] = curr.x[1];
          draft.c[2] = curr.x[2];
          if (mirror.sB)
          {
            // Collapse
            draft.kB = true;
          }
        }
      }
    }
    */

    return false;
  }

  void diffuse(Cell& curr, Cell &draft, Cell &forward, Cell &north, Cell &west, Cell &down, Cell &south, Cell &east, Cell &up)
  {
    if (curr.k < SLOT1)
    {
      // Free the cell occupied by the orphan
      if ((north.a == W_DIM && north.d < curr.d) ||
    	  (west.a == W_DIM && west.d < curr.d) ||
		  (down.a == W_DIM && down.d < curr.d) ||
          (south.a == W_DIM && south.d < curr.d) ||
          (east.a == W_DIM && east.d < curr.d) ||
          (up.a == W_DIM && up.d < curr.d))
      {
    	if (curr.a == W_DIM)
          draft.a = curr.x[3];
      }
      // Patch
      if ((north.a == W_DIM && north.d > curr.d) ||
    	  (west.a == W_DIM && west.d > curr.d) ||
		  (down.a == W_DIM && down.d > curr.d) ||
          (south.a == W_DIM && south.d > curr.d) ||
          (east.a == W_DIM && east.d > curr.d) ||
          (up.a == W_DIM && up.d > curr.d))
      {
    	draft.a = W_DIM;
      }
      // Hunting using hB
      if (curr.d == curr.t)
      {
        if (north.hB)
        {
          // Update vector c
          draft.c[0] = (north.c[0] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
        else if (west.hB)
        {
          // Update vector c
          draft.c[1] = (west.c[1] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
        else if (down.hB)
        {
          // Update vector c
          draft.c[2] = (down.c[2] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
        else if (south.hB)
        {
          // Update vector c
          draft.c[0] = (south.c[0] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
        else if (east.hB)
        {
          // Update vector c
          draft.c[1] = (east.c[1] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
        else if (up.hB)
        {
          // Update vector c
          draft.c[2] = (up.c[2] + 1) % EL;
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
          }
        }
      }
    }
    else if (curr.k < SLOT2)
    {
      // Diffuse c vector in 3D
      if (north.c[0] || north.c[1] || north.c[2])
      {
        draft.c[0] = north.c[0];
    	draft.c[1] = north.c[1];
    	draft.c[2] = north.c[2];
    	if (north.kB)
    	  draft.kB = north.kB;
      }
      if (west.c[0] || west.c[1] || west.c[2])
      {
        draft.c[0] = west.c[0];
        draft.c[1] = west.c[1];
        draft.c[2] = west.c[2];
        // Diffuse collapse
        if (west.kB)
          draft.kB = west.kB;
      }
      if (down.c[0] || down.c[1] || down.c[2])
      {
        draft.c[0] = down.c[0];
        draft.c[1] = down.c[1];
        draft.c[2] = down.c[2];
        // Diffuse collapse
        if (down.kB)
          draft.kB = down.kB;
      }
      if (south.c[0] || south.c[1] || south.c[2])
      {
        draft.c[0] = south.c[0];
    	draft.c[1] = south.c[1];
    	draft.c[2] = south.c[2];
    	if (south.kB)
    	  draft.kB = south.kB;
      }
      if (east.c[0] || east.c[1] || east.c[2])
      {
        draft.c[0] = east.c[0];
        draft.c[1] = east.c[1];
        draft.c[2] = east.c[2];
        // Diffuse collapse
        if (east.kB)
          draft.kB = east.kB;
      }
      if (up.c[0] || up.c[1] || up.c[2])
      {
        draft.c[0] = up.c[0];
        draft.c[1] = up.c[1];
        draft.c[2] = up.c[2];
        // Diffuse collapse
        if (up.kB)
          draft.kB = up.kB;
      }
      // Propagate frequency
      draft.f = max(down.f, max(west.f, max(north.f, max(south.f, max(east.f, up.f)))));
    }
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
    else if (curr.k < SLOT4)
    {
      draft.hB = false;
      if (curr.d == curr.t && curr.cB)
      {
    	draft.a = W_DIM; // Mark as orphan
      }
      else if (curr.a == W_DIM)
      {
    	draft.a = W_DIM; // Preserve orphan
      }
      if (curr.d == curr.t && curr.t == RMAX / 2 && !curr.cB)
      {
    	if (north.cB || west.cB || down.cB || south.cB || east.cB || up.cB)
    	{
    	  draft.cB = true; // Propagate cB across shell
    	}
      }
      if (curr.d < curr.t)
      {
    	if (!curr.cB)
    	{
    	  if ((north.cB && north.d == curr.d + 1) ||
    	      (west.cB && west.d == curr.d + 1) ||
    	      (down.cB && down.d == curr.d + 1) ||
    	      (south.cB && south.d == curr.d + 1) ||
    	      (east.cB && east.d == curr.d + 1) ||
    	      (up.cB && up.d == curr.d + 1))
    	  {
    	    draft.cB = true; // Inward propagation
    	  }
    	}
      }
    }
  }

  /**
   * Grid relocation.
   */
  void relocate(Cell& curr, Cell &draft, Cell &north, Cell &west, Cell &down)
  {
    if (curr.k < DIFFUSION + (EL - 1) && north.c[0] > 0)
    {
      draft = north;
      draft.c[0]--;
    }
    else if (curr.k < DIFFUSION + 2*(EL - 1) && west.c[1] > 0)
    {
      draft = west;
      draft.c[1]--;
    }
    else if (curr.k < DIFFUSION + 3*(EL - 1) && down.c[2] > 0)
    {
      draft = down;
      draft.c[2]--;
    }
  }

  /**
   * Reset time for all marked cells.
   */
  void reissue2(Cell& curr, Cell &draft)
  {
    if (curr.cB)
    {
      draft.cB = false;
      draft.t = 0;
    }
  }

  // In interaction.cpp: Modify reissue()
  void reissue(Cell& curr, Cell &draft)
  {
    // Only reset the cell if it has completed its propagation (t == d)
    if (curr.cB && curr.t == curr.d)
    {
      draft.cB = false;
      draft.t = 0;
      // You may also want to set draft.d = 0 here if you want the cell to fully revert to white
    }
    // If curr.cB is true, but t < d, the wave should continue, so we do nothing here.
  }
}
