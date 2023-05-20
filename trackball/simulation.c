#include "simulation.h"

extern boolean rebuild;
extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;
extern unsigned long timer;
extern boolean stop;

void simulation() {}
void initSimulation() {}

/*
 * The automaton thread.
 */
void *AutomatonLoop()
{
    pthread_detach(pthread_self());
	initSimulation();
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
	        Sleep(80);
		}
	}
	return NULL;
}
