/*
 * vector3d.cu
 */
#include "vector3d.h"
#include <math.h>
#include <stdio.h>

char vectorBuf[4][30];

__host__ __device__ void norm3d(Vector3d *v)
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

__host__ __device__ void add3d(Vector3d *a, Vector3d b)
{
	a->x += b.x;
	a->y += b.y;
	a->z += b.z;
}

__host__ __device__ void sub3d(Vector3d *a, Vector3d b)
{
	a->x -= b.x;
	a->y -= b.y;
	a->z -= b.z;
}

__host__ __device__ double dot3d(Vector3d v1, Vector3d v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

__host__ __device__ void cross3d(Vector3d v1, Vector3d v2, Vector3d *v3)
{
	v3->x = v1.y * v2.z - v1.z * v2.y;
	v3->y = v1.z * v2.x - v1.x * v2.z;
	v3->z = v1.x * v2.y - v1.y * v2.x;
}

__host__ __device__ void rot3d(Vector3d *p, Vector3d axis, double angle)
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

__host__ __device__ void scale3d(Vector3d *v, double s)
{
	v->x *= s;
	v->y *= s;
	v->z *= s;
}

__host__ __device__ double module3d(Vector3d *v)
{
	return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

__host__ __device__ void absV3d(Vector3d *v)
{
	v->x = fabs(v->x);
	v->y = fabs(v->y);
	v->z = fabs(v->z);
}

__host__ __device__ void invert3d(Vector3d *v)
{
	v->x = -v->x;
	v->y = -v->y;
	v->z = -v->z;
}

__host__ char *vector2str(Vector3d *v)
{
        static int index = 0;
        char *ptr = vectorBuf[index];
        sprintf(ptr, "(%f,%f,%f)", v->x, v->y, v->z);
        index++;
        index &= 3;
        return ptr;
}

