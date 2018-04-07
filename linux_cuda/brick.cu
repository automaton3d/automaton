/*
 * brick.cu
 */

#include "brick.h"

__device__ void copyBrick(Brick *dst, Brick *org)
{
	Tuple p0 = dst->p0;
	unsigned short w = dst->p19;
	*dst = *org;
	dst->p0 = p0;
	dst->p19 = w;
}

__device__ void cleanBrick(Brick *t)
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


