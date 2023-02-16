/*
 * quaternion.c
 */
#include "quaternion.h"
#include <math.h>

void normalise(Quaternion q0)
{
	double n = sqrt(q0.x*q0.x + q0.y*q0.y + q0.z*q0.z + q0.w*q0.w);
	q0.x /= n;
	q0.y /= n;
	q0.z /= n;
	q0.w /= n;
}
void scaleQ(Quaternion q0, double s)
{
	q0.x *= s;
	q0.y *= s;
	q0.z *= s;
	q0.w *= s;
}
void mul(Quaternion *q0, Quaternion q1, Quaternion q2)
{
	q0->x =  q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
	q0->y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
	q0->z =  q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
	q0->w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
}
