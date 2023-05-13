/*
 * vector3d.h
 *
 * Conceptually it is not necessary the use of floating point numbers,
 * but in practice it is used in a few cases.
 *
 *  Created on: 11/01/2016
 *      Author: Alexandre
 */

#ifndef VEC3_H_
#define VEC3_H_

#include <windows.h>

#define TINY 0.00001

typedef struct { double x, y, z; } Vec3;

/// Functions ///

void normalize(Vec3 *v);
void scale3d(Vec3 *v, double s);
void reset3d(Vec3 *v);
void cross3d(Vec3 v1, Vec3 v2, Vec3 *v3);
void add3d(Vec3 *a, Vec3 b);
void sub3d(Vec3 *a, Vec3 b);

#endif /* VEC3_H_ */
