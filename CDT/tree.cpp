/**
 * tree.c
 */

#include "model/simulation.h"

bool isAllowed(int dir, short org[3])
{
	if (MAG(org) > SIDE2)
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

/*
void explore(short org[3], int level)
{
	boolean shell = true;
	for(int dir = 0; dir < 6; dir++)
	{
		short o[3];
		CP(o, org);
	    o[dir>>1] += (dir % 2 == 0) ? +1 : -1;
        if(abs(o[dir >> 1]) < abs(org[dir >> 1]))
            continue;
		if(isAllowed(dir, o))
		{
			shell = false;
			explore(o, level+1);
		}
	}
	if(shell)
	{
	    const float GRID_SIZE =  0.5 / SIDE2;
        float px = org[0] * GRID_SIZE - 0.25f;
        float py = org[1] * GRID_SIZE - 0.25f;
        float pz = org[2] * GRID_SIZE - 0.25f;
        glVertex3f(px, py, pz);
	}
}
*/
