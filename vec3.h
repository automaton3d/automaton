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

void reset3d(Vec3 *v);
void normalize(Vec3 *v);
void add3d(Vec3 *a, Vec3 b);
void sub3d(Vec3 *a, Vec3 b);
Vec3 subvecs(Vec3 a, Vec3 b);
double dot3d(Vec3 v1, Vec3 v2);
void cross3d(Vec3 v1, Vec3 v2, Vec3 *v3);
Vec3 crossvecs(Vec3 a, Vec3 b);
void rot3d(Vec3 *p, Vec3 axis, double angle);
void scale3d(Vec3 *v, double s);
double module3d(Vec3 *v);
double norm(Vec3 v);
void absV3d(Vec3 *v);
char *vector2str(Vec3 *v);
void invert3d(Vec3 *v);
double distance3d(Vec3 v1, Vec3 v2);
boolean equalVecs(Vec3 v1, Vec3 v2);
boolean nullVec(Vec3 v);
double modSquared(Vec3 v);

#endif /* VEC3_H_ */
