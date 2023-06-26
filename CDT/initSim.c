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
    	char w0  = i % 2;
    	char w1  = (i >> 1) % 2;
    	char q   = w0 ^ w1;
        ptr->ch  = (i % 8) | (w0 << 3) | (1 << 4) | (q << 5);
        ptr->a1  = i + 1;	           // dodges zero
        ptr->a2  = 0;                  // no chaining
        ptr->k   = FERMION;            // intially just singles
        ptr->occ = SIDE_2 - 1;         // crest cell
        CP(ptr->p, template[i % 6]);   // momentum
        CP(ptr->s,(grid + (ptr - ref))->s); // spin
        RSET(ptr->o);	               // ready for immediate expansion
        RSET(ptr->po);                 // already at the reissue cell
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
    ptr->ch  = 0;     // charges
    ptr->n   = 0;     // tick
    ptr->a1  = 0;     // affinity
    ptr->a2  = 0;     // affinity
    ptr->syn = 0;     // spherical timing
    ptr->u   = 0;     // sine
    ptr->k   = NONE;  // no role whatsoever
    ptr->obj = SIDE3; // no target
    ptr->occ = 0;     // free cell
    ptr->off = i;     // offset
    SAT(ptr->o);      // invalid value
    SAT(ptr->po);     // unreachable
    SAT(ptr->m);      // messenger
    RSET(ptr->p);     // null momentum
    RSET(ptr->s);     // trivial spin
    RSET(ptr->pP);    // no empodion
    assert(ptr==SKEW(ptr,grid));
    ptr++;
  }
}

void initSimulation()
{
  printf("SIDE=%d DIAG=%d\n", SIDE, DIAG);

  latt0 = malloc(SIDE6 * sizeof(Cell));
  assert(latt0 != NULL);
  latt1 = malloc(SIDE6 * sizeof(Cell));
  assert(latt1 != NULL);
  initGrid(latt0);
  initGrid(latt1);
  singularity(latt0);
  singularity(latt1);
  assert(sanity(latt0));
  assert(sanity(latt1));
}
