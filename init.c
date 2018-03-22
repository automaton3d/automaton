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
#include "scenarios.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int prime;

/*
 * Initializes sine wave parameters.
 * Axiom 3 - Preon phase
 */
void initSineWave()
{
	K = 2 * cos(wT);
	U1 = SIDE * sin(-2 * wT);
	U2 = SIDE * sin(-wT);
}

/*
 * Builds an empty lattice.
 * Called twice. One for pri and one to dual grid
 */
void buildLattice(Brick *grid)
{
	Brick *t = grid;
	for(int x = 0; x < SIDE; x++)
		for(int y = 0; y < SIDE; y++)
			for(int z = 0; z < SIDE; z++)
				for(int w = 0; w < NPREONS; w++, t++)
				{
					cleanBrick(t);
					//
					// Once and for all
					//
					t->p0.x = x;
					t->p0.y = y;
					t->p0.z = z;
					t->p19 = w;
				}
}

/*
 * Inserts a preon in a specified address of the pri0 lattice.
 *
 * @p4	electric charge
 * @p5	chirality
 * @p6	color
 * @p7	spin
 * @p8  gravity
 * @p20 status
 * @p24	messenger
 */
void addPreon(int x, int y, int z, int w, char p4, char p5, unsigned char p6, Tuple p7, int p8, int p20, int p24, unsigned schedule)
{
	Brick *t = pri0 + (SIDE2 * x + SIDE * y + z) * NPREONS + w;
	cleanBrick(t);
	t->p6 = p4;
	t->p7 = p5;
	t->p9 = p6;
	t->p4 = p7;
	t->p8 = p8;
	t->p15.x = -1;
	t->p21 = p20;
	t->p25 = p24;
	t->p24t = schedule;
	printf("%2d,%2d,%2d,%2d: %+d, %+d\n", x, y, z, w, p4, p8);
}

void createVacuum()
{
	int p8 = +1;
	for(int w = 0; w < NPREONS; w += 2)
	{
		Tuple p7;
		resetTuple(&p7);
		addPreon(0,0,0,w, +1, UNDEF, UNDEF, p7, p8, PREON, false, BURST);
		addPreon(0,0,0,w+1, -1, UNDEF, UNDEF, p7, p8, PREON, false, BURST);
		p8 *= -1;
	}
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
	initSineWave();
	buildLattice(pri0);
	buildLattice(dual0);
	limit = floor(sqrt(3) * (1 << (ORDER - 1)));
	prime = getPrime(SIDE);
	//
	// Triple buffering
	//
	draft = imgbuf[0];
	clean = imgbuf[1];
	snap  = imgbuf[2];
	//
	// Initial state of the universe
	//
	scenario = 1;
	switch(scenario)
	{
		case 0:
			BurstScenario();
			break;
		case 1:
			VacuumScenario();
			break;
		case 2:
			VIRTScenario();
			break;
		case 3:
			REALScenario();
			break;
		case 4:
			UXUScenario();
			break;
		case 5:
			GRAVScenario();
			break;
		case 6:
			SEEDScenario();
			break;
		case 7:
			ElectronScenario();
			break;
		case 8:
			LonePScenario();
			break;
	}
	//
	begin = GetTickCount();				// initial milliseconds
	setvbuf(stdout, null, _IOLBF, 0);
	sleep(4);
}

