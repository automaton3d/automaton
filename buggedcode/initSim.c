/*
 * init.c
 *
 *  Created on: 23 de mai. de 2023
 *      Author: Alexandre
 */

#include <assert.h>
#include "simulation.h"

extern Cell *latt0, *latt1;

// Template for momentum recursion

int template[6][3] =
{
  {+SIDE_2, 0, 0},
  {0, +SIDE_2, 0},
  {0, 0, +SIDE_2},
  {-SIDE_2, 0, 0},
  {0, -SIDE_2, 0},
  {0, 0, -SIDE_2},
};

/*
 * Initial state.
 */
void singularity(Cell *grid)
{
  Cell *ptr = grid;

  // Create a sheet.

  int i = 0;
  for(int y = 0; y < SIDE; y++)
  {
    for(int x = 0; x < SIDE; x++, i++)
    {
      CP(ptr->s, template[i % 6]);
      ptr++;
    }
    ptr += SIDE2 - SIDE;
  }

  // Create bulk.

  Cell *ref;
  i = 0;
  ptr = grid;
  for(int z = 0; z < SIDE; z++)
  {
	ref = ptr;
    for(int y = 0; y < SIDE; y++)
    {
      for(int x = 0; x < SIDE; x++)
      {
        CP(ptr->p, template[i % 6]);   // momentum
        CP(ptr->s,(grid + (ptr - ref))->s); // spin
        RSET(ptr->o);	               // ready for immediate expansion
        i++;
        ptr++;
      }
      ptr += SIDE2 - SIDE;
    }
    ptr += SIDE4 - SIDE3;
  }
}

void initGrid(Cell *grid)
{
  Cell *ptr = grid;
  for(int i = 0; i < SIDE6; i++)
  {
    ptr->n   = 0;     // tick
    ptr->syn = 0;     // spherical timing
	ptr->off = i;     // offset
    SAT(ptr->o);      // invalid value
    RSET(ptr->p);     // null momentum
    RSET(ptr->s);     // trivial spin
    ptr++;
  }
}

#define ANY 10

void initSimulation()
{
  latt0 = malloc(SIDE6 * sizeof(Cell));
  latt1 = malloc(SIDE6 * sizeof(Cell));
  initGrid(latt0);
  initGrid(latt1);
  singularity(latt0 + ANY * SIDE3);
  singularity(latt1 + ANY * SIDE3);
}
