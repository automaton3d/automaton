/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include "simulation.h"
#include "test/test.h"

extern boolean rebuild;

boolean verbose = false;

extern Cell *stb, *drf;
extern Cell *latt0, *latt1;

extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;

extern unsigned long timer;

/*
 * Executes one cycle of the evolution algorithm.
 */
void simulation()
{
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE6; i++, stb++, drf++)
   		copy();

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE6; i++, stb++, drf++)
    	model(0);

    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE6; i++, stb++, drf++)
    	model(1);

    //delay(50);
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
		}
		else
		{
	        Sleep(80);
		}
	}
	return NULL;
}
