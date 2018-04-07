/*
 * tuple.cu
 */
#include "tuple.h"
#include "params.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "automaton.h"

__constant__ Tuple V0 = { 1.732*SIDE, 1.732*SIDE, 1.732*SIDE};

__host__ __device__ void resetTuple(Tuple *t)
{
        t->x = 0;
        t->y = 0;
        t->z = 0;
}

__device__ void rectify(Tuple *v)
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

__device__ int isNull(Tuple t)
{
	return t.x == 0 && t.y == 0 && t.z == 0;
}

__device__ int isEqual(Tuple t1, Tuple t2)
{
	return t1.x == t2.x && t1.y == t2.y && t1.z == t2.z;
}

__device__ int isOpposite(Tuple t1, Tuple t2)
{
	return t1.x == -t2.x && t1.y == -t2.y && t1.z == -t2.z;
}

__device__ void invertTuple(Tuple *t)
{
	t->x = -t->x;
	t->y = -t->y;
	t->z = -t->z;
}

__device__ void addTuples(Tuple *a, Tuple b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
}

__device__ void addRectify(Tuple *a, Tuple b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
	rectify(a);
}

__device__ void subTuples(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
}

__device__ void subRectify(Tuple *a, Tuple b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
	rectify(a);
}

__device__ void subTuples3(Tuple *r, Tuple a, Tuple b)
{
	r->x = a.x - b.x;
	r->y = a.y - b.y;
	r->z = a.z - b.z;
}

/*
 * Module.
 */
__device__ double modTuple(Tuple *v)
{
	return sqrt((double)(v->x * v->x + v->y * v->y + v->z * v->z));
}

/*
 * Module squared.
 */
__device__ double mod2Tuple(Tuple *v)
{
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

__device__ void normalizeTuple(Tuple *t)
{
	double h = sqrt((double)(t->x * t->x + t->y * t->y + t->z * t->z));
	t->x = (int)(t->x * SIDE / h);
	t->y = (int)(t->y * SIDE / h);
	t->z = (int)(t->z * SIDE / h);
}

__device__ void tupleCross(Tuple v1, Tuple v2, Tuple *v3)
{
	v3->x = v1.y * v2.z - v1.z * v2.y;
	v3->y = v1.z * v2.x - v1.x * v2.z;
	v3->z = v1.x * v2.y - v1.y * v2.x;
}

__device__ int compareTuples(Tuple *a, Tuple *b)
{
	int la = a->x * a->x + a->y * a->y + a->z * a->z;
	int lb = b->x * b->x + b->y * b->y + b->z * b->z;
	if(la == lb)
		return 0;
	return la > lb ? 1 : -1;
}

__device__ int tupleDot(Tuple *a, Tuple *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

__device__ void tupleAbs(Tuple *t)
{
	t->x = abs(t->x);
	t->y = abs(t->x);
	t->z = abs(t->x);
}

__device__ void scaleTuple(Tuple *t, int s)
{
	t->x *= s;
	t->y *= s;
	t->z *= s;
}

char *tuple2str(Tuple *t)
{
	char *s;
	asprintf((char **)&s, "[%d,%d,%d]", t->x, t->y, t->z);
	return s;
}


