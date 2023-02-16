/*
 * gadget.c
 *
 *  Created on: 23 de set de 2019
 *      Author: Alexandre
 */

#include <windows.h>

#include "common.h"
#include "engine.h"
#include "plot3d.h"
#include "text.h"

extern boolean ticks[NTICKS];

void showCheckbox(int x, int y, char *label)
{
	line2d(x, HEIGHT-y, x+10, HEIGHT-y, WHT);
	line2d(x, HEIGHT-y-10, x+10, HEIGHT-y-10, WHT);
	line2d(x, HEIGHT-y, x, HEIGHT-y-10, WHT);
	line2d(x+10, HEIGHT-y, x+10, HEIGHT-y-10, WHT);
	//
	vprints(x+15, y, label);
	//
	if(ticks[(y-100)/20-1])
	{
		line2d(x+1, HEIGHT-y-4, x+4, HEIGHT-y-8, YELLOW);
		line2d(x+4, HEIGHT-y-8, x+9, HEIGHT-y-1, YELLOW);
	}
}

boolean testCheckbox(int x, int y)
{
	if(x > 17 && x < 100)
	{
		int yy = 120;
		for(int i = 0; i < NTICKS; i++)
		{
			if(y > yy && y < yy+20)
			{
				ticks[i] = !ticks[i];
				return true;
			}
			yy += 20;
		}
	}
	return false;
}

