/**
 * tree.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "simulation.h"
#include "utils.h"

#define true	1
#define false	0

boolean isAllowed(int dir, int org[3])
{
    // Wrapping test

    if (abs(org[0]) == SIDE - 1 || abs(org[1]) == SIDE - 1 || abs(org[2]) == SIDE - 1)
        return false;

    // Root allows all six directions

    int level = abs(org[0]) + abs(org[1]) + abs(org[2]);
    if (level == 1)
        return true;

    int dx = (org[0] < 0) + 0;
    int dy = (org[1] < 0) + 2;
    int dz = (org[2] < 0) + 4;

    // Axes

    if(!org[1] && !org[2])
    	return dir == dx;
    if(!org[0] && !org[2])
    	return dir == dy;
    if(!org[0] && !org[1])
    	return dir == dz;

    // Planes

    int mod2 = level % 2;
    if(!org[2])
    {
    	if(mod2 == 0)
    		return dir == dx;
    	else
    		return dir == dy;
    }
    if(!org[1])
    {
    	if(mod2 == 0)
    		return dir == dx;
    	else
    		return dir == dz;
    }
    if(!org[0])
    {
    	if(mod2 == 0)
    		return dir == dy;
    	else
    		return dir == dz;
    }

    // Spirals

    int mod3 = level % 3;
    if(mod3 == 0)
    	return dir == dx;
    if(mod3 == 1)
    	return dir == dy;
    else
    	return dir == dz;
}
