/*
 * utils.c
 *
 *  Created on: 12 de jun. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

namespace automaton
{

extern Cell *latt0;
extern boolean single, partial, full, momentum, wavefront, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;
extern boolean active;

COLORREF voxels[SIDE6];

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
	if (!active)
		return;

	momentum  = true;
	wavefront = true;

	single    = false;
	partial   = false;
	full      = true;
	rnd       = false;

	lattice   = false;
	track     = false;
	cube      = false;
	plane     = false;
	axes      = false;

	xy        = false;
	yz        = false;
	zx        = false;
	iso       = true;

//	pthread_mutex_lock(&mutex);
	Cell *stb = latt0;
	for(int i = 0; i < SIDE6; i++, stb++)
	{
		boolean ok = full || (partial && isPartial(i)) ||
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
//	pthread_mutex_unlock(&mutex);
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

}

void normalize(float vec[3])
{
    float length = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
    vec[0] /= length;
    vec[1] /= length;
    vec[2] /= length;
}

