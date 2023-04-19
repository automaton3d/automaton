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
#include <assert.h>

extern int visit;

void explore(int org[3], int level)
{
	if(abs(org[0] == SIDE_2 - 1) && abs(org[1] == SIDE_2 - 1) && abs(org[2] == SIDE_2 - 1))
		return;
	for(int i = 0; i < level; i++)
		printf("    ");
	printf("%d: [%d,%d,%d]\n", visit, org[0], org[1], org[2]);

	visit++;
	delay(30);


    int o[3];

	for(int dir = 0; dir < 6; dir++)
	{
	    CP(o, org);
	    int i = dir >> 1;
	    o[i] += (dir % 2 == 0) ? +1 : -1;

	    if(tree3d(dir, o))
		{
			int color = o[0] >= 0 && o[1] >= 0 && o[2] >= 0 ? GREEN : ORANGE;
			Vec3 p1, p2;
			p1.x = org[0]*200;
			p1.y = org[1]*200;
			p1.z = org[2]*200;
			p2.x = o[0]*200;
			p2.y = o[1]*200;
			p2.z = o[2]*200;
			line3d(p1, p2, color);
			explore(o, level+1);
		}
	}
}


