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

typedef struct { double x, y, z; } Vector3d;

/// Functions ///

void norm3d(Vector3d *v);
void add3d(Vector3d *a, Vector3d b);
void sub3d(Vector3d *a, Vector3d b);
double dot3d(Vector3d v1, Vector3d v2);
void cross3d(Vector3d v1, Vector3d v2, Vector3d *v3);
void rot3d(Vector3d *p, Vector3d axis, double angle);
void scale3d(Vector3d *v, double s);
double module3d(Vector3d *v);
void absV3d(Vector3d *v);
char *vector2str(Vector3d *v);
void invert3d(Vector3d *v);

#endif /* VECTOR3D_H_ */
