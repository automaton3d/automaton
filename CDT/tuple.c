/*
 * tuple.c
 *
 * Discrete vectors management.
 *
 *  Created on: 13/01/2016
 *      Author: Alexandre
 */

#define _GNU_SOURCE
#include "tuple.h"
#include <math.h>
#include <stdio.h>
#include "utils.h"

const Tuple dirs[] = {{+1,0,0}, {-1,0,0}, {0,+1,0}, {0,-1,0}, {0,0,+1}, {0,0,-1}};

int minXYZ(Tuple *v)
{
	return min(min(abs(v->x), abs(v->y)), abs(v->z));
}

bool isNull(Tuple t)
{
	return t.x == 0 && t.y == 0 && t.z == 0;
}

bool isEqual(Tuple t1, Tuple t2)
{
	return t1.x == t2.x && t1.y == t2.y && t1.z == t2.z;
}

bool isOpposite(Tuple t1, Tuple t2)
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
}

void subTuples(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
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
	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
}

/*
 * Module squared.
 */
double mod2Tuple(Tuple *v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

/*
void normalizeTuple(Tuple *t)
{
	double h = sqrt(t->x * t->x + t->y * t->y + t->z * t->z);
	t->x = (int)(t->x * SIDE / h);
	t->y = (int)(t->y * SIDE / h);
	t->z = (int)(t->z * SIDE / h);
}
*/

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

/*
 * Integer module of a Tuple;
 */
unsigned imod(Tuple v)
{
	return (int)sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

unsigned imod2(Tuple v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

char *tuple2str(Tuple *t)
{
	char *s = NULL;
	asprintf(&s, "[%d,%d,%d]", t->x, t->y, t->z);
	return s;
}

Tuple getUnit(Tuple *t)
{
	Tuple r;
	r.x = 0;
	r.y = 0;
	r.z = 0;
	//
	if(abs(t->x) > abs(t->y))
	{
		if(abs(t->x) > abs(t->z))
			r.x = signum(t->x);
		else
			r.z = signum(t->z);
	}
	else if(abs(t->x) < abs(t->y))
	{
		if(abs(t->y) > abs(t->z))
			r.y = signum(t->y);
		else
			r.z = signum(t->z);
	}
	else
	{
		if(abs(t->x) > abs(t->z))
			r.x = signum(t->x);
		else
			r.z = signum(t->z);
	}
	return r;
}

/*
 * Gets the von Neummann direction that closely matches this tuple.
 */
int getDir(Tuple t)
{
	if(abs(t.x) > abs(t.y))
	{
		if(abs(t.x) > abs(t.z))
			t.y = t.z = 0;
		else
			t.y = t.x = 0;
	}
	else if(abs(t.x) < abs(t.y))
	{
		if(abs(t.y) > abs(t.z))
			t.x = t.z = 0;
		else
			t.x = t.y = 0;
	}
	else
	{
		if(abs(t.x) > abs(t.z))
			t.y = t.z = 0;
		else
			t.y = t.x = 0;
	}
	//
	if(t.x) return t.x<0 ? 1 : 0;
	if(t.y) return t.y<0 ? 3 : 2;
	if(t.z) return t.z<0 ? 5 : 4;
	return -1;
}

int distanceTT(Tuple *t1, Tuple *t2)
{
	int dx = t1->x - t2->x;
	int dy = t1->y - t2->y;
	int dz = t1->z - t2->z;
	return (int) sqrt(dx*dx + dy*dy + dz*dz);
}

