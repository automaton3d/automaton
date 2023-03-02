/*
============
tree-test.c
============
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "simulation.h"
#include "utils.h"

#define true	1
#define false	0

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
int isAllowed(int dir, int org[3])
{
    // Wrapping test
    //
    if (org[0] == S + 1 || org[0] == -S || org[1] == S + 1 || org[1] == -S || org[2] ==S + 1 || org[2] == -S)
    {
    	//puts("PUTZ!!!!");
        return false;
    }
    //
    // Root allows all six directions
    //
    int level = abs(org[0]) + abs(org[1]) + abs(org[2]);
    assert(level > 0);
    if (level == 1)
        return true;
    //
    // x-axis
    //
    if (org[0] > 0 && org[1] == 0 && org[2] == 0 && dir == 0)
        return true;
    else if (org[0] < 0 && org[1] == 0 && org[2] == 0 && dir == 1)
        return true;
    //
    // y-axis
    //
    else if (org[0] == 0 && org[1] > 0 && org[2] == 0 && dir == 2)
        return true;
    else if (org[0] == 0 && org[1] < 0 && org[2] == 0 && dir == 3)
        return true;
    //
    // z-axis
    //
    else if (org[0] == 0 && org[1] == 0 && org[2] > 0 && dir == 4)
        return true;
    else if (org[0] == 0 && org[1] == 0 && org[2] < 0 && dir == 5)
        return true;
    //
    // xy-plane
    //
    else if (org[0] > 0 && org[1] > 0 && org[2] == 0)
    {
        if (level % 2 == 1)
            return (dir == 0);
        else
            return (dir == 2);
    }
    else if (org[0] < 0 && org[1] > 0 && org[2] == 0)
    {
        if (level % 2 == 1)
            return (dir == 1);
        else
            return (dir == 2);
    }
    else if (org[0] > 0 && org[1] < 0 && org[2] == 0)
    {
        if (level % 2 == 1)
            return (dir == 0);
        else
            return (dir == 3);
    }
    else if (org[0] < 0 && org[1] < 0 && org[2] == 0)
    {
        if (level % 2 == 1)
            return (dir == 1);
        else
            return (dir == 3);
    }
    //
    // yz-plane
    //
    else if (org[0] == 0 && org[1] > 0 && org[2] > 0)
    {
        if (level % 2 == 0)
            return (dir == 4);
        else
            return (dir == 2);
    }
    else if (org[0] == 0 && org[1] < 0 && org[2] > 0)
    {
        if (level % 2 == 0)
            return (dir == 4);
        else
            return (dir == 3);
    }
    else if (org[0] == 0 && org[1] > 0 && org[2] < 0)
    {
        if (level % 2 == 0)
            return (dir == 5);
        else
            return (dir == 2);
    }
    else if (org[0] == 0 && org[1] < 0 && org[2] < 0)
    {
        if (level % 2 == 0)
            return (dir == 5);
        else
            return (dir == 3);
    }
    //
    // zx-plane
    //
    else if (org[0] > 0 && org[1] == 0 && org[2] > 0)
    {
        if (level % 2 == 1)
            return (dir == 4);
        else
            return (dir == 0);
    }
    else if (org[0] < 0 && org[1] == 0 && org[2] > 0)
    {
        if (level % 2 == 1)
            return (dir == 4);
        else
            return (dir == 1);
    }
    else if (org[0] > 0 && org[1] == 0 && org[2] < 0)
    {
        if (level % 2 == 1)
            return (dir == 5);
        else
            return (dir == 0);
    }
    else if (org[0] < 0 && org[1] == 0 && org[2] < 0)
    {
        if (level % 2 == 1)
            return (dir == 5);
        else
            return (dir == 1);
    }
    else
    {
    	int q = (org[0] < 0) | ((org[1] < 0)<<1) | ((org[2] < 0)<<2);
    	int ph = level % 3;

        // Spirals
        //
        switch (q)
        {
        case 0:
        	return (dir == 4 && ph == 0) || (dir == 2 && ph == 1) || (dir == 0 && ph == 2);
        case 1:
        	return (dir == 4 && ph == 0) || (dir == 2 && ph == 1) || (dir == 1 && ph == 2);
        case 2:
        	return (dir == 4 && ph == 0) || (dir == 3 && ph == 1) || (dir == 0 && ph == 2);
        case 3:
        	return (dir == 4 && ph == 0) || (dir == 3 && ph == 1) || (dir == 1 && ph == 2);
        case 4:
        	return (dir == 5 && ph == 0) || (dir == 2 && ph == 1) || (dir == 0 && ph == 2);
        case 5:
        	return (dir == 5 && ph == 0) || (dir == 2 && ph == 1) || (dir == 1 && ph == 2);
        case 6:
        	return (dir == 5 && ph == 0) || (dir == 3 && ph == 1) || (dir == 0 && ph == 2);
        case 7:
        	return (dir == 5 && ph == 0) || (dir == 3 && ph == 1) || (dir == 1 && ph == 2);
        }
    }
    return false;
}
