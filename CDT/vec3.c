/*
 * vector3d.c
 *
 *  Created on: 11/01/2016
 *      Author: Alexandre
 */

#include "vec3.h"

#include <math.h>
#include <stdio.h>

char vectorBuf[4][30];

void reset3d(Vec3 *v)
{
	v->x = 0;
	v->y = 0;
	v->z = 0;
}

void normalize(Vec3 *v)
{
	double h = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
	if(h == 0.0)
	{
		v->x = 0;
		v->y = 0;
		v->z = 0;
	}
	else
	{
		v->x /= h;
		v->y /= h;
		v->z /= h;
	}
}

void add3d(Vec3 *a, Vec3 b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
}

void sub3d(Vec3 *a, Vec3 b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
}

void cross3d(Vec3 v1, Vec3 v2, Vec3 *v3)
{
	v3->x = v1.y * v2.z - v1.z * v2.y;
	v3->y = v1.z * v2.x - v1.x * v2.z;
	v3->z = v1.x * v2.y - v1.y * v2.x;
}

void scale3d(Vec3 *v, double s)
{
	v->x *= s;
	v->y *= s;
	v->z *= s;
}
