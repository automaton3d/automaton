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

  void seggregation(Cell& curr, Cell &draft, Cell &mirror)
  {
    // Candidate to be a master?
    // Opposite w1 charges?
    if (ZERO(curr.c) && ZERO(mirror.c) && curr.pole && curr.aff != W_DIM && ((curr.charge ^ mirror.charge) & W1_MASK))
    {
      // Non trivial radius?
      // Superposing?
      if (curr.t > 4*LIGHT && curr.pos[0] == mirror.pos[0] && curr.pos[1] == mirror.pos[1] && curr.pos[2] == mirror.pos[2] && curr.aff != W_DIM)
      {
        draft.fxf = true;
        // Master relocates to its own pole
        draft.c[0] = (curr.pos[0] + SIDE - CENTER) % SIDE;
        draft.c[1] = (curr.pos[1] + SIDE - CENTER) % SIDE;
        draft.c[2] = (curr.pos[2] + SIDE - CENTER) % SIDE;
        // Slave relocates to the opposite master's pole
        mirror.c[0] = (CENTER - curr.pos[0] + SIDE) % SIDE;
        mirror.c[1] = (CENTER - curr.pos[1] + SIDE) % SIDE;
        mirror.c[2] = (CENTER - curr.pos[2] + SIDE) % SIDE;
      }
    }
  }

  /*
   * Analizes the cases when the wavefronts cross
   * during the convolution process.
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror, Cell &wleft, Cell &wright)
  {
	  /*
    // Ok, we have two active wavefront cells.
    // Separate the charges
    unsigned char c_current  = curr.charge   & COLOR_MASK;
    unsigned char c_mirror   = mirror.charge & COLOR_MASK;
    // unsigned char c_left     = wleft.charge  & COLOR_MASK;
    // unsigned char c_right    = wright.charge & COLOR_MASK;

    unsigned char w0_current = curr.charge   & W0_MASK;
    unsigned char w0_mirror  = mirror.charge & W0_MASK;
    // unsigned char w0_left    = wleft.charge  & W0_MASK;
    // unsigned char w0_right   = wright.charge & W0_MASK;

    unsigned char w1_current = curr.charge   & W1_MASK;
    unsigned char w1_mirror  = mirror.charge & W1_MASK;
    unsigned char w1_left    = wleft.charge  & W1_MASK;
    unsigned char w1_right   = wright.charge & W1_MASK;

    // Compute the weak force
    bool wf_current = w0_current != w1_current;
    bool wf_mirror  = w0_mirror != w1_mirror;
    // bool wf_left    = w0_left != w1_left;
    // Bubbles are superposing?
    // Radius > zero?
    if (mirror.pos[0] == curr.pos[0] &&
        mirror.pos[1] == curr.pos[1] &&
        mirror.pos[2] == curr.pos[2] && curr.t > LIGHT)
    {
      // Check if there is a segregation
      if (curr.pole)
      {
        // Current momentum non trivial.
        // Belong to different sectors?
        if (!curr.wxw && w1_mirror != w1_current)
        {
          // Segregation Orbis x Umbra:
          //  force immediate re-emission
          draft.wxw = true;
          draft.c[0] = (curr.pos[0] + SIDE - CENTER) % SIDE;
          draft.c[1] = (curr.pos[1] + SIDE - CENTER) % SIDE;
          draft.c[2] = (curr.pos[2] + SIDE - CENTER) % SIDE;
          draft.boson = false;      // invalidate boson check
          draft.collapse = false;   // invalidate collapse check
          return true;
        }
      }
      else
      {
        // Current momentum is trivial.
        // Belong to different sectors?
        if (!curr.wxw && wleft.pole && w1_mirror != w1_left)
        {
          // Segregation Orbis <.Umbra:
          //  force immediate re-emission
          draft.wxw = true;
          draft.c[0] = (curr.pos[0] + SIDE - CENTER) % SIDE;
          draft.c[1] = (curr.pos[1] + SIDE - CENTER) % SIDE;
          draft.c[2] = (curr.pos[2] + SIDE - CENTER) % SIDE;
          // invalidate boson check
          draft.boson = false;
          // invalidate collapse check
          draft.collapse = false;
          return true;
        }
      }
      // Count charges in dimension W
      compileNetCharges(mirror, draft);
      // boson x non boson
      if (draft.aff != mirror.aff)
      {
        // There is a rogue: invalidate as boson
        draft.boson = false;
      }
      // After the full circular shift, the rogue will have
      // spoiled all cells along the way
    }
    // Boson average amplitude
    draft.A_bar.a += mirror.A.a;
    draft.A_bar.a >>= 1;
    // Intersector operations
    if (w1_mirror != w1_right)
    {
      draft.wxw = true;
      return true;
    }
    // Both non trivial momenta?
    if (mirror.pole && curr.pole)
    {
      draft.collapse = true;  // hint enforce collapse
      // Opposite charges? (disregard w1 in this comparison)
      if (((curr.charge ^ mirror.charge) & 0x1F) == 0x1F)
      {
        // Hint annihilation
        draft.fxf = true;
      }
      else if (curr.boson)
      {
        // Hint fermion x boson
        draft.fxb = true;
      }
    }
    // At least one has non trivial momentum
    else if (mirror.pole || wleft.pole)
    {
      // We are in case (a) (see paper)
      // Are both bosons?
      if (curr.boson && mirror.boson)
      {
        // Test boson cohesion:
        //
        // Colored partners?
        if (c_mirror > 0 && c_mirror < 7 && c_current > 0 && c_current < 7)
        {
          // Check resultant color neutrality
          if (isColorNeutral(curr.charge, mirror.charge))
          {
            // Hint gluon x gluon
            draft.bxb = true;
          }
        }
        // Both have weak force?
        else if (wf_mirror && wf_current)
        {
          // Must have similar electric charges
          if (((curr.charge ^ mirror.charge) & Q_MASK) == 0)
          {
            // Hint W, Z boson
            draft.bxb = true;
          }
        }
      }
      // Assume now that none are bosons in case (a) or (c).
      // Same charge?
      else if (((curr.charge ^ mirror.charge) & Q_MASK) == 0)
      {
        // Hint fermion cohesion
        draft.fxf = true;
      }
      // Do we have already a hint for interaction?
      if (draft.fxf || draft.fxb || draft.bxb)
      {
        // Calculate look ahead value of relocation to c.p.
        draft.c[0] = (curr.pos[0] + SIDE - CENTER) % SIDE;
        draft.c[1] = (curr.pos[1] + SIDE - CENTER) % SIDE;
        draft.c[2] = (curr.pos[2] + SIDE - CENTER) % SIDE;
        // Is this bubble the slave?
        if (wleft.pole && !draft.fxf)
        {
          // Case (c):
          // Calculate look ahead value for parallel transport
          draft.m[0] = (mirror.pos[0] + SIDE - draft.pos[0]) % SIDE;
          draft.m[1] = (mirror.pos[1] + SIDE - draft.pos[1]) % SIDE;
          draft.m[2] = (mirror.pos[2] + SIDE - draft.pos[2]) % SIDE;
        }
      }
    }
    */
    return false;
  }

  /*
   * Counts the charges in dimension W.
   */
  void compileNetCharges(Cell &mirror, Cell &draft)
  {
    // Compile blindly net charges
    if (mirror.charge & C0_MASK)
      draft.net_c0++;
    else
      draft.net_c0--;
    //
    if (mirror.charge & C1_MASK)
      draft.net_c1++;
    else
      draft.net_c1--;
    //
    if (mirror.charge & C2_MASK)
      draft.net_c2++;
    else
      draft.net_c2--;
    //
    if (mirror.charge & Q_MASK)
      draft.net_q++;
    else
      draft.net_q--;
    //
    if (mirror.charge & W0_MASK)
      draft.net_w0++;
    else
      draft.net_w0--;
    //
    if (mirror.charge & W1_MASK)
      draft.net_w1++;
    else
      draft.net_w1--;
  }

  void executeInteraction(Cell &curr, Cell &draft)
  {
//    if (curr.fxf && curr.pos[0] == CENTER && curr.pos[1] == CENTER && curr.pos[2] == CENTER)
  //      draft.t = 0;
  }
  /*
   * Executes the interaction if all right.
   *
   * Converts hints (fxb, bxb, fxf, wxw, charge count) into
   * a decision.
   */
  void executeInteraction2(Cell &curr, Cell &draft)
  {
    // Avoid radius 0, not stable
    if (curr.wv && curr.pole && curr.t > 2 * LIGHT)
    {
      // Convert hints into decision
      bool fxb = curr.fxb && curr.boson;
      bool bxb = curr.bxb && curr.boson;
      bool fxf = curr.fxf && !curr.boson;
      bool wxw = curr.wxw;
      // Is this a boson cohesion?
      if (wxw)
      {
        draft.collapse = false;
        draft.t = 0;
        // Obs.: the relocation offset has been calculated in clash()
      }
      // This test covers annihilation, fermion cohesion and boson cohesion
      else if (fxf || bxb)
      {
        // Test threshold
        if (curr.A.a > CENTER && curr.ph.a > CENTER)
        {
          // Enforce relocation
          draft.t = 0;
          // Obs.: the relocation offset has been calculated in clash()
        }
      }
      // This test covers static forces and light-matter interaction
      else if (fxb)
      {
        // Enforce relocation
        draft.t = 0;
        // Obs.: the relocation offset has been calculated in clash()
      }
      else
      {
        // Disarm collapse, interaction did not complete
        draft .collapse = false;
      }
    }
  }

}
