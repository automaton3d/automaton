/*
 * init.c
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
					memset(t, 0, sizeof(Brick));
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
 * @p3  momentum
 * @p4	spin
 * @p5	helicity
 * @p6	electric charge
 * @p7	chirality
 * @p8  gravity
 * @p9	color
 * @p21 status
 * @p24 schedule
 * @p25	messenger
 */
Brick *addPreon(Tuple p0, int w, Tuple p3, Tuple p4, char p5, char p6, char p7, int p8, unsigned char p9, int p21, unsigned p24, int p25)
{
	Brick *t = pri0 + (SIDE2 * p0.x + SIDE * p0.y + p0.z) * NPREONS + w;
	t->p3 = p3;
	t->p4 = p4;
	t->p5 = p5;
	t->p6 = p6;
	t->p7 = p7;
	t->p8 = p8;
	t->p9 = p9;
	t->p15.x = -1;
	t->p21 = p21 | GRAV;
	t->p24 = p24;
	t->p25 = p25;
	printf("%2d %2d %2d %2d: %+d %+d %+d %+d %02xH\t%s\t%s\n", p0.x, p0.y, p0.z, w, p5, p6, p7, p8, p9, tuple2str(&p3), tuple2str(&p4));
	return t;
}

/*
 * Initializes the automaton program.
 */
void initAutomaton()
{
	printf("Scenario: %s\n", sceneNames[scene]);
	printf("x  y   z  w  p5 p6 p7 p8 p9\tp3\tp4\n");
	//
	pri0  = malloc(SIDE4 * sizeof(Brick));
	dual0 = malloc(SIDE4 * sizeof(Brick));
	//
	srand(time(NULL));
	initSineWave();
	buildLattice(pri0);
	buildLattice(dual0);
	//
	// Initialize constants
	//
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
	(*scenarios[scene])();
	//
	begin = GetTickCount();				// initial milliseconds
	setvbuf(stdout, null, _IOLBF, 0);
	sleep(3);
}

