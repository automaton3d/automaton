/*
 * quaternion.h
 *
 *  Created on: 7 de mai de 2017
 *      Author: Alexandre
 */

#ifndef QUATERNION_H_
#define QUATERNION_H_

#include "vector3d.h"

typedef struct
{
	double x, y, z, w;

} Quaternion;

/// Functions ///

void conjugate(Quaternion q0, Quaternion q1);
void normalise(Quaternion *q0);
void scaleQ(Quaternion *q0, double s);
void mul(Quaternion *q0, Quaternion q1, Quaternion q2);
void add(Quaternion q0, Quaternion q1, Quaternion q2);
void fromBetweenVectors(Quaternion *q, Vector3d u, Vector3d v);
void mulQ(Quaternion *r, Quaternion a, Quaternion b);
void rotateVector(Vector3d *r, Quaternion q, Vector3d v);
void identityQ(Quaternion *q);


#endif /* QUATERNION_H_ */
