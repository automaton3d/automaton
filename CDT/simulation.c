/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include "simulation.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "common.h"
#include "tuple.h"
#include "utils.h"
#include "plot3d.h"
#include "text.h"
#include "main3d.h"
#include "engine.h"
#include "test/test.h"
#include "vec3.h"

extern boolean rebuild;

boolean verbose = false;

extern Cell *stb, *drf;
extern Cell *latt0, *latt1;

extern pthread_barrier_t barrier;

int off = 0;

/*
 * Executes one cycle of the evolution algorithm.
 */
void simulation()
{
	#define COPY
	#ifdef COPY
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
   		copy();

	#endif

	#define FLASHLOOP
	#ifdef FLASHLOOP

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
 		flash();

    #endif

	#define EXPAND
	#ifdef EXPAND

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
   		expand();

    #endif

	#define UPDATE
	#ifdef UPDATE

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	update();

	#endif

	#define INTERACT
	#ifdef INTERACT

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	interact();

	#endif

    // delay(100);
}

/*
 * The automaton thread.
 */
void *AutomatonLoop()
{
    pthread_detach(pthread_self());
	initAutomaton();
    pthread_barrier_wait(&barrier);
	//
    printf("n=%d\n", countMomentum(latt1));
	while(true)
	{
		if(!stop)
		{
			simulation();
	    	pthread_mutex_lock(&mutex);
			rebuild = true;
		    pthread_mutex_unlock(&mutex);
			timer++;

			// Debug

			if(timer % 256)
				off = rand() % SIDE3;
		}
		else
		{
	        Sleep(80);
		}
	}
	return NULL;
}
