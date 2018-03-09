/*
 * brick.c
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include "brick.h"

#include <stdio.h>

void copyTile(Brick *dst, Brick *org)
{
	Tuple p0 = dst->p0;
	unsigned short w = dst->p18;
	*dst = *org;
	dst->p0 = p0;
	dst->p18 = w;
}

void cleanTile(Brick *t)
{
	Tuple p0 = t->p0;
	unsigned short w = t->p18;
	memset(t, 0, sizeof(Brick));
	t->p0 = p0;
	t->p18 = w;
}

char *tile2str(Brick *t)
{
	char *ptr;
	asprintf(&ptr, "[p0=%s p1=%d]", tuple2str(&t->p0), t->p1);
	return ptr;
}
