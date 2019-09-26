/*
 * gadget.c
 *
 *  Created on: 23 de set de 2019
 *      Author: Alexandre
 */

#include "common.h"
#include "plot3d.h"
#include "text.h"

boolean ticks[2];

void showCheckbox(int x, int y, char *label)
{
	line(x, HEIGHT-y, x+10, HEIGHT-y, WHT);
	line(x, HEIGHT-y-10, x+10, HEIGHT-y-10, WHT);
	line(x, HEIGHT-y, x, HEIGHT-y-10, WHT);
	line(x+10, HEIGHT-y, x+10, HEIGHT-y-10, WHT);
	//
	vprints(x+15, y, label);
	//
	if(ticks[(y-100)/20])
	{
		line(x+1, HEIGHT-y-4, x+4, HEIGHT-y-8, GG);
		line(x+4, HEIGHT-y-8, x+9, HEIGHT-y-1, GG);
	}
}

void checkTick(int x, int y)
{
	if(x > 18 && x < 100)
	{
		if(y > 100 && y < 120)
			ticks[0] = !ticks[0];
		if(y > 120 && y < 140)
			ticks[1] = !ticks[1];
	}
}

