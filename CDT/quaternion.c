/*
 * quaternion.c
 */
#include <math.h>
#include "quaternion.h"

void normalise(Quaternion *q0)
{
	double n = sqrt(q0->x*q0->x + q0->y*q0->y + q0->z*q0->z + q0->w*q0->w);
	q0->x /= n;
	q0->y /= n;
	q0->z /= n;
	q0->w /= n;
}

void scaleQ(Quaternion *q0, double s)
{
	q0->x *= s;
	q0->y *= s;
	q0->z *= s;
	q0->w *= s;
}
void mul(Quaternion *q0, Quaternion q1, Quaternion q2)
{
	q0->x =  q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
	q0->y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
	q0->z =  q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
	q0->w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
}

/**
 * Calculates the quaternion to rotate one vector onto another
 */
void fromBetweenVectors(Quaternion *q, Vector3d u, Vector3d v)
{
  float ux = u.x;
  float uy = u.y;
  float uz = u.z;
  //
  float vx = v.x;
  float vy = v.y;
  float vz = v.z;
  //
  float dot = ux * vx + uy * vy + uz * vz;
  float wx = uy * vz - uz * vy;
  float wy = uz * vx - ux * vz;
  float wz = ux * vy - uy * vx;
  q->w = dot + sqrt(dot * dot + wx * wx + wy * wy + wz * wz);
  q->x = wx;
  q->y = wy;
  q->z = wz;
  normalise(q);
}

/**
 * Calculates the Hamilton product of two quaternions
 * Leaving out the imaginary part results in just scaling the quat
 */
void mulQ(Quaternion *r, Quaternion a, Quaternion b)
{
  // Q1 * Q2 = [w1 * w2 - dot(v1, v2), w1 * v2 + w2 * v1 + cross(v1, v2)]

  // Not commutative because cross(v1, v2) != cross(v2, v1)!

  float w1 = a.w;
  float x1 = a.x;
  float y1 = a.y;
  float z1 = a.z;

  float w2 = b.w;
  float x2 = b.x;
  float y2 = b.y;
  float z2 = b.z;

  r->w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
  r->x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
  r->y = w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2;
  r->z = w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2;
}

/**
 * Rotates a vector according to the current quaternion, assumes |q|=1
 * @link https://www.xarg.org/proof/vector-rotation-using-quaternions/
 *
 * @param {Array} v The vector to be rotated
 * @returns {Array}
 */
void rotateVector(Vector3d *r, Quaternion q, Vector3d v)
{
  float qw = q.w;
  float qx = q.x;
  float qy = q.y;
  float qz = q.z;

  float vx = v.x;
  float vy = v.y;
  float vz = v.z;

  // t = 2q x v
  float tx = 2 * (qy * vz - qz * vy);
  float ty = 2 * (qz * vx - qx * vz);
  float tz = 2 * (qx * vy - qy * vx);

  // v + w t + q x t

  r->x = vx + qw * tx + qy * tz - qz * ty;
  r->y = vy + qw * ty + qz * tx - qx * tz;
  r->z = vz + qw * tz + qx * ty - qy * tx;
}

void identityQ(Quaternion *q)
{
	q->w = 1;
	q->x = 0;
	q->y = 0;
	q->z = 0;
}
