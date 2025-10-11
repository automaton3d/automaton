/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
 */

#include "simulation.h"
bool ctrl = true;
namespace automaton
{

  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.t == curr.d)
    {
      // Assure it is unique
      if (curr.t == RMAX / 2 && curr.pB && curr.x[3] == 0 &&
    	 !curr.cB && curr.a != W_DIM && ctrl)
      {
    	draft.c[0] = curr.x[0];  // This is the pBit location
        draft.c[1] = curr.x[1];
        draft.c[2] = curr.x[2];
        draft.cB = true;
        draft.a = W_DIM;
        ctrl = false;
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

  int count = 0;

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
      draft.cB = (north.cB && curr.d <= north.d) ||
                 (west.cB && curr.d <= west.d)   ||
                 (down.cB && curr.d <= down.d)   ||
                 (south.cB && curr.d <= south.d) ||
                 (east.cB && curr.d <= east.d)   ||
                 (up.cB && curr.d <= up.d);
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
      // Erase all cB's	and change time to the last tick, so in the next FRAME cycle
      // it will be t=0.
      if (curr.cB)
      {
   	    draft.cB = false;
        draft.t = RMAX - 1;
        // Reset propagation status
        draft.kB = false;
        draft.hB = false;
        draft.bB = false;
      }
    }
  }

  /**
   * Grid relocation.
   */
  void relocate(Cell& curr, Cell &draft, Cell &north, Cell &west, Cell &down)
  {
    /****** SLOT V ******/
    if (curr.k < SLOT5 && north.c[0] > 0)
    {
      draft = north;
      draft.c[0]--;
    }
    /****** SLOT VI ******/
    else if (curr.k < SLOT6 && west.c[1] > 0)
    {
      draft = west;
      draft.c[1]--;
    }
    /****** SLOT VII ******/
    else if (curr.k < SLOT7 && down.c[2] > 0)
    {
      draft = down;
      draft.c[2]--;
    }
  }

  /**
   * Prepares new wavefront.
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
