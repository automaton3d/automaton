/*
 * vector3d.h
 *
 * Conceptually it is not necessary the use of floating point numbers,
 * but in practice it is used in a few cases.
 *
 *  Created on: 11/01/2016
 *      Author: Alexandre
 */

#ifndef VECTOR3D_H_
#define VECTOR3D_H_

#include <windows.h>

#define TINY 0.00001

typedef struct { double x, y, z; } Vector3d;

/// Functions ///

void reset3d(Vector3d *v);
void normalize(Vector3d *v);
void add3d(Vector3d *a, Vector3d b);
void sub3d(Vector3d *a, Vector3d b);
Vector3d subvecs(Vector3d a, Vector3d b);
double dot3d(Vector3d v1, Vector3d v2);
void cross3d(Vector3d v1, Vector3d v2, Vector3d *v3);
Vector3d crossvecs(Vector3d a, Vector3d b);
void rot3d(Vector3d *p, Vector3d axis, double angle);
void scale3d(Vector3d *v, double s);
double module3d(Vector3d *v);
double norm(Vector3d v);
void absV3d(Vector3d *v);
char *vector2str(Vector3d *v);
void invert3d(Vector3d *v);
double distance3d(Vector3d v1, Vector3d v2);
boolean equalVecs(Vector3d v1, Vector3d v2);
boolean nullVec(Vector3d v);
double modSquared(Vector3d v);

#endif /* VECTOR3D_H_ */
