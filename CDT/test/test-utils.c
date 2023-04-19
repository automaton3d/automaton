/*
 * test-utils.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */
#include "test.h"
#include "../simulation.h"
#include "../utils.h"

Cell *getAddress(Cell *latt, int x, int y, int z)
{
	return latt + OFFSET(x, y, z);
}

Cell *isSingular(Cell *latt)
{
	Cell *last;
	boolean first = true;
	int unique[3];
    for(int i = 0; i < SIDE3 * SIDE3; i++, latt++)
    {
    	if(latt->k == EMPTY && latt->k != EMPTY)
    	{
    		if(first)
    		{
    			CP(unique, latt->pos);
    			first = false;
    		}
    		if(!EQ(unique, latt->pos))
    			return false;
    		last = latt;
    	}
    }
    return last;
}

Cell *huntFlash(Cell *latt)
{
    for(int i = 0; i < SIDE3 * SIDE3; i++, latt++)
    {
    	if(latt->a == 0 && latt->f)
    		return latt;
    }
    return NULL;
}

int countMomentum(Cell *latt)
{
	int n = 0;
    for(int i = 0; i < SIDE3 * SIDE3; i++, latt++)
    {
    	if(!ZERO(latt->p))
    		n++;
    }
    return n;
}

Cell *huntMomentum(Cell *latt)
{
    for(int i = 0; i < SIDE3 * SIDE3; i++, latt++)
    {
    	if(!ZERO(latt->p) && latt->off == 0)
    	{
    		return latt;
    	}
    }
    return NULL;
}
