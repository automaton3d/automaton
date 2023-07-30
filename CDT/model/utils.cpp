/*
 * utils.c
 *
 *  Created on: 12 de jun. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"
#include "mygl.h"

namespace framework
{
  extern std::vector<Tickbox> checkboxes;
}

namespace automaton
{
extern Cell *latt0;


COLORREF *voxels;

bool isCentralPoint(int i)
{
  int x = i % SIDE2 - SIDE_2;
  int y = (i / SIDE2) % SIDE2 - SIDE_2;
  int z = (i / SIDE4) % SIDE2 - SIDE_2;
  if(x < 0 || y < 0 || z < 0)
    return false;
  return x % SIDE == 0 && y % SIDE == 0 && z % SIDE == 0;
}

bool isPartial(int i)
{
  int x = i % SIDE2 - SIDE_2;
  int y = (i / SIDE2) % SIDE2 - SIDE_2;
  int z = (i / SIDE4) % SIDE2 - SIDE_2;
  for (int j = -1; j < 2; j++)
    for (int k = -1; k < 2; k++)
      for (int l = -1; l < 2; l++)
      {
        if(x + j < 0 || y + k < 0 || z + l < 0)
          return false;
        if((x + j) % SIDE == 0 && (y + k) % SIDE == 0 && (z + l) % SIDE == 0)
          return true;
      }
  return false;
}

void updateBuffer()
{
  if (!framework::active || framework::checkboxes.empty())
    return;
  bool wavefront = ((framework::Tickbox)framework::checkboxes[0]).getState();
  bool momentum  = ((framework::Tickbox)framework::checkboxes[1]).getState();
  bool lattice   = ((framework::Tickbox)framework::checkboxes[4]).getState();
  bool single    = framework::dataset[0].isSelected();
  bool partial   = framework::dataset[1].isSelected();
  bool full      = framework::dataset[2].isSelected();
  bool rnd       = framework::dataset[3].isSelected();
  Cell *stb = latt0;
  for(int i = 0; i < SIDE6; i++, stb++)
  {
    bool ok = full || (partial && isPartial(i)) ||
        (single && isCentralPoint(i)) || (rnd && rand() % 100 == 0);
      if(!ZERO(stb->p) && momentum && ok)
        voxels[i] = RGB(255,0,0);
      else if(!ZERO(stb->s) && wavefront && ok)
        voxels[i] = RGB(0, 255,0);
      else if (lattice && isCentralPoint(i))
        voxels[i] = RGB(150,150,150);
      else
        voxels[i] = RGB(0,0,0);
  }
}

bool isEmpty(Cell *c)
{
  return (c->a1==0 &&
      c->a2==0 &&
      c->syn==0 &&
      c->u==0 &&
      c->k==0 &&
      c->obj==SIDE3 &&
      c->occ==0 &&
      ZERO(c->p) &&
      ZERO(c->s) &&
      ISSAT(c->o) &&
      ISSAT(c->po) &&
      ZERO(c->pP) &&
      ISSAT(c->m));
}

void empty(Cell *c)
{
    c->a1  = 0;
    c->a2  = 0;
    c->syn = 0;
    c->occ = 0;
    c->u   = 0;
    c->obj = SIDE3;
    c->k   = NOROLE;
    RSET(c->p);
    RSET(c->s);
    SAT(c->o);
    RSET(c->pP);
    SAT(c->m);
    SAT(c->po);
}

Cell *skew(Cell *c)
{
    int i = c->off;
    int xe = i / SIDE3;
    i %= SIDE3;
    int ye = i / SIDE4;
    i %= SIDE4;
    int ze = i / SIDE5;
    i %= SIDE5;

    int e = xe * SIDE3 + ye * SIDE4 + ze * SIDE5;
    return (c - c->off) + e + ((i + 1) % SIDE3);
}

}

void normalize(float vec[3])
{
    float length = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
}
