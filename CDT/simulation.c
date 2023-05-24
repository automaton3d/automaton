#include "simulation.h"

extern boolean rebuild;
extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;
extern unsigned long timer;
extern boolean stop;
extern char voxels[SIDE6];

Cell *latt0, *latt1;
Cell *stb, *drf;

Cell *neighbor(Cell *ptr, int dir)
{
  int off = ptr - latt1;
  int m = off % SIDE3;
  int x0 = (off / SIDE3) % SIDE;
  int y0 = (off / SIDE4) % SIDE;
  int z0 = off / SIDE5;
  switch (dir)
  {
    case 0:
      x0 = (x0 + 1) % SIDE;
      break;
    case 1:
      x0 = (x0 - 1 + SIDE) % SIDE;
      break;
    case 2:
      y0 = (y0 + 1) % SIDE;
      break;
    case 3:
      y0 = (y0 - 1 + SIDE) % SIDE;
      break;
    case 4:
      z0 = (z0 + 1) % SIDE;
      break;
    case 5:
      z0 = (z0 - 1 + SIDE) % SIDE;
      break;
  }
  return latt1 + x0 * SIDE3 + y0 * SIDE4 + z0 * SIDE5 + m;
}

void simulation()
{
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE6; i++, stb++, drf++)
    	phase1();
    stb = latt0;
    drf = latt1;
    for(int i = 0; i < SIDE6; i++, stb++, drf++)
    	phase4();

    /////////////////////
//    modified = true;

    // Update video buffer.

    Cell *stb = latt0;
    for(int i = 0; i < SIDE6; i++, stb++)
    {
        if(!ZERO(stb->p))
        	voxels[i] = RED;
        else if(!ZERO(stb->s))
        	voxels[i] = GREEN;
       	else
       		voxels[i] = BLK;
    }
//    modified = false;
//    ready = true;
}



/*
 * The automaton thread.
 */
void *SimulationLoop()
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
