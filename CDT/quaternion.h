/*
 * quaternion.h
 *
 *  Created on: 7 de mai de 2017
 *      Author: Alexandre
 */

#ifndef QUATERNION_H_
#define QUATERNION_H_

#include <math.h>

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
void fromBetweenVectors(Quaternion *q, float *u, float *v);
void mulQ(Quaternion *r, Quaternion a, Quaternion b);
void rotateVector(float *r, Quaternion q, float v[3]);
void identityQ(Quaternion *q);


#endif /* QUATERNION_H_ */
