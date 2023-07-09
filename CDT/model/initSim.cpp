/*
 * init.c
 *
 *  Created on: 23 de mai. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

short points[SIDE5][3];
int indx = 0;

namespace automaton
{
extern Cell *latt0, *latt1;

void initSpin(Cell *grid)
{
  int i = 0;
  Cell *ptr = grid;
  Cell *curr;
  for (int z = 0; z < SIDE; z++)
  {
    for (int y = 0; y < SIDE; y++)
    {
      for (int x = 0; x < SIDE; x++)
      {
    	if (i == SIDE3 - 1)
    	{
      	  CROSS(ptr->s, ptr->p, grid->p);
          double length = sqrt(ptr->s[0] * ptr->s[0] + ptr->s[1] * ptr->s[1] + ptr->s[2] * ptr->s[2]);
          if (length != 0)
          {
              ptr->s[0] = (int)round((ptr->s[0] / length) * SIDE);
              ptr->s[1] = (int)round((ptr->s[1] / length) * SIDE);
              ptr->s[2] = (int)round((ptr->s[2] / length) * SIDE);
          }
    	}
    	if (i > 0)
    	{
    	  CROSS(curr->s, curr->p, ptr->p);
          double length = sqrt(curr->s[0] * curr->s[0] + curr->s[1] * curr->s[1] + curr->s[2] * curr->s[2]);
          if (length != 0)
          {
        	  curr->s[0] = (int)round((curr->s[0] / length) * SIDE);
        	  curr->s[1] = (int)round((curr->s[1] / length) * SIDE);
        	  curr->s[2] = (int)round((curr->s[2] / length) * SIDE);
          }
    	}
        curr = ptr;
        i++; ptr++;
      }
      ptr += SIDE2 - SIDE;
    }
    ptr += SIDE4 - SIDE3;
  }
}

/*
 * Partial initialization of singularity.
 */
static void singularity(Cell *grid)
{
  Cell *ptr = grid;
  int i = 0;
  for (int z = 0; z < SIDE; z++)
  {
    for (int y = 0; y < SIDE; y++)
    {
      for (int x = 0; x < SIDE; x++)
      {
    	char w0  = i % 2;
    	char w1  = (i >> 1) % 2;
    	char q   = w0 ^ w1;
        ptr->ch  = (i % 8) | (w0 << 3) | (1 << 4) | (q << 5);
        ptr->a1  = i + 1;	                // dodges zero
        ptr->a2  = 0;                       // no chaining
        ptr->k   = FERMION;                 // intially just singles
        ptr->occ = SIDE_2 - 1;              // crest cell
        RSET(ptr->o);	                    // ready for immediate expansion
        RSET(ptr->po);                      // already at the reissue cell
        i++; ptr++;
      }
      ptr += SIDE2 - SIDE;
    }
    ptr += SIDE4 - SIDE3;
  }
}

/*
 * Initial pattern for empty cells.
 */
static void initGrid(Cell *grid)
{
  Cell *ptr = grid;
  for (int i = 0; i < SIDE6; i++, ptr++)
  {
    ptr->ch  = 0;      // charges
    ptr->n   = 0;      // tick
    ptr->a1  = 0;      // affinity
    ptr->a2  = 0;      // affinity
    ptr->syn = 0;      // spherical timing
    ptr->u   = 0;      // sine
    ptr->k   = NOROLE; // no role whatsoever
    ptr->obj = SIDE3;  // no target
    ptr->occ = 0;      // free cell
    ptr->off = i;      // offset
    SAT(ptr->o);       // invalid value
    SAT(ptr->po);      // unreachable
    SAT(ptr->m);       // messenger
    RSET(ptr->p);      // null momentum
    RSET(ptr->s);      // trivial spin
    RSET(ptr->pP);     // no empodion
  }
}

void explore(short org[3], int level)
{
	boolean shell = true;
	for (int dir = 0; dir < 6; dir++)
	{
		short o[3];
		CP(o, org);
	    o[dir>>1] += (dir % 2 == 0) ? +1 : -1;
        if(abs(o[dir >> 1]) < abs(org[dir >> 1]))
            continue;
		if(isAllowed(dir, o))
		{
			shell = false;
			explore(o, level+1);
		}
	}
	if(shell)
	{
        points[indx][0] = org[0];
        points[indx][1] = org[1];
        points[indx][2] = org[2];
        indx++;
	}
}

void initMomentum(Cell *grid)
{
  indx = 0;
  short org[3] = { 0, 0, 0 };
  explore(org, 0);
  //
  Cell *ptr = grid;
  int i = 0;
  for (int z = 0; z < SIDE; z++)
  {
    for (int y = 0; y < SIDE; y++)
    {
      for (int x = 0; x < SIDE; x++)
      {
        CP(ptr->p, points[i % indx]);
        i++; ptr++;
      }
      ptr += SIDE2 - SIDE;
    }
    ptr += SIDE4 - SIDE3;
  }
}

boolean sanity(Cell *grid)
{
  int n = 0;
  Cell *ptr = grid;
  for (int i = 0; i < SIDE6; i++)
  {
    if(!ZERO(ptr->s))
      n++;
    ptr++;
  }
  return n == SIDE3;
}

/**
 * Hub for initialization routines.
 */
void initSimulation()
{
  printf("SIDE=%d DIAG=%d\n", SIDE, DIAG);
  latt0 = (Cell*)malloc(SIDE6 * sizeof(Cell));
  assert(latt0 != NULL);
  latt1 = (Cell*)malloc(SIDE6 * sizeof(Cell));
  assert(latt1 != NULL);
  initGrid(latt0);
  initGrid(latt1);
  singularity(latt0);
  singularity(latt1);
  initMomentum(latt0);
  initMomentum(latt1);
  initSpin(latt0);
  initSpin(latt1);
  assert(sanity(latt0));
  assert(sanity(latt1));
}

/**
 * Tests if the two given vectors are antialigned.
 */
bool antialigned(short* a, short* b)
{
  int dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
  int mag_a = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];
  int mag_b = b[0]*b[0] + b[1]*b[1] + b[2]*b[2];
  if (dot*dot == mag_a*mag_b)
    return dot < 0;
  else
    return false;
}

/**
 * Tests if the two given vectors are aligned.
 */
bool aligned(short* a, short* b)
{
  for (int i = 0; i < 3; i++)
  {
    if ((a[i] == 0 && b[i] != 0) || (a[i] != 0 && b[i] == 0))
      return false;
    if (a[i] * b[(i + 1) % 3] != a[(i + 1) % 3] * b[i])
      return false;
    if (a[i] * b[i] < 0)
      return false;
  }
  return true;
}

}
