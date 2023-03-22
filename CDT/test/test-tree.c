/*
 * tree-test.c
 *
 *  Created on: 28 de fev. de 2023
 *      Author: Alexandre
 */
#include <stdio.h>
#include "../simulation.h"
#include "../plot3d.h"
#include "../utils.h"
#include "../bresenham.h"

extern int visit;

void explore(int org[3], int level)
{
	visit++;
	for(int dir = 0; dir < 6; dir++)
	{
		int o[3];
		CP(o, org);
	    o[dir>>1] += (dir % 2 == 0) ? +1 : -1;
	    /*
	    if(abs(o[dir>>1]) < abs(org[dir>>1]))
	    {
	    	continue;
	    }
	    */
		if(isAllowed(dir, o))
		{
			int color = o[0] >= 0 && o[1] >= 0 && o[2] >= 0 ? GREEN : ORANGE;
			Vec3 p1, p2;
			if(org[0]==2&&org[1]==0&&org[2]==3&&o[0]==3&&o[1]==0&&o[2]==3)
				color = BLUE;
			if(org[0]==3&&org[1]==0&&org[2]==2&&o[0]==3&&o[1]==0&&o[2]==3)
				color = BLUE;
			p1.x = org[0]*200;
			p1.y = org[1]*200;
			p1.z = org[2]*200;
			p2.x = o[0]*200;
			p2.y = o[1]*200;
			p2.z = o[2]*200;
			//if(color == GREEN)
				line3d(p1, p2, color);
//			if(visit < 346)
	//			printf("dir=%d: [%d,%d,%d] -> [%d,%d,%d]\n", dir, org[0], org[1], org[2], o[0], o[1], o[2]);
			explore(o, level+1);
		}
	}
}


