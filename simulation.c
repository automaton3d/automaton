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

int a = 0;

/*
 * Executes one cycle of the evolution algorithm.
 */
void simulation()
{
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
   		copy();

//	#define FLASH
	#ifdef FLASH

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
 		flash();

    //if(rand() % 40 == 0)
    	//latt1->f = true;

    #endif

	#define EXPAND
	#ifdef EXPAND

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
   		expand();
    int n = countMomentum(latt1);
    if(n == 0)
    	printf(">>>n=%ld: np=%d\n", timer, n);

    #endif

	//#define UPDATE
	#ifdef UPDATE

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	update();

	#endif

	//#define INTERACT
	#ifdef INTERACT

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	interact();

	#endif

    delay(100);
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
			#ifndef TEST_TREE
			simulation();
			#endif
	    	pthread_mutex_lock(&mutex);
			rebuild = true;
		    pthread_mutex_unlock(&mutex);
			timer++;
			if(timer % 256)
				a = rand() % SIDE3;
		}
		else
		{
	        Sleep(80);
		}
	}
	return NULL;
}
