/*
 * preon.c
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include "tile.h"
#include <stdio.h>

void copyTile(Tile *dst, Tile *org)
{
	Tuple p0 = dst->p0;
	unsigned short w = dst->p16;
	*dst = *org;
	dst->p0 = p0;
	dst->p16 = w;
}

void cleanTile(Tile *t)
{
	Tuple p0 = t->p0;
	unsigned short w = t->p16;
	memset(t, 0, sizeof(Tile));
	t->p0 = p0;
	t->p16 = w;
}

char *tile2str(Tile *t)
{
	char *ptr;
	asprintf(&ptr, "[p0=%s p1=%d]", tuple2str(&t->p0), t->p1);
	return ptr;
}
