/*
 * tuple.c
 *
 *  Created on: 13/01/2016
 *      Author: Alexandre
 */

#include <math.h>
#include <stdio.h>
#include "tuple.h"

char tupleBuf[4][30];

void initDirs()
{
	dirs[0].x = 1;
	dirs[0].y = 0;
	dirs[0].z = 0;
	//
	dirs[1].x = -1;
	dirs[1].y = 0;
	dirs[1].z = 0;
	//
	dirs[2].x = 0;
	dirs[2].y = 1;
	dirs[2].z = 0;
	//
	dirs[3].x = 0;
	dirs[3].y = -1;
	dirs[3].z = 0;
	//
	dirs[4].x = 0;
	dirs[4].y = 0;
	dirs[4].z = 1;
	//
	dirs[5].x = 0;
	dirs[5].y = 0;
	dirs[5].z = -1;
}

void rectify(Tuple *v)
{
	if(v->x >= SIDE)
		v->x -= SIDE;
	if(v->y >= SIDE)
		v->y -= SIDE;
	if(v->z >= SIDE)
		v->z -= SIDE;
	//
	if(v->x < -SIDE)
		v->x += SIDE;
	if(v->y < -SIDE)
		v->y += SIDE;
	if(v->z < -SIDE)
		v->z += SIDE;
}

boolean isNull(Tuple t)
{
	return t.x == 0 && t.y == 0 && t.z == 0;
}

boolean isEqual(Tuple t1, Tuple t2)
{
	return t1.x == t2.x && t1.y == t2.y && t1.z == t2.z;
}

boolean isOpposite(Tuple t1, Tuple t2)
{
	return t1.x == -t2.x && t1.y == -t2.y && t1.z == -t2.z;
}

void invertTuple(Tuple *t)
{
	t->x = -t->x;
	t->y = -t->y;
	t->z = -t->z;
}

void addTuples(Tuple *a, Tuple b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
	//rectify(a);
}

void subTuples(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
	//rectify(a);
}

void subTuples3(Tuple *r, Tuple a, Tuple b)
{
	r->x = a.x - b.x;
	r->y = a.y - b.y;
	r->z = a.z - b.z;
}

/*
 * Module.
 */
double modTuple(Tuple *v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

/*
 * Module squared.
 */
double mod2Tuple(Tuple *v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

void normalizeTuple(Tuple *t)
{
	double h = sqrt(t->x * t->x + t->y * t->y + t->z * t->z);
	t->x = (int)(t->x * SIDE / h);
	t->y = (int)(t->y * SIDE / h);
	t->z = (int)(t->z * SIDE / h);
}

char *tuple2str(Tuple *t)
{
	static int index = 0;
	char *ptr = tupleBuf[index];
	sprintf(ptr, "[%d,%d,%d]", t->x, t->y, t->z);
	index++;
	index &= 3;
	return ptr;
}

void tupleCross(Tuple v1, Tuple v2, Tuple *v3)
{
	v3->x = v1.y * v2.z - v1.z * v2.y;
	v3->y = v1.z * v2.x - v1.x * v2.z;
	v3->z = v1.x * v2.y - v1.y * v2.x;
}

int compareTuples(Tuple *a, Tuple *b)
{
	int la = a->x * a->x + a->y * a->y + a->z * a->z;
	int lb = b->x * b->x + b->y * b->y + b->z * b->z;
	if(la == lb)
		return 0;
	return la > lb ? 1 : -1;
}

int tupleDot(Tuple *a, Tuple *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

void tupleAbs(Tuple *t)
{
	t->x = abs(t->x);
	t->y = abs(t->x);
	t->z = abs(t->x);
}

void tupleCopy(Tuple *a, Tuple b)
{
	a->x = b.x;
	a->y = b.y;
	a->z = b.z;
}

void resetTuple(Tuple *t)
{
	t->x = 0;
	t->y = 0;
	t->z = 0;
}

void scaleTuple(Tuple *t, int s)
{
	t->x *= s;
	t->y *= s;
	t->z *= s;
}

