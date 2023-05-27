#include "simulation.h"

extern boolean rebuild;
extern unsigned long timer;
extern boolean stop;

Cell *latt0, *latt1;
Cell *stb, *drf;

boolean rebuild = true;

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

    // Update video buffer.

    updateBuffer();
}

/*
 * The automaton thread.
 */
void *SimulationLoop()
{
    pthread_detach(pthread_self());
	initSimulation();
	//
	while(true)
	{
		if(!stop)
		{
			simulation();
			rebuild = true;
			timer++;
		}
		else
		{
	        Sleep(80);
		}
	}
	return NULL;
}
