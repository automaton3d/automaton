#include "simulation.h"

extern unsigned long timer;
extern boolean stop;

Cell *latt0, *latt1;
Cell *stb, *drf;

Cell *neighbor(Cell *ptr, int dir)
{
	int i = ptr - latt1;
    int x = i % SIDE2;
    int y = (i / SIDE2) % SIDE2;
    int z = (i / SIDE4) % SIDE2;
    switch(dir)
    {
        case 0:
            return latt1 + ((x + SIDE) % SIDE2) + SIDE2*y + SIDE4*z;
        case 1:
            return latt1 + ((x - SIDE + SIDE2) % SIDE2) + SIDE2*y + SIDE4*z;
        case 2:
            return latt1 + x + SIDE2*((y + SIDE) % SIDE2) + SIDE4*z;
        case 3:
            return latt1 + x + SIDE2*((y - SIDE + SIDE2) % SIDE2) + SIDE4*z;
        case 4:
            return latt1 + x + SIDE2*y + SIDE4*((z + SIDE) % SIDE2);
        case 5:
            return latt1 + x + SIDE2*y + SIDE4*((z - SIDE + SIDE2) % SIDE2);
    }
    return NULL;
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
}

DWORD WINAPI SimulateThread(LPVOID lpParam)
{
	initSimulation();
    while (true)
    {
		if(!stop)
		{
			simulation();
			updateBuffer();
			timer++;
		}
		else
		{
	        Sleep(80);
		}
    }
    return 0;
}
