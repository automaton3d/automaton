/*
 * init.c
 *
 *  Created on: 23 de mai. de 2023
 *      Author: Alexandre
 */

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
    for(int x = 0; x < SIDE; x++, ptr++, i++)
      CP(ptr->s, template[i % 6]);

  // Create bulk.

  i = 0;
  ptr = grid;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++)
      {
    	char w0 = i % 2;
    	char w1 = (i >> 1) % 2;
    	char q  = w0 ^ w1;
        ptr->ch = (i % 8) | (w0 << 3) | (1 << 4) | (q << 5);
        ptr->a1  = i + 1;	           // dodges zero
        ptr->a2  = 0;                  // no chaining
        ptr->k   = FERMION;            // intially just singles
        ptr->occ = SIDE_2 - 1;         // crest cell
        CP(ptr->p, template[i % 6]);   // momentum
        CP(ptr->s, grid[i % SIDE2].s); // spin
        RSET(ptr->o);	               // ready for immediate expansion
        RSET(ptr->po);                 // already at the reissue cell
        ptr++;
        i++;
      }
}

void initEspacito(Cell *latt, Cell *espacito)
{
  Cell *ptr = espacito;
  for(int i = 0; i < SIDE3; i++)
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
    SAT(ptr->o);      // invalid value
    SAT(ptr->po);     // unreachable
    RSET(ptr->p);     // null momentum
    RSET(ptr->s);     // trivial spin
    RSET(ptr->pP);    // no empodion
    RSET(ptr->m);     // messenger
    ptr++;
  }
}

void initSimulation()
{
  printf("DIAG=%d DIAG=%d SIDE=%d\n", DIAG, (int)(sqrt(3)*SIDE + 0.5), SIDE);

  latt0 = malloc(SIDE6 * sizeof(Cell));
  latt1 = malloc(SIDE6 * sizeof(Cell));

  Cell *stb = latt0;
  Cell *drf = latt1;

  for(int i = 0; i < SIDE3; i++, stb += SIDE3, drf += SIDE3)
  {
    initEspacito(latt0, stb);
    initEspacito(latt1, drf);
  }
  stb = latt0;
  drf = latt1;
  for(int i = 0; i < SIDE6; i++, stb++, drf++)
  {
	  stb->off = i;
	  drf->off = i;
  }
  singularity(latt0);
  singularity(latt1);
}
