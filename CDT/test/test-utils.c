/*
 * test-utils.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */
#include "test.h"
#include "../simulation.h"

extern Cell *latt1;

int countMomentum(Cell *latt)
{
	int n = 0;
    for(int i = 0; i < SIDE6; i++, latt++)
    {
    	if(!ZERO(latt->p))
    		n++;
    }
    return n;
}

int countMomentum__(Cell *latt)
{
	int n = 0;
    for(int i = 0; i < SIDE6; i++, latt++)
    {
    	if(!ZERO(latt->p))
    	{
    		n++;
    		return 1;
    	}
    }
    return n;
}

boolean sanity(Cell *latt)
{
	int n = 0;
	for(int i = 0; i < SIDE6; i++, latt++)
	{
		if(GET_ROLE(latt) == EMPTY)
			n++;
	}
	return n == SIDE6 - SIDE3;
}

void printCoords(Cell *c)
{
    int x = c->off % SIDE2;
    int y = (c->off / SIDE2) % SIDE2;
    int z = (c->off / SIDE4) % SIDE2;
    printf("(%d,%d,%d)\n", x, y, z); fflush(stdout);
}

int main___()
{
	initSimulation();
	Cell *c0 = latt1 + SIDE_2 * (1 + SIDE2 + SIDE4);
	printCoords(c0);
	printf("c0: %d\n", c0->off); fflush(stdout);
	assert(isCentralPoint(c0->off));
	for (int i = 0; i < 100; i++)
	{
		int dir = rand() % 6;
		printf("\ndir=%d\n", dir);
		for(int i = 0; i < SIDE2; i++)
		{
			c0 = neighbor(c0, dir);
			printCoords(c0);
		}
		assert(isCentralPoint(c0->off));
	}
	printf("If ==(%d,%d,%d) then Ok.\n", SIDE_2, SIDE_2, SIDE_2);
	return 0;
}
