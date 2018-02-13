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
#include "common.h"
#include "params.h"
#include "tile.h"
#include "utils.h"
#include "automaton.h"
#include "scenarios.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Initializes sine wave parameters.
 * Axiom: sine
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
void buildLattice(Tile *grid)
{
	Tile *t = grid;
	for(int x = 0; x < SIDE; x++)
		for(int y = 0; y < SIDE; y++)
			for(int z = 0; z < SIDE; z++)
				for(int w = 0; w < NPREONS; w++, t++)
				{
					cleanTile(t);
					//
					// Once and for all
					//
					t->p0.x = x;
					t->p0.y = y;
					t->p0.z = z;
					t->p16 = w;
				}
}

/*
 * Inserts a preon in a specified address of the pri0 lattice.
 *
 * @p3	electric charge
 * @p4	chirality
 * @p5	color
 * @p6	spin
 * @p7  gravity
 * @p19	messenger
 */
void addPreon(int x, int y, int z, int w, char p3, char p4, unsigned char p5, Tuple p6, int p7, int p17, int p19, unsigned schedule)
{
	Tile *t = pri0 + (SIDE2 * x + SIDE * y + z) * NPREONS + w;
	cleanTile(t);
	t->p3 = p3;
	t->p4 = p4;
	t->p5 = p5;
	t->p6 = p6;
	t->p7 = p7;
	t->p14.x = -1;
	t->p17 = p17;
	t->p19 = p19;
	t->p23 = schedule;
	printf("%2d,%2d,%2d,%2d\n", x, y, z, w);
}

void createVacuum()
{
	for(int i = 0, w = 0; i < NPREONS; i += 2, w += 2)
	{
		int schedule = rand() % SYNCH + SYNCH;
		unsigned x = rndCoord();
		unsigned y = rndCoord();
		unsigned z = rndCoord();
		Tuple p6;
		resetTuple(&p6);
		addPreon(x,y,z,w, UNDEF, UNDEF, UNDEF, p6, false, PREON, false, schedule);
		addPreon(x,y,z,w+1, UNDEF, UNDEF, UNDEF, p6, false, PREON, false, schedule);
	}
}

/*
 * Initializes the automaton program
 */
void init()
{
	printf("Running...\n");
	//
	pri0  = malloc(SIDE4 * sizeof(Tile));
	dual0 = malloc(SIDE4 * sizeof(Tile));
	srand(time(NULL));
	initSineWave();
	buildLattice(pri0);
	buildLattice(dual0);
	limit = floor(sqrt(3) * (1 << (ORDER - 1)));
	//
	// Triple buffering
	//
	draft = imgbuf[0];
	clean = imgbuf[1];
	snap  = imgbuf[2];
	//
	// Initial state of the universe
	//
	int scenario = 1;
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
	}
	//
	begin = GetTickCount();				// initial milliseconds
	setvbuf(stdout, null, _IOLBF, 0);
	sleep(4);
}

