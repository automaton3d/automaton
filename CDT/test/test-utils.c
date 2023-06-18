/*
 * test-utils.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */
#include "test.h"
#include "../simulation.h"

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
