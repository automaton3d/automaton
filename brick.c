/*
 * brick.c
 *
 * Bricks are used to build preon wavefronts.
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include <stdio.h>
#include <assert.h>
#include "automaton.h"
#include "brick.h"

/*
 * Copies a brick.
 * Preserves registers a, dirs
 */
void copyBrick(Brick *dst, Brick *org)
{
	assert(dst->w==org->w);
	Tuple a = dst->a;
	unsigned char tmp[6];
	memcpy(tmp, dst->dirs, 6);
	*dst = *org;
	dst->a = a;
	memcpy(dst->dirs, tmp, 6);
}

/*
 * Cleans the brick's registers,
 * except a, dirs and w.
 */
void cleanBrick(Brick *b)
{
	Tuple a = b->a;
	unsigned char tmp[6];
	memcpy(tmp, b->dirs, 6);
	unsigned short w = b->w;
	memset(b, 0, sizeof(Brick));
	b->a = a;
	b->w = w;
	memcpy(b->dirs, tmp, 6);
}

/*
 * String representing the brick (for debug purposes).
 */
char *brick2str(Brick *b)
{
	char *ptr;
	asprintf(&ptr, "[p0=%s p1=%d]", tuple2str(&b->a), b->t);
	return ptr;
}

/*
 * Tests if the colors of the pairs neutralize each other.
 */
boolean neutralized(Brick *b1, Brick *b2)
{
	return (b1->R^b2->R) && (b1->G^b2->G) && (b1->B^b2->B);
}
