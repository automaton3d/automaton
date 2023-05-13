/*
 * init.c
 *
 *  Created on: 3 de abr de 2017
 *      Author: Alexandre
 */

#include "simulation.h"

extern Voxel *imgbuf[2], *buff, *clean;
extern Cell *latt0, *latt1;
extern Quaternion currQ, lastQ;
extern View view;
extern unsigned long begin;

View *vu;

long OFFSET(int x, int y, int z)
{
  return (x*SIDE3+y*SIDE4+z*SIDE5);
}

void initScreen()
{
  // Alocate memory for voxel buffers

  imgbuf[0] = malloc(IMGBUFFER*sizeof(Voxel));
  imgbuf[1] = malloc(IMGBUFFER*sizeof(Voxel));
  buff = imgbuf[0];
  clean = imgbuf[1];
  buff->color = -1;
  clean->color = -1;

  begin = GetTickCount64();      // initial milliseconds
  setvbuf(stdout, 0, _IOLBF, 0);

  // Initialize trackball

  identityQ(&lastQ);
  identityQ(&currQ);

  // TRACKBALL

  vu = &view;
  vu->width = WIDTH;
  vu->height = HEIGHT;
  trackball (vu->quat , 0.0, 0.0, 0.0, 0.0);
  vset(vu->rot_axis, 0., 0., 1.);

  Sleep(4);
}

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
  int i = 0;

  // Create a sheet

  for(int y = 0; y < SIDE; y++)
    for(int x = 0; x < SIDE; x++, ptr++, i++)
      CP(ptr->s, template[i % 6]);
  i = 0;
  ptr = grid;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++, ptr++, i++)
      {
        ptr->ch = 0;
        ptr->ch |= ((i >> 1) << 5);  // dualitat
        ptr->ch |= ((i % 2) << 4);   // weak
        ptr->ch |= ((((i % 2)^(i >> 1))  & 1) << 3); // electrical
        ptr->ch |= (i % 8);  // color
        CP(ptr->p, template[i % 6]);  // momentum
        CP(ptr->s, grid[i % SIDE2].s);  // spin
        ptr->a1  = i + 1;	// dodges zero
        ptr->a2  = 0;
        ptr->k   = FERMION;
        RSET(ptr->o);	// ready for immediate expansion
        RSET(ptr->po);  // already at the reissue cell
      }
}

void initEspacito(Cell *latt, Cell *espacito)
{
  Cell *ptr = espacito;
  for(int i = 0; i < SIDE3; i++)
  {
    ptr->ch    = 0;   // charges
    ptr->n     = 0;   // tick
    ptr->a1    = 0;	  // affinity
    ptr->a2    = 0;	  // affinity
    ptr->syn   = 0;   // spherical timing
    ptr->u     = 0;   // sine
    ptr->k   = EMPTY; // no role whatsoever
    ptr->obj = SIDE3; // no target
    ptr->dir = 0;
    SAT(ptr->o);      // invalid value
    SAT(ptr->po);     // unreachable
    RSET(ptr->p);     // null momentum
    RSET(ptr->s);     // trivial spin
    RSET(ptr->pP);    // no empodion
    RSET(ptr->m);     // messenger
    pthread_mutex_init(&ptr->mutex, NULL);
  }
}

void initAutomaton()
{
  printf("DIAG=%d DIAG=%d SIDE=%d\n", DIAG, (int)(sqrt(3)*SIDE + 0.5), SIDE);

  latt0 = malloc(SIDE6 * sizeof(Cell));
  latt1 = malloc(SIDE6 * sizeof(Cell));

  // Create the lattice graph.

  Cell *stb = latt0;
  Cell *drf = latt1;
  for(int i = 0; i < SIDE6; i++, stb++, drf++)
  {
    for (int dir = 0; dir < NDIR; dir++)
      stb->ws[dir] = wires(i, stb, dir, latt0);
    for (int dir = 0; dir < NDIR; dir++)
      drf->ws[dir] = wires(i, drf, dir, latt1);
  }

  // Introduce data.

  stb = latt0;
  drf = latt1;
  for(int i = 0; i < SIDE6; i += SIDE3, stb += SIDE3, drf += SIDE3)
  {
    initEspacito(latt0, stb);
    initEspacito(latt1, drf);
  }
  singularity(latt0);
  singularity(latt1);
}

