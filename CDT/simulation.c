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
#include "vector3d.h"
#include "utils.h"
#include "plot3d.h"
#include "text.h"
#include "main3d.h"
#include "engine.h"

extern boolean rebuild;

boolean verbose = false;
boolean stop = false;
Tuple center;

struct Space *space0 = NULL;

int i0=0;	// msgr x bub
int i1=0;	// f=1 x f=1
int i2=0;	// strong
int i3=0;	// weak
int i4=0;	// electric
int i5=0;	// inertia

//Tuple cm1, cm2;		// debug
extern Tensor *stb, *drf;
extern Tensor *lattice0, *lattice1;

extern pthread_barrier_t barrier;

/*
 * Returns the point on sphere in cen.
 */
Tuple getPole(Vector3d v, Tuple cen, int r)
{
	double a = v.x*v.x + v.y*v.y + v.z*v.z;
	double c = -r*r;
	double t = sqrt(-4*a*c)/(2*a);
	Tuple result;
	result.x = (int)(cen.x + v.x * t);
	result.y = (int)(cen.y + v.y * t);
	result.z = (int)(cen.z + v.z * t);
	return result;
}

/*
 * Executes one cycle of the evolution algorithm.
 */
void simulation()
{
    stb = lattice0;
    drf = lattice1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	copy();

//#define CP
#ifdef CP

    stb = lattice0;
    drf = lattice1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	flash();

#endif

#define EXPAND
#ifdef EXPAND

    stb = lattice0;
    drf = lattice1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	expand();

#endif

//#define UPDATE
#ifdef UPDATE

    stb = lattice0;
    drf = lattice1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stb++, drf++)
    	update();

#endif

#ifdef INTERACT
    stbl = lattice0;
    draft = lattice1;
    for(int i = 0; i < SIDE3 * SIDE3; i++, stbl++, draft++)
    	interact();
#endif

   //delay(150);
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
			simulation();
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
