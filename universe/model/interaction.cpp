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

  /**
   * Analizes the cases when the wavefronts cross
   * during the convolution process.
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
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
            curr.c[0] == 0 && curr.c[1] == 0 && curr.c[2] == 0 &&
			curr.a != W_DIM)
        {
          // Who has the pB true interacts once
          if (curr.pB && !mirror.pB)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
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
          if (!curr.kB && !mirror.kB && curr.Q() == !mirror.Q() &&
               curr.W0() == !mirror.W0() &&
         curr.COLOR() == mirror.ANTICOLOR() &&
               curr.f == curr.t && mirror.f == mirror.t)
          {
            // Reissue from C.P.
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.kB = true;
            draft.a = curr.x[3];  // TODO: checar se propaga
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

  /**
   * Complete.
   */
  void diffuse(Cell& curr, Cell &draft)
  {
    // Calculate neighbors
    Cell &forward = curr.getNeighbor(FORWARD);
    Cell &north   = curr.getNeighbor(NORTH);
    Cell &west    = curr.getNeighbor(WEST);
    Cell &down    = curr.getNeighbor(DOWN);
    Cell &south   = curr.getNeighbor(SOUTH);
    Cell &east    = curr.getNeighbor(EAST);
    Cell &up      = curr.getNeighbor(UP);
    // Window for hunting in 3d space
    if (curr.k < CONVOL + 3 *(EL - 1))
    {
      // Hunting happens on the active shell
      if (curr.d == curr.t)
      {
        // Propagate hunting
        if (north.hB)
        {
          if (curr.sB)
          {
        	// Bit sB was found
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
            draft.c[0] = (north.c[0] + 1) % EL;
          }
        }
        else if (west.hB)
        {
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
            draft.c[1] = (west.c[1] + 1) % EL;
          }
        }
        else if (down.hB)
        {
          if (curr.sB)
          {
            draft.hB = false;
            draft.cB = true;
          }
          else
          {
            draft.hB = true;
            draft.c[2] = (down.c[2] + 1) % EL;
          }
        }
      }
    }
    else if (curr.k < CONVOL + 3 * (EL - 1) + W_DIM)
    {
      // Collapse happens in cell with the same affinity
      if (forward.kB && forward.a == curr.a)
      {
    	  puts("propag collapse");
        // Diffuse the collapse flag
        draft.kB = true;
        // Diffuse hunting bit
        draft.hB = forward.hB;
        // Diffuse the relocation vector c
        draft.c[0] = forward.c[0];
        draft.c[1] = forward.c[1];
        draft.c[2] = forward.c[2];
      }
      // Diffuse the frequency variable
      draft.f = max(forward.f, curr.f);
    }
    // Window for diffusion in 3D space
    else if (curr.k < CONVOL + W_DIM + 6 * (EL - 1))
    {
      // Diffuse the collapse flag in 3D space
      if (north.kB)
      {
        draft.kB = true;
        if (north.a == north.x[3])
        {
          // Dissolve the particle
          draft.a = curr.x[3];
        }
      }
      else if (west.kB)
      {
        draft.kB = true;
        if (west.a == west.x[3])
        {
          // Dissolve the particle
          draft.a = curr.x[3];
        }
      }
      else if (down.kB)
      {
        draft.kB = true;
        if (down.a == down.x[3])
        {
          // Dissolve the particle
          draft.a = curr.x[3];
        }
      }
      // Diffuse the frequency variable in 3D space
      draft.f = max(down.f, max(west.f, max(north.f, curr.f)));
      // Diffuse the relocation vector c in 3D space
      if ((north.c[0] | north.c[1] | north.c[2]) ||
    	  (west.c[0] | west.c[1] | west.c[2]) ||
		  (down.c[0] | down.c[1] | down.c[2]))
      {
        // Este código é executado
        draft.c[0] = north.c[0];
        draft.c[1] = north.c[1];
        draft.c[2] = north.c[2];
      }
    }
    else if (curr.k < CONVOL + W_DIM + 9*(EL - 1))
    {
      // All hB bits must be reset for the next cycle
      draft.hB = false;
      // Bubble contraction
      if (!curr.cB)
      {
        if ((north.cB && north.d == curr.d + 1) ||
            (west.cB  && west.d  == curr.d + 1) ||
            (down.cB  && down.d  == curr.d + 1) ||
            (south.cB && south.d == curr.d + 1) ||
            (east.cB  && east.d  == curr.d + 1) ||
            (up.cB    && up.d    == curr.d + 1))
        {
          // Código é executado
          draft.cB = true;
          draft.t = 0;
          // Turn the abandoned wavefront an orphan
          if (curr.d == curr.t)
            draft.a = W_DIM;
        }
      }
    }
  }

  /**
   * Not Complete.
   */
  void relocate(Cell& curr, Cell &draft)
  {
    // All cB bits must be reset;
    draft.cB = false;
    // Calculate neighbors
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
    // Reset diffusion variable for the next cycle
    draft.cB = true;
  }

  /*
   * Tests color neutrality.
   */
  bool neutralColor(Cell &a, Cell &b)
  {
    int color_a = a.ch & 0x07;
    int color_b = b.ch & 0x07;
    return (color_a ^ color_b) == 0x07;
  }

  /*
   * Tests weak neutrality.
   */
  bool neutralWeak(Cell &a, Cell &b)
  {
    int weak_a = (a.ch >> 3) & 0x03;
    int weak_b = (b.ch >> 3) & 0x03;
    return (weak_a ^ weak_b) == 0x03;
  }

}
