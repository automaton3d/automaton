/*
 * init.c
 *
 *  Created on: 3 de abr de 2017
 *      Author: Alexandre
 */

#include "init.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "common.h"
#include "params.h"
#include "utils.h"
#include "automaton.h"
#include "brick.h"
#include "plot3d.h"
#include "gadget.h"


void swap(unsigned char *a, unsigned char *b)
{
	unsigned char t = *a;
	*a = *b;
	*b = t;
}

/*
 * Calculates the next permutation of the von Neumann directions out of the 720 possible.
 */
void next(unsigned char dirs[])
{
	static unsigned char idx = 6;
	static unsigned char permutation[] = { 0, 1, 2, 3, 4, 5 };
	static unsigned char temp[6];
	//
	while(true)
	{
		if(idx == 6)
		{
			int i;
		    for(i = 0; i < 6; i++)
		        temp[i] = 0;
			idx = 0;
		}
		if(temp[idx] < idx)
		{
			if(idx % 2 == 0)
				swap(&permutation[0], &permutation[idx]);
			else
				swap(&permutation[temp[idx]], &permutation[idx]);
			temp[idx]++;
			idx = 0;
			//
			for(int j = 0; j < 6; j++)
				dirs[j] = permutation[j];
			return;
		}
		else
		{
			temp[idx] = 0;
			idx++;
		}
	}
}

/*
 * Builds an empty lattice.
 * Called twice. One for pri and one to dual grid
 */
void buildLattice(Brick *grid)
{
	Brick *t = grid;
	int x, y, z, w;
	for(x = 0; x < SIDE; x++)
		for(y = 0; y < SIDE; y++)
			for(z = 0; z < SIDE; z++)
				for(w = 0; w < NPREONS; w++, t++)
				{
					cleanBrick(t);
					//
					// Once and for all
					//
					t->a.x = x;
					t->a.y = y;
					t->a.z = z;
					t->w = w;
					//
					// Pick a randomized directions array
					//
					next(t->dirs);
				}
	//
	// Build the hologram
	//
	x = SIDE/4; y = 10; z = 0; w = 0;
	t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
	t->p.x = 1;
	//
	t->seed = t->a;
	/*

	x = 3*SIDE/4; y = 8; z = 3; w = 1;
	t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
	t->p.x = 2;
	//
	t->seed = t->a;

	x = SIDE/2; y = 2; z = 6; w = 2;
	t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
	t->p.x = 3;
	//
	t->seed = t->a;

	x = SIDE/4; y = 1; z = SIDE-1; w = 3;
	t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
	t->p.x = -1;
	//
	t->seed = t->a;

	x = SIDE/4; y = 4; z = SIDE-2; w = 4;
	t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
	t->p.x = 4;
	//
	t->seed = t->a;
	 */

	/*
	z = 0;
	for(x = 0; x < SIDE; x++)
		for(y = 0; y < SIDE; y++)
		{
			w = SIDE * x + y;
			t = pri0 + (SIDE2*x + SIDE*y + z)*NPREONS + w;
			int a = 2 * (x%2) - 1;
			int b = ((SIDE*y + x) / 2) % 3;
			resetTuple(&t->s);
			resetTuple(&t->o);
			t->e = 0;
			t->m = 0;
			//
			t->seed = t->a;		// NEW!
			//
			if(y == SIDE-1)
			{
				t->p.x = 0;
				t->p.y = 0;
				t->p.z = a;
				if(x < SIDE/2)
				{
					t->q = 0;
					t->w = 0;
					t->R = 0;
					t->G = 0;
					t->B = 0;
					t->g = 0;
					t->d = 0;
				}
				else
				{
					t->q = 1;
					t->w = 1;
					t->R = 1;
					t->G = 1;
					t->B = 1;
					t->g = 1;
					t->d = 1;
				}
			}
			else
			{
				switch(b)
				{
					case 0:
						t->p.x = a;
						t->p.y = 0;
						t->p.z = 0;
						break;
					case 1:
						t->p.x = 0;
						t->p.y = a;
						t->p.z = 0;
						break;
					case 2:
						t->p.x = 0;
						t->p.y = 0;
						t->p.z = a;
						break;
				}
				if(x % 2)
				{
					t->q = 0;
					t->w = 0;
					t->R = 0;
					t->G = 0;
					t->B = 0;
					t->g = 0;
					t->d = 0;
				}
				else
				{
					t->q = 1;
					t->w = 1;
					t->R = 1;
					t->G = 1;
					t->B = 1;
					t->g = 1;
					t->d = 1;
				}
			}

		}
		*/
}

/*
 * Initializes the automaton program.
 */
void initAutomaton()
{
	printf("Running...\n");
	//
	pri0  = malloc(SIDE4 * sizeof(Brick));
	dual0 = malloc(SIDE4 * sizeof(Brick));
	srand(time(NULL));
	buildLattice(pri0);
	buildLattice(dual0);
	limit = floor(sqrt(3) * (1 << (ORDER - 1)));
	period = floor(PI*SIDE/4);
	//
	// Initialize gadgets
	//
	ticks[0] = true;
	ticks[1] = true;
	//
	// Triple buffering
	//
	draft = imgbuf[0];
	clean = imgbuf[1];
	snap  = imgbuf[2];
	//
	begin = GetTickCount();				// initial milliseconds
	setvbuf(stdout, null, _IOLBF, 0);
	sleep(4);
}

