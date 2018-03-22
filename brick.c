/*
 * brick.c
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include "brick.h"

#include <stdio.h>

void copyBrick(Brick *dst, Brick *org)
{
	Tuple p0 = dst->p0;
	unsigned short w = dst->p18;
	*dst = *org;
	dst->p0 = p0;
	dst->p18 = w;
}

void cleanBrick(Brick *t)
{
	Tuple p0 = t->p0;
	unsigned short w = t->p18;
	memset(t, 0, sizeof(Brick));
	t->p0 = p0;
	t->p18 = w;
}

char *brick2str(Brick *t)
{
	char *ptr;
	asprintf(&ptr, "[p0=%s,%d p1=%d p23=%u]", tuple2str(&t->p0), t->p18, t->p1, t->p23);
	return ptr;
}
