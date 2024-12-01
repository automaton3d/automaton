/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include "simulation.h"

namespace automaton
{
  using namespace std;

  extern EntropyCalculator entropyCalc;

  // Grid constants
  const unsigned SIDE2 = SIDE * SIDE;
  const unsigned SIDE3 = SIDE2 * SIDE;
  const unsigned W_DIM = 3 * SIDE2;
  const unsigned BLOCK = SIDE3 * W_DIM;

  // Dynamics constants
  const unsigned DIAG = (unsigned) SIDE * sqrt(3);
  const unsigned RMAX = DIAG / 2;
  const unsigned FMAX = RMAX / 2;
  const unsigned CONVOL = W_DIM + 1;  // +1 needed for temp
  const unsigned COLLISION = CONVOL + 1;
  const unsigned W_DIFFUSION = COLLISION + W_DIM;
  const unsigned XYZ_DIFFUSION = W_DIFFUSION + RMAX;
  const unsigned RELOC = XYZ_DIFFUSION + 3 * SIDE;
  const unsigned UPDATE = RELOC;
  const unsigned LIGHT = (SIDE - 1) / 2;
  const unsigned RANGE = RMAX * LIGHT;
  const unsigned FRAME = UPDATE + LIGHT;

  vector<Cell> lattice_current(BLOCK);
  vector<Cell> lattice_draft(BLOCK);
  vector<Cell> lattice_mirror(BLOCK);

  // Used for circular shift in the W dimension
  bool shift;

  /*
   * Executes a one tick operation in every cell.
   */
  void update_lattice()
  {
    for (unsigned index = 0; index < BLOCK; ++index)
    {
      Cell &current = lattice_current[index];
      Cell &draft = lattice_draft[index];
      Cell &mirror = lattice_mirror[index];
      shift = false;
      if (current.ctrl)
      {
        // UPDATE step
        if (current.k < CONVOL)
        {
          if (current.k == 0)
          {
            mirror = current;  // Copy current to mirror on first tick
          }
          else if (current.wv && mirror.wv)
          {
            // Handle wavefront clash if needed
          }
          // shift = true;  // Perform a circular shift in the W dimension
        }
        else if (current.k < COLLISION)
        {
          if (current.pole && current.wv && current.t > 2*LIGHT && rand() % 3 == 0)
          {
            draft.t = 0;//RANGE;
            assert(draft.c[0]==0 && draft.c[1]==0 && draft.c[2]==0);
            draft.c[rand() % 3] = (rand() % 2) ? 1 : SIDE - 1;
          }
        }
        else if (current.k < W_DIFFUSION)
        {
          // Handle W Diffusion if needed
          draft.wv = false;
        }
        else if (current.k < XYZ_DIFFUSION)
        {
          for (int dir = 0; dir < 6; ++dir)
          {
            unsigned nIndex = getNeighbor(index, dir);
            Cell &nei = lattice_current[nIndex];
            if (nei.c[0] > current.c[0])
              draft.c[0] = nei.c[0];
            if (nei.c[1] > current.c[1])
              draft.c[1] = nei.c[1];
            if (nei.c[2] > current.c[2])
              draft.c[2] = nei.c[2];
            //draft.freq = nei.freq;
            //draft.A.a = 0;
            /*
                else if (current.A.a == 0 && neighbor.A.a > 0)
                {
                  draft.A.a = neighbor.A.a;
                }
            */
          }
        }
        else if (current.k < RELOC)
        {
          for (unsigned dir : {NORTH, WEST, DOWN})
          {
            unsigned nIndex = getNeighbor(index, dir);
            Cell &nei = lattice_current[nIndex];
            unsigned coord = (dir == NORTH) ? 0 : (dir == WEST) ? 1 : 2;
            if (nei.c[coord] > 0)
            {
              //printf("c[]=%u k=%u t=%u\n", nei.c[coord], current.k, current.t); fflush(stdout);
              /*
              // Copy pos and c arrays
              copy(nei.pos, nei.pos + 3, draft.pos);
              copy(nei.c, nei.c + 3, draft.c);
              // Copy other members
              draft.pole = nei.pole;
              draft.d = nei.d;
              draft.sin = nei.sin;
              draft.aff = nei.aff;
              draft.angle = nei.angle;
              draft.e = nei.e;
              */
              draft = nei;
              // Update the specific component of `c`
              draft.c[coord] = nei.c[coord] - 1;
              //printf("\tc[]=%u k=%u t=%u\n", draft.c[coord], current.k, current.t); fflush(stdout);
            }
            draft.wv = false;
            draft.A.a = 0;
          }
        }
        // Finalize the update step
        draft.k = current.k + 1;
        if (draft.k == UPDATE)
        {
          draft.k = 0;
          draft.ctrl = false;
        }
      }
      // Expansion phase
      else
      {
      if (current.t == RANGE)
      {
        //draft.t = RANGE;
      }
      else
      {
          if (current.t == current.d)
            draft.wv = true;
          for (int dir = 0; dir < 6; ++dir)
          {
            unsigned neighborIndex = getNeighbor(index, dir);
            Cell &neighbor = lattice_current[neighborIndex];
            if (neighbor.freq > 0)
              draft.freq = neighbor.freq;
          }
          if (current.angle == current.d)
            draft.A.a = current.sin;
          draft.angle += current.freq;
          if (draft.angle >= RANGE)
            draft.angle -= RANGE;
          if ((current.d ^ current.sin) % LIGHT == 0)
          {
            draft.e = false;
            draft.aff = draft.d ^ draft.sin;
          }
          draft.t = current.t + 1;
        }
        if (draft.t % LIGHT == 0)
        {
          draft.ctrl = true;
          draft.ph.a >>= 1;
          if (draft.t == RANGE)
          {
            draft.t = 0;
          }
        }
        draft.net_c0 = draft.net_c1 = draft.net_c2 = 0;
        draft.net_q = draft.net_w0 = draft.net_w1 = 0;
        draft.boson = false;
        draft.fxf = false;
        draft.collapse = false;
        fill(begin(draft.c), end(draft.c), 0);
        fill(begin(draft.m), end(draft.m), 0);
      }
    }
  }

  /*
   * Swap lattices after an update.
   */
  void swap_lattices()
  {
    if (shift)
    {
      for (size_t i = 0; i < BLOCK; ++i)
        lattice_current[i] = lattice_draft[(i + SIDE3) % BLOCK];
    }
    else
    {
      for (size_t i = 0; i < BLOCK; ++i)
        lattice_current[i] = lattice_draft[i];
    }
  }

  /*
   * One step of the CA.
   */
  void simulation()
  {
    update_lattice();
    swap_lattices();
    entropyCalc.updateEntropy();
  }

}
