/*
 * interaction.cpp
 *
 * This file contains important routines used to
 * evaluate interactions in the CA.
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

  bool ctrl = true; // debug

  // Wrapping periódico (toroidal) – usado apenas na relocação
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
   *
   * @curr the current lattice
   * @draft the draft lattice
   * @mirror the mirrored lattice
   */
  bool convolute(Cell& curr, Cell &draft, Cell &mirror)
  {
    switch(gConfig.simulation.scenario)
    {
      case 0:
        return convolute0(curr, draft, mirror);
      case 1:
        return convolute1(curr, draft, mirror);
      case 2:
        return convolute2(curr, draft, mirror);
      case 3:
        return convolute3(curr, draft, mirror);
      case 4:
        return convolute4(curr, draft, mirror);
      case 5:
        return convolute5(curr, draft, mirror);
      case 6:
        return convolute6(curr, draft, mirror);
      case 7:
        return convolute7(curr, draft, mirror);
      default:
      {
        exit(1);
      }
    }
    return false;
  }

  void diffuse(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up)
  {
      /****** SLOT I ******/
      if (curr.k < SLOT1)
      {
        /*--- Orphan propagation ---*/
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
      /****** SLOT II ******/
    if (curr.k < SLOT2)
    {
      /*--- Orphan propagation ---*/
      if ((north.a == W_USED && curr.d >= north.d) ||
          (west.a  == W_USED && curr.d >= west.d)  ||
          (down.a  == W_USED && curr.d >= down.d)  ||
          (south.a == W_USED && curr.d >= south.d) ||
          (east.a  == W_USED && curr.d >= east.d)  ||
          (up.a    == W_USED && curr.d >= up.d))
      {
        draft.a = W_USED;
      }
      /*--- Hunting using hB ---*/
      if (curr.d == effective_t(curr.t))
      {
        if (north.hB) { draft.c[0] = (north.c[0] + 1) % EL; curr.sB = !draft.hB; }
        else if (west.hB)  { draft.c[1] = (west.c[1] + 1) % EL; curr.sB = !draft.hB; }
        else if (down.hB)  { draft.c[2] = (down.c[2] + 1) % EL; curr.sB = !draft.hB; }
        else if (south.hB) { draft.c[1] = (south.c[1] + 1) % EL; curr.sB = !draft.hB; }
        else if (east.hB)  { draft.c[0] = (east.c[0] + 1) % EL; curr.sB = !draft.hB; }
        else if (up.hB)    { draft.c[2] = (up.c[2] + 1) % EL; curr.sB = !draft.hB; }
      }
    }
    /****** SLOT III ******/
    else if (curr.k < SLOT3)
    {
      if (!ZERO(north.c))
      {
        draft.c[0] = north.c[0];
        draft.c[1] = north.c[1];
        draft.c[2] = north.c[2];
        if (north.kB) draft.kB = north.kB;
      }
      if (!ZERO(west.c))
      {
        draft.c[0] = west.c[0];
        draft.c[1] = west.c[1];
        draft.c[2] = west.c[2];
        if (west.kB) draft.kB = west.kB;
      }
      if (!ZERO(down.c))
      {
        draft.c[0] = down.c[0];
        draft.c[1] = down.c[1];
        draft.c[2] = down.c[2];
        if (down.kB) draft.kB = down.kB;
      }
      draft.f = max(down.f, max(west.f, max(north.f,
                  max(south.f, max(east.f, up.f)))));
      // Diffuse CB toward center (d=0)
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
    /****** SLOT IV ******/
    else if (curr.k < SLOT4)
    {
      if (forward.kB && forward.a == curr.a)
      {
        int delta_x = (curr.x[0] - forward.x[0] + EL) % EL;
        int delta_y = (curr.x[1] - forward.x[1] + EL) % EL;
        int delta_z = (curr.x[2] - forward.x[2] + EL) % EL;

        draft.c[0] = (forward.c[0] + delta_x) % EL;
        draft.c[1] = (forward.c[1] + delta_y) % EL;
        draft.c[2] = (forward.c[2] + delta_z) % EL;
        draft.kB = forward.kB;
        draft.cB = forward.cB;
      }
      draft.f = max(forward.f, curr.f);
    }
    /****** SLOT V ******/
    else if (curr.k < SLOT5)
    {
      if (curr.a == W_USED)
      {
        if (curr.d < curr.t)
        {
          draft.a = curr.x[3];
        }
      }
    }
  }

  /**
   * Grid relocation – uses periodic (toroidal) wrapping.
   */
  void relocate(Cell& curr, Cell &draft, Cell &north, Cell &west, Cell &down)
  {
    // Save the 3D address
    unsigned x = curr.x[0];
    unsigned y = curr.x[1];
    unsigned z = curr.x[2];
    unsigned w = curr.x[3];

    /****** SLOT VI – deslocamento em X ******/
    if (curr.k < SLOT6)
    {
      if (north.c[0] > 0)
      {
        draft = north;
        draft.c[0]--;
        int nx = (int)draft.x[0] + 1;
        int ny = (int)draft.x[1];
        int nz = (int)draft.x[2];
        int nw = (int)draft.x[3];
        periodic_wrap(nx, ny, nz, nw);   // periódico, não esférico
        draft.x[0] = (unsigned)nx;
        draft.x[1] = (unsigned)ny;
        draft.x[2] = (unsigned)nz;
        draft.x[3] = (unsigned)nw;
      }
    }
    /****** SLOT VII – deslocamento em Y ******/
    else if (curr.k < SLOT7)
    {
      if (west.c[1] > 0)
      {
        draft = west;
        draft.c[1]--;
        int nx = (int)draft.x[0];
        int ny = (int)draft.x[1] + 1;
        int nz = (int)draft.x[2];
        int nw = (int)draft.x[3];
        periodic_wrap(nx, ny, nz, nw);   // periódico
        draft.x[0] = (unsigned)nx;
        draft.x[1] = (unsigned)ny;
        draft.x[2] = (unsigned)nz;
        draft.x[3] = (unsigned)nw;
      }
    }
    /****** SLOT VIII – deslocamento em Z ******/
    else if (curr.k < SLOT8)
    {
      if (down.c[2] > 0)
      {
        draft = down;
        draft.c[2]--;
        int nx = (int)draft.x[0];
        int ny = (int)draft.x[1];
        int nz = (int)draft.x[2] + 1;
        int nw = (int)draft.x[3];
        periodic_wrap(nx, ny, nz, nw);   // periódico
        draft.x[0] = (unsigned)nx;
        draft.x[1] = (unsigned)ny;
        draft.x[2] = (unsigned)nz;
        draft.x[3] = (unsigned)nw;
      }
    }

    // Recover 3D address (caso nenhum deslocamento tenha ocorrido)
    draft.x[0] = x;
    draft.x[1] = y;
    draft.x[2] = z;
    draft.x[3] = w;
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
      // Reset propagation status
      draft.kB = false;
      draft.hB = false;
      draft.bB = false;
      // Propagate normal affinity outward, overwriting normal or orphan
      if (curr.d == effective_t(curr.t))
      {
          if (north.d == curr.d + 1)
          {
              // Copy a from inner to outer cell
              draft.a = north.a;
          }
          if (south.d == curr.d + 1)
          {
              draft.a = south.a;
          }
          if (east.d == curr.d + 1)
          {
              draft.a = east.a;
          }
          if (west.d == curr.d + 1)
          {
              draft.a = west.a;
          }
          if (up.d == curr.d + 1)
          {
              draft.a = up.a;
          }
          if (down.d == curr.d + 1)
          {
              draft.a = down.a;
          }
      }
      if (curr.cB)
      {
        // Consume cB
        draft.cB = false;
        if (curr.a != W_USED && curr.d < 2)
        {
          draft.t = 0;
        }
      }
  }

  void flood(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up)
  {
    if (curr.a != W_USED)
      draft.t = min({ north.t, south.t, east.t, west.t, down.t, up.t });
  }
}