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
    	if(latt->a == 0 && latt->f > 0)
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
    	if(latt->a == 0 && latt->flash)
    		return latt;
    }
    return NULL;
}
