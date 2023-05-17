/*
 * test-utils.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */
#include "test.h"
#include "../simulation.h"
#include "../utils.h"

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
