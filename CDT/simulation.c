/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include "simulation.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
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

int i0=0;	// msgr x bub
int i1=0;	// f=1 x f=1
int i2=0;	// strong
int i3=0;	// weak
int i4=0;	// electric
int i5=0;	// inertia

//Tuple cm1, cm2;		// debug
extern Cell *stb, *drf;
extern Cell *latt0, *latt1;

extern pthread_barrier_t barrier;

/*
 * Executes one cycle of the evolution algorithm.
 */
void simulation()
{
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    {
    	if(stb->a == 0)	// DEBUG
    		copy();
    }

#define CP
#ifdef CP

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	if(stb->a == 0)	// DEBUG
    		flash();

#endif

#define EXPAND
#ifdef EXPAND

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    {
    	if(stb->a == 0)	// DEBUG
    		expand();
    }
    /* DEBUG
    Cell *sing = isSingular(latt1);
    if(sing != NULL)
    	printf("SING %d,%d,%d\n", sing->pos[0], sing->pos[1], sing->pos[2]);
    */


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

    //delay(250);
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
		}
		else
		{
	        usleep(80000);
		}
	}
	return NULL;
}
