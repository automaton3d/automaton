/*
 * tree.c
 */

#include "tree.h"
#include "common.h"

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
boolean isAllowed(int dir, Tuple p, unsigned char d0)
{
	double d1 = mod2Tuple(&p);
	int x = p.x + dirs[dir].x;
	int y = p.y + dirs[dir].y;
	int z = p.z + dirs[dir].z;
	double d2 = x*x + y*y + z*z;
	if(d2 <= d1)
		return false;
	//
	// Wrapping test
	//
	if(x == SIDE/2 || x == -SIDE/2-1 || y == SIDE/2 || y == -SIDE/2-1 || z == SIDE/2 || z == -SIDE/2-1)
		return false;
	//
	// Root
	//
	int level = abs(x) + abs(y) + abs(z);
	if(level == 1)
		return true;
	//
	// x axis
	//
	if(x > 0 && y == 0 && z == 0 && dir == 0)
		return true;
	else if(x < 0 && y == 0 && z == 0 && dir == 1)
		return true;
	//
	// y axis
	//
	else if(x == 0 && y > 0 && z == 0 && dir == 2)
		return true;
	else if(x == 0 && y < 0 && z == 0 && dir == 3)
		return true;
	//
	// z axis
	//
	else if(x == 0 && y == 0 && z > 0 && dir == 4)
		return true;
	else if(x == 0 && y == 0 && z < 0 && dir == 5)
		return true;
	//
	// xy plane
	//
	else if(x > 0 && y > 0 && z == 0)
	{
		if(level % 2 == 1)
			return (dir == 0 && d0 == 2);
		else
			return (dir == 2 && d0 == 0);
	}
	else if(x < 0 && y > 0 && z == 0)
	{
		if(level % 2 == 1)
			return (dir == 1 && d0 == 2);
		else
			return (dir == 2 && d0 == 1);
	}
	else if(x > 0 && y < 0 && z == 0)
	{
		if(level % 2 == 1)
			return (dir == 0 && d0 == 3);
		else
			return (dir == 3 && d0 == 0);
	}
	else if(x < 0 && y < 0 && z == 0)
	{
		if(level % 2 == 1)
			return (dir == 1 && d0 == 3);
		else
			return (dir == 3 && d0 == 1);
	}
	//
	// yz plane
	//
	else if(x == 0 && y > 0 && z > 0)
	{
		if(level % 2 == 0)
			return (dir == 4 && d0 == 2);
		else
			return (dir == 2 && d0 == 4);
	}
	else if(x == 0 && y < 0 && z > 0)
	{
		if(level % 2 == 0)
			return (dir == 4 && d0 == 3);
		else
			return (dir == 3 && d0 == 4);
	}
	else if(x == 0 && y > 0 && z < 0)
	{
		if(level % 2 == 0)
			return (dir == 5 && d0 == 2);
		else
			return (dir == 2 && d0 == 5);
	}
	else if(x == 0 && y < 0 && z < 0)
	{
		if(level % 2 == 0)
			return (dir == 5 && d0 == 3);
		else
			return (dir == 3 && d0 == 5);
	}
	//
	// zx plane
	//
	else if(x > 0 && y == 0 && z > 0)
	{
		if(level % 2 == 1)
			return (dir == 4 && d0 == 0);
		else
			return (dir == 0 && d0 == 4);
	}
	else if(x < 0 && y == 0 && z > 0)
	{
		if(level % 2 == 1)
			return (dir == 4 && d0 == 1);
		else
			return (dir == 1 && d0 == 4);
	}
	else if(x > 0 && y == 0 && z < 0)
	{
		if(level % 2 == 1)
			return (dir == 5 && d0 == 0);
		else
			return (dir == 0 && d0 == 5);
	}
	else if(x < 0 && y == 0 && z < 0)
	{
		if(level % 2 == 1)
			return (dir == 5 && d0 == 1);
		else
			return (dir == 1 && d0 == 5);
	}
	else
	{
		// Spirals
		//
		int x0 = x + SIDE/2;
		int y0 = y + SIDE/2;
		int z0 = z + SIDE/2;
		//
		switch(level % 3)
		{
			case 0:
				if(x0 != SIDE/2 && y0 != SIDE/2)
					return (z0 > SIDE/2 && dir == 4) || (z0 < SIDE/2 && dir == 5);
				break;
			case 1:
				if(y0 != SIDE/2 && z0 != SIDE/2)
					return (x0 > SIDE/2 && dir == 0) || (x0 < SIDE/2 && dir == 1);
				break;
			case 2:
				if(x0 != SIDE/2 && z0 != SIDE/2)
					return (y0 > SIDE/2 && dir == 2) || (y0 < SIDE/2 && dir == 3);
				break;
		}
	}
	return false;
}
