/*
 * init.c
 *
 *  Created on: 3 de abr de 2017
 *      Author: Alexandre
 */

#include <stdio.h>
#include <math.h>
#include "plot3d.h"
#include "gadget.h"
#include "simulation.h"
#include "mouse.h"
#include "assert.h"
#include "engine.h"
#include "quaternion.h"
#include "trackball.h"

extern boolean ticks[NTICKS];
extern Voxel *imgbuf[2], *buff, *clean;
extern Cell *latt0, *latt1;
extern Quaternion currQ, lastQ;

extern View view;

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

  // Initialize gadgets

  for(int i = 0; i < NTICKS; i++)
    ticks[i] = true;

  //ticks[FRONT]	  = false;
  ticks[MESSENGER]  = false;
  //ticks[TRACK] 	  = false;
  ticks[MODE0] 	  = false;
  ticks[MODE2] 	  = false;
  ticks[PLANE] 	  = false;
  ticks[CUBE] 	  = false;

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

void init()
{
  int oE = 0;
  Cell *stb = latt0;
  Cell *drf = latt1;
  for(int i = 0; i < SIDE6; i++, stb++, drf++)
  {
    int n = i / SIDE3;
    int x0 = n % SIDE;
    int y0 = (n / SIDE) % SIDE;
    int z0 = n / SIDE2;
    stb->occ = 0;
    drf->occ = 0;
    RSET(stb->p);
    RSET(drf->p);
    if(i < SIDE3)
    {
      stb->p[0] = 1;
      drf->p[0] = 1;
    }
    wires(stb, x0, y0, z0, latt0, oE);
    wires(drf, x0, y0, z0, latt1, oE);
  }
}

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
        ptr->a  = i + 1;	// dodges zero
        ptr->k  = FERMION;
        RSET(ptr->o);
        MILD(ptr->po);
      }
}

void initEspacito(Cell *latt, Cell *espacito, int x0, int y0, int z0)
{
  Cell *ptr = espacito;
  unsigned oE = 0;
  unsigned oL = x0 + SIDE * y0 + SIDE2 * z0;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++, ptr++, oE++)
      {
        ptr->ch    = 0;   // charges
        ptr->n     = 0;   // tick
        ptr->a     = 0;	  // affinity
        ptr->syn   = 0;
        ptr->u     = 0;
        ptr->pmf   = 0;
        ptr->pow   = 1;   // Eq.4
        ptr->den   = 1;   // Eq. 4
        ptr->k   = EMPTY; // no role whatsoever
        ptr->f   = 0;	  // flash inactive
        ptr->obj = SIDE3; // no target
        ptr->occ = 0;     // free to receive propag.
        ptr->oL  = oL;    // offset in the grid
        ptr->oE  = oE;    // offset inside espacito
        ptr->r   = 0;     // lcg algorithm
        SAT(ptr->o);      // invalid value
        SAT(ptr->po);     // unreachable
        RSET(ptr->p);     // null momentum
        RSET(ptr->s);     // trivial spin
        RSET(ptr->pP);    // no empodion
        RSET(ptr->m);     // messenger
    	wires(ptr, x0, y0, z0, latt, oE);      // wires
        pthread_mutex_init(&ptr->mutex, NULL);
      }
}

void initAutomaton()
{
  printf("DIAG=%d DIAG=%d SIDE=%d\n", DIAG, (int)(sqrt(3)*SIDE + 0.5), SIDE);

  latt0 = malloc(SIDE6 * sizeof(Cell));
  assert(latt0 != NULL);
  latt1 = malloc(SIDE6 * sizeof(Cell));
  assert(latt1 != NULL);

  Cell *espacito;

  // Create draft lattice

  Cell *sing = NULL;
  espacito = latt1;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++)
      {
    	  if(x == SIDE/2 && y == SIDE/2 && z == SIDE/2)
    		  sing = espacito;
    	  initEspacito(latt1, espacito, x, y, z);
    	  espacito += SIDE3;
      }

  // Create stable lattice

  espacito = latt0;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++)
      {
    	  initEspacito(latt0, espacito, x, y, z);
    	  espacito += SIDE3;
      }

  // Create singularity

  singularity(sing);
}

