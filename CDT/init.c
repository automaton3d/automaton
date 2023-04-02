/*
 * init.c
 *
 *  Created on: 3 de abr de 2017
 *      Author: Alexandre
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "common.h"
#include "utils.h"
#include "plot3d.h"
#include "gadget.h"
#include "simulation.h"
#include "mouse.h"
#include "assert.h"
#include "engine.h"
#include "quaternion.h"
#include "trackball.h"

#define PAIR

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
	//
	imgbuf[0] = malloc(IMGBUFFER*sizeof(Voxel));
	imgbuf[1] = malloc(IMGBUFFER*sizeof(Voxel));
	buff = imgbuf[0];
	clean = imgbuf[1];
	buff->color = -1;
	clean->color = -1;
	//
	// Initialize gadgets
	//
	for(int i = 0; i < NTICKS; i++)
		ticks[i] = true;
	//
	ticks[PLANE] 	  = false;
	ticks[MESSENGER]  = false;
	ticks[CUBE] 	  = false;
	ticks[TRAJECTORY] = false;
	ticks[SPIN] 	  = false;
	ticks[MOMENTUM]	  = false;

	//
	begin = GetTickCount();				// initial milliseconds
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
	//
	sleep(4);
}

// Template for momentum recursion

#define S (SIDE/2)

int template[6][3] =
{
  {+S, 0, 0},
  {0, +S, 0},
  {0, 0, +S},
  {-S, 0, 0},
  {0, -S, 0},
  {0, 0, -S},
};

void singularity(Cell *grid)
{
  Cell *pointer;

  pointer = grid;
  int i = 0;

  // Create a sheet

  for(int y = 0; y < SIDE; y++)
    for(int x = 0; x < SIDE; x++)
    {
      CP(pointer->s, template[i % 6]);
      pointer++; i++;
    }
  i = 0;
  pointer = grid;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++)
      {
        pointer->ch = 0;

        // dualitat

        pointer->ch |= ((i >> 1) << 5);

        // weak

        pointer->ch |= ((i % 2) << 4);

        // electrical

        pointer->ch |= ((((i % 2)^(i >> 1))  & 1) << 3);

        // color

        pointer->ch |= (i % 8);

        // momentum

        CP(pointer->p, template[i % 6]);

        // spin

        CP(pointer->s, grid[i % SIDE2].s);

        // Other properties

        pointer->t     = 0;
        pointer->f     = 1;
        pointer->bmp   = 1;
        pointer->a     = i;
        pointer->seed  = i;
        pointer->kind  = 0;
        RESET(pointer->o);
        RESET(pointer->pole);
        RESET(pointer->dom);

        // Next

        pointer++; i++;
      }
}

void initEspacito(Cell *lattice, Cell *espacito, int x0, int y0, int z0)
{
  Cell *pointer = espacito;
  unsigned offset = 0;
  for(int z = 0; z < SIDE; z++)
    for(int y = 0; y < SIDE; y++)
      for(int x = 0; x < SIDE; x++)
      {
        pointer->ch    = 0;
        pointer->t     = 0;
        pointer->f     = 0;
        pointer->bmp   = 0;
        pointer->a     = offset;
        pointer->syn   = 0;
        pointer->u     = 0;
        pointer->pmf   = 0;

        // Eq. 4

        pointer->pow   = 1;
        pointer->den   = 1;

        pointer->seed  = offset;
        pointer->kind  = 0;
        pointer->flash  = false;
        pointer->target = SIDE3;
        RESET(pointer->o);
        RESET(pointer->pole);
        RESET(pointer->p);
        RESET(pointer->s);
        RESET(pointer->pP);
        RESET(pointer->msg);

        // DEBUG
#define DEBUG
#ifdef DEBUG
        pointer->pos[0] = x0;
        pointer->pos[1] = y0;
        pointer->pos[2] = z0;
#endif
        // Wires

        if(x0 == SIDE - 1)
        	pointer->wires[0] = lattice + OFFSET(0, y0, z0) + offset;
        else
        	pointer->wires[0] = lattice + OFFSET(x0 + 1, y0, z0) + offset;
        if(x0 == 0)
        	pointer->wires[1] = lattice + OFFSET(SIDE - 1, y0, z0) + offset;
        else
        	pointer->wires[1] = lattice + OFFSET(x0 - 1, y0, z0) + offset;

        if(y0 == SIDE - 1)
        	pointer->wires[2] = lattice + OFFSET(x0, 0, z0) + offset;
        else
        	pointer->wires[2] = lattice + OFFSET(x0, y0 + 1, z0) + offset;
        if(y0 == 0)
        	pointer->wires[3] = lattice + OFFSET(x0, SIDE - 1, z0) + offset;
        else
        	pointer->wires[3] = lattice + OFFSET(x0, y0 - 1, z0) + offset;

        if(z0 == SIDE - 1)
        	pointer->wires[4] = lattice + OFFSET(x0, y0, 0) + offset;
        else
        	pointer->wires[4] = lattice + OFFSET(x0, y0, z0 + 1) + offset;
        if(z0 == 0)
        	pointer->wires[5] = lattice + OFFSET(x0, y0, SIDE -1) + offset;
        else
        	pointer->wires[5] = lattice + OFFSET(x0, y0, z0 - 1) + offset;

        // Offset inside espacito

        pointer->offset = offset;

        // Next

        pointer++;
        offset++;
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
