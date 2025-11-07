/*
 * convolutes.cpp
 */
#include "simulation.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;

  extern bool ctrl;

  /**
   * Wrapping test.
   * Suggested paramaters in Splash: L=9, M=10
   *
   * @return true if failed any sanity test.
   */
  bool convolute0(Cell& curr, Cell &draft, Cell &mirror)
  {
	return false;
  }

  /**
   * Relocate test.
   * Suggested paramaters in Splash: L=19, M=10
   *
   * @return true if failed any sanity test.
   */
  bool convolute1(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.t == curr.d && curr.t == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
   	  draft.c[0] = getRandomUnsigned(EL);
      draft.c[1] = getRandomUnsigned(EL);
      draft.c[2] = getRandomUnsigned(EL);
      ctrl = false;
    }
	return false;
  }

  /**
   * Orphan expansion test.
   * Suggested paramaters in Splash: L=19, M=10
   *
   * @return true if failed any sanity test.
   */
  bool convolute2(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.t == curr.d && curr.t == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      ctrl = false;
    }
	return false;
  }

  /**
   * Contraction test.
   * Suggested paramaters in Splash: L=19, M=10
   *
   * @return true if failed any sanity test.
   */
  bool convolute3(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.t == curr.d && curr.t == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      draft.cB = true;
      ctrl = false;
    }
	return false;
  }

  /**
   * Hunting test.
   * Suggested paramaters in Splash: L=9, M=10
   *
   * @return true if failed any sanity test.
   */
  bool convolute4(Cell& curr, Cell &draft, Cell &mirror)
  {
    // Assure it is unique
    if (curr.t == curr.d && curr.t == RMAX / 2 && curr.sB && curr.x[3] == 0 && ctrl)
    {
      draft.hB = true;
      ctrl = false;
    }
	return false;
  }

  /**
   * Reissue test.
   *
   * @return true if failed any sanity test.
   */
  bool convolute5(Cell& curr, Cell &draft, Cell &mirror)
  {
    // Assure it is unique
    if (curr.t == curr.d && curr.t == RMAX / 2 && curr.pB && curr.x[3] == 0 &&
       !curr.cB && curr.a != W_USED && ctrl)
    {
      draft.c[0] = curr.x[0];  // This is the pBit location
      draft.c[1] = curr.x[1];
      draft.c[2] = curr.x[2];
      draft.cB = true;
      draft.a = W_USED;
      ctrl = false;
    }
    return false;
  }

  /**
   * Dispersion test.
   *
   * @return true if failed any sanity test.
   */
  bool convolute6(Cell& curr, Cell &draft, Cell &mirror)
  {
    // Cells awaken?
    if (curr.t == curr.d && mirror.t == mirror.d)
    {
      // Test superposition
      if (curr.x[0] == mirror.x[0] &&
    	  curr.x[1] == mirror.x[1] &&
          curr.x[2] == mirror.x[2])
      {
        // Test dispersion
        if (curr.a != W_USED &&
        	curr.W1() != mirror.W1() &&
			!curr.cB &&
        	curr.t == RMAX / 2)
        {
          if (curr.pB && mirror.sB)
          {
            // Reissue from pBit point.
            draft.c[0] = curr.x[0];  // This is the pBit location
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            // Trigger a contraction
            draft.cB = true;
            // Orphan seed
            draft.a = W_USED;
            printf("pB [k=%d: (%d,%d,%d) pB L=%d M=%d]\n", curr.x[0],curr.x[1],curr.x[2], curr.k, curr.x[3], mirror.x[3]); fflush(stdout);
          }
          else if (curr.sB && !mirror.pB)
          {
            // Reissue from sB
            draft.hB = true;
            draft.cB = true;
            // Orphan seed
            draft.a = W_USED;
            printf("sB [k=%d: (%d,%d,%d) NOT pB L=%d M=%d}\n", curr.x[0],curr.x[1],curr.x[2], curr.k, curr.x[3], mirror.x[3]); fflush(stdout);
          }
        }
      }
    }
    return false;
  }

  /**
   * Full simulation.
   * Analizes the cases when the wavefronts cross
   * during the convolution process.
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   *
   * @return true if failed any sanity test.
   */
  bool convolute7(Cell& curr, Cell &draft, Cell &mirror)
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
           !curr.cB && curr.a != W_USED)
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
    }
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
    return false;
  }

}
