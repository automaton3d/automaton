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

boolean tree3d(int dir, int org[3])
{
    // Wrapping test

    if (abs(org[0]) == SIDE_2 + 1 || abs(org[1]) == SIDE_2 + 1 || abs(org[2]) == SIDE_2 + 1)
    	return false;

    // Root allows all six directions
    // Wrapping level too

    int level = abs(org[0]) + abs(org[1]) + abs(org[2]);
    if (level == 1 || level == 3 * SIDE_2)
        return true;

    // Axes

    if(abs(org[0]) == SIDE_2 && abs(org[1]) == SIDE_2)
    {
    	if(org[2] < 0)
    		return dir == 5;
    	else
    		return dir == 4;
    }
    if(abs(org[0]) == SIDE_2 && abs(org[2]) == SIDE_2)
    {
    	if(org[1] < 0)
    		return dir == 3;
    	else
    		return dir == 2;
    }
    if(abs(org[1]) == SIDE_2 && abs(org[2]) == SIDE_2)
    {
    	if(org[0] < 0)
    		return dir == 1;
    	else
    		return dir == 0;
    }

    // Allowed directions

    int dx = (org[0] < 0) + 0;
    int dy = (org[1] < 0) + 2;
    int dz = (org[2] < 0) + 4;

    if(!org[1] && !org[2])
    	return dir == dx;
    if(!org[0] && !org[2])
    	return dir == dy;
    if(!org[0] && !org[1])
    	return dir == dz;

    // Planes

	int parity = (level % 2 == 0);

    if(abs(org[2]) == SIDE_2)
    {
    	// x-axis or y-axis

    	if(org[1] == 0)
    		return parity ? dir == dx : dir == dy;
    	if(org[0] == 0)
    		return parity ? dir == dy : dir == dx;

    	// xy-plane

    	if(parity)
    		return dir == dx;
    	else
    		return dir == dy;
    }
    else if(abs(org[1]) == SIDE_2)
    {
    	// x-axis or z-axis

    	if(org[2] == 0)
    		return parity ? dir == dx : dir == dz;
    	if(org[0] == 0)
    		return parity ? dir == dz : dir == dx;

    	// zx-plane

    	if(parity)
    		return dir == dx;
    	else
    		return dir == dz;
    }
    else if(abs(org[0]) == SIDE_2)
    {
    	// y-axis or z-axis

    	if(org[1] == 0)
    		return parity ? dir == dz : dir == dy;
    	if(org[2] == 0)
    		return parity ? dir == dy : dir == dz;

    	// yz-plane

    	if(parity)
    		return dir == dz;
    	else
    		return dir == dy;
    }

    if(!org[2])
    {
    	if(parity)
    		return dir == dx;
    	else
    		return dir == dy;
    }
    if(!org[1])
    {
    	if(parity)
    		return dir == dz;
    	else
    		return dir == dx;
    }
    if(!org[0])
    {
    	if(parity)
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
