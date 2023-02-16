/*
 * vector3d.c
 *
 *  Created on: 11/01/2016
 *      Author: Alexandre
 */

#include "vector3d.h"
#include <math.h>
#include <stdio.h>

char vectorBuf[4][30];

void normalize(Vector3d *v)
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

void add3d(Vector3d *a, Vector3d b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
}

void sub3d(Vector3d *a, Vector3d b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
}

double dot3d(Vector3d v1, Vector3d v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

void cross3d(Vector3d v1, Vector3d v2, Vector3d *v3)
{
	v3->x = v1.y * v2.z - v1.z * v2.y;
	v3->y = v1.z * v2.x - v1.x * v2.z;
	v3->z = v1.x * v2.y - v1.y * v2.x;
}

void rot3d(Vector3d *p, Vector3d axis, double angle)
{
	double x = p->x;
	double y = p->y;
	double z = p->z;
	double s = sin(angle);
	double c = cos(angle);
	p->x = (c + axis.x * axis.x * (1 - c)) * x + (axis.x * axis.y * (1 - c) - axis.z * s) * y + (axis.x * axis.z * (1 - c) + axis.y * s) * z;
	p->y = (axis.y * axis.x * (1 - c) + axis.z * s) * x + (c + axis.y * axis.y * (1 - c)) * y + (axis.y * axis.z * (1 - c) - axis.x * s) * z;
	p->z = (axis.z * axis.x * (1 - c) - axis.y * s) * x + (axis.z * axis.y * (1 - c) + axis.x * s) * y + (c + axis.z * axis.z * (1 - c)) * z;
}

void scale3d(Vector3d *v, double s)
{
	v->x *= s;
	v->y *= s;
	v->z *= s;
}

double module3d(Vector3d *v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

double norm(Vector3d v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

void absV3d(Vector3d *v)
{
	v->x = fabs(v->x);
	v->y = fabs(v->y);
	v->z = fabs(v->z);
}

char *vector2str(Vector3d *v)
{
	static int index = 0;
	char *ptr = vectorBuf[index];
	sprintf(ptr, "(%f,%f,%f)", v->x, v->y, v->z);
	index++;
	index &= 3;
	return ptr;
}

void invert3d(Vector3d *v)
{
	v->x = -v->x;
	v->y = -v->y;
	v->z = -v->z;
}

double distance3d(Vector3d v1, Vector3d v2)
{
	sub3d(&v2, v1);
	return module3d(&v2);
}

boolean equalVecs(Vector3d v1, Vector3d v2)
{
	return v1.x==v2.x && v1.y==v2.y && v1.z==v2.z;
}

Vector3d subvecs(Vector3d a, Vector3d b)
{
	sub3d(&a, b);
	return a;
}

Vector3d crossvecs(Vector3d a, Vector3d b)
{
	Vector3d v;
	cross3d(a, b, &v);
	return v;
}

boolean nullVec(Vector3d v)
{
	return v.x==0 && v.y==0 && v.z==0;
}

double modSquared(Vector3d v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}
