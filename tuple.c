/*
 * tuple.c
 *
 * Discrete vectors management.
 *
 *  Created on: 13/01/2016
 *      Author: Alexandre
 */

#include "tuple.h"
#include <math.h>
#include <stdio.h>
#include "utils.h"

const Tuple dirs[] = {{+1,0,0}, {-1,0,0}, {0,+1,0}, {0,-1,0}, {0,0,+1}, {0,0,-1}};

const Tuple X0 = { 1,0,0 };
const Tuple Y0 = { 0,1,0 };
const Tuple Z0 = { 0,0,1 };

Tuple V0 = { 1.732*SIDE, 1.732*SIDE, 1.732*SIDE};

int minXYZ(Tuple *v)
{
	return min(min(abs(v->x), abs(v->y)), abs(v->z));
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
	if(v->x < 0)
		v->x += SIDE;
	if(v->y < 0)
		v->y += SIDE;
	if(v->z < 0)
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
}

void addRectify(Tuple *a, Tuple b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
	rectify(a);
}

void subTuples(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
}

void subRectify(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
	rectify(a);
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

void normalizeTuple(Tuple *t)
{
	double h = sqrt(t->x * t->x + t->y * t->y + t->z * t->z);
	t->x = (int)(t->x * SIDE / h);
	t->y = (int)(t->y * SIDE / h);
	t->z = (int)(t->z * SIDE / h);
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

Tuple getUnit(Tuple *t)
{
	Tuple r = X0;
	if(abs(t->x) >= abs(t->y) && abs(t->y) >= abs(t->z))
	{
		r = X0;
		r.x = signum(t->x);
	}
	else if(t->y >= t->x && t->y >= t->z)
	{
		r = Y0;
		r.y = signum(t->y);
	}
	else if(t->z >= t->y && t->z >= t->x)
	{
		r = Z0;
		r.z = signum(t->z);
	}
	return r;
}

Tuple getDirection(Tuple a, Tuple b)
{
	Tuple d;
	d.x = b.x - a.x;
	d.y = b.y - a.y;
	d.z = b.z - a.z;
	d.x = b.x - signum(d.x) * SIDE;
	d.y = b.y - signum(d.y) * SIDE;
	d.z = b.z - signum(d.z) * SIDE;
	return getUnit(&d);
}

/*
 * Normalized, discrete dot product.
 */
int dot(Tuple t1, Tuple t2)
{
	long id = t1.x*t2.x + t1.y*t2.y + t1.z*t2.z;
	return signum(id)*(abs(id) >> (ORDER-1));
}

/*
 * Integer module of a Tuple;
 */
unsigned imod(Tuple v)
{
	return sqr(v.x*v.x + v.y*v.y + v.z*v.z);
}

unsigned imod2(Tuple v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

char *tuple2str(Tuple *t)
{
	char *s;
	asprintf((char **)&s, "[%d,%d,%d]", t->x, t->y, t->z);
	return s;
}

