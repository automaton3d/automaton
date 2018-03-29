/*
 * brick.c
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include "brick.h"

#include <stdio.h>

unsigned signature(Brick *b)
{
	return (SIDE+1)*(SIDE+1) * b->p0.x + (SIDE+1)*b->p0.y + b->p0.z + 1;
}

void copyBrick(Brick *dst, Brick *org)
{
	Tuple p0 = dst->p0;
	unsigned short w = dst->p19;
	*dst = *org;
	dst->p0 = p0;
	dst->p19 = w;
}

void cleanBrick(Brick *t)
{
	Tuple p0 = t->p0;
	unsigned p18 = t->p18;
	unsigned short w = t->p19;
	memset(t, 0, sizeof(Brick));
	t->p0 = p0;
	t->p15.x = -1;
	t->p18 = p18;
	t->p19 = w;
}

char *brick2str(Brick *t)
{
	char *ptr;
	asprintf(&ptr, "[p0=%s,%d p1=%d p23=%u]", tuple2str(&t->p0), t->p18, t->p1, t->p23);
	return ptr;
}
