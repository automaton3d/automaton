/*
 * convolutes.cpp (CORRIGIDO)
 */

#include "model/simulation.h"
#include <algorithm> // para std::min

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;

  extern bool ctrl;

  bool convolute0(Cell& curr, Cell &draft, Cell &mirror)
  {
    return false;
  }

  bool convolute1(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.c[0] = getRandomUnsigned(EL);
      draft.c[1] = getRandomUnsigned(EL);
      draft.c[2] = getRandomUnsigned(EL);
      ctrl = false;
    }
    return false;
  }

  bool convolute2(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      ctrl = false;
    }
    return false;
  }

  bool convolute3(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && effective_t(curr.t) == RMAX / 2 && curr.x[3] == 0 && ctrl)
    {
      draft.a = W_USED;
      draft.cB = true;
      ctrl = false;
    }
    return false;
  }

  bool convolute4(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && effective_t(curr.t) == RMAX / 2 && curr.sB && curr.x[3] == 0 && ctrl)
    {
      draft.hB = true;
      ctrl = false;
    }
    return false;
  }

  bool convolute5(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && effective_t(curr.t) == RMAX / 2 && curr.pB && curr.x[3] == 0 &&
       !curr.cB && curr.a != W_USED && ctrl)
    {
      draft.c[0] = curr.x[0];
      draft.c[1] = curr.x[1];
      draft.c[2] = curr.x[2];
      draft.cB = true;
      draft.a = W_USED;
      ctrl = false;
    }
    return false;
  }

  bool convolute6(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && mirror.d == effective_t(mirror.t))
    {
      if (curr.x[0] == mirror.x[0] &&
          curr.x[1] == mirror.x[1] &&
          curr.x[2] == mirror.x[2])
      {
        if (curr.a != W_USED &&
            curr.W1() != mirror.W1() &&
            !curr.cB &&
            effective_t(curr.t) == RMAX / 2)
        {
          if (curr.pB && mirror.sB)
          {
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.cB = true;
            draft.a = W_USED;
          }
          else if (curr.sB && !mirror.pB)
          {
            draft.hB = true;
            draft.cB = true;
            draft.a = W_USED;
          }
        }
      }
    }
    return false;
  }

  bool convolute7(Cell& curr, Cell &draft, Cell &mirror)
  {
    if (curr.d == effective_t(curr.t) && mirror.d == effective_t(mirror.t))
    {
      if (curr.x[0] == mirror.x[0] &&
          curr.x[1] == mirror.x[1] &&
          curr.x[2] == mirror.x[2])
      {
        if (curr.W1() != mirror.W1() &&
            effective_t(curr.t) == RMAX / 2 &&
            !curr.cB &&
            curr.a != W_USED)
        {
          if (curr.pB && !mirror.pB)
          {
            draft.c[0] = curr.x[0];
            draft.c[1] = curr.x[1];
            draft.c[2] = curr.x[2];
            draft.cB = true;
          }

          if (!curr.pB && mirror.pB)
          {
            draft.hB = true;
            draft.cB = true;
          }
        }
        else if (curr.f == effective_t(curr.t) && mirror.f == effective_t(mirror.t))
        {
          if (curr.W1() != mirror.W1())
          {
            if (curr.pB && mirror.pB)
            {
              draft.f += curr.t;
              draft.s2B &= curr.phiB;
              draft.a = std::min(curr.a, mirror.a);
            }
          }
          else if ((curr.Q() ^ mirror.Q()) &&
                   (curr.W1() == mirror.W1()) &&
                   (curr.W0() ^ mirror.W0()) &&
                   (curr.C2() == mirror.C2()) &&
                   (curr.C1() == mirror.C1()) &&
                   (curr.C0() == mirror.C0()))
          {
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = std::min(curr.a, mirror.a);
            draft.bB = true;
          }
          else if ((curr.ch == 0 && mirror.ch == 0) ||
                   (curr.ch == 63 && mirror.ch == 63))
          {
            draft.f += curr.t;
            draft.s2B &= curr.phiB;
            draft.a = std::min(curr.a, mirror.a);
          }
        }
      }
      else
      {
        if (curr.W1() == mirror.W1())
        {
          if (curr.ch == mirror.ch &&
              curr.f == effective_t(curr.t) &&
              mirror.f == effective_t(mirror.t))
          {
            // ✅ CORREÇÃO AQUI
            if (curr.a > mirror.a)
            {
              draft.c[0] = curr.x[0];
              draft.c[1] = curr.x[1];
              draft.c[2] = curr.x[2];
              draft.a = std::min(curr.a, mirror.a);
            }
            else
            {
              draft.hB = true;
              draft.a = std::min(curr.a, mirror.a);
            }
          }
        }
      }
    }

    return false;
  }

}