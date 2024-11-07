/**
 * tree.cpp
 */

#include "simulation.h"

namespace automaton
{
  /**
   * Tests if expansion in the given direction is legal.
   *
   * @dir the von Neumann direction
   * @dst the probed address
   */
  bool isAllowed(int dir, int dst[3])
  {
    // Depth test.

    if (MAG(dst) > SIDE2)
	{
	  return false;
	}

	// Root allows all six directions

    int level = abs(dst[0]) + abs(dst[1]) + abs(dst[2]);
	if (level == 1)
	  return true;

	int dx = (dst[0] < 0) + 0;
	int dy = (dst[1] < 0) + 2;
	int dz = (dst[2] < 0) + 4;

	// Axes

	if(!dst[1] && !dst[2])
	  return dir == dx;
	if(!dst[0] && !dst[2])
	  return dir == dy;
    if(!dst[0] && !dst[1])
	  return dir == dz;

	// Planes

	int mod2 = level % 2;
	if(!dst[2])
	{
	  if(mod2 == 0)
	    return dir == dx;
	  else
	    return dir == dy;
	}
	if(!dst[1])
	{
	  if(mod2 == 0)
	    return dir == dx;
	  else
	    return dir == dz;
	}
	if(!dst[0])
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
}
