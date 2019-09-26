/*
 * quaternion.h
 *
 *  Created on: 7 de mai de 2017
 *      Author: Alexandre
 */

#ifndef QUATERNION_H_
#define QUATERNION_H_

typedef struct
{
	double x, y, z, w;

} Quaternion;

/// Functions ///

void conjugate(Quaternion q0, Quaternion q1);
void normalise(Quaternion q0);
void scaleQ(Quaternion q0, double s);
void mul(Quaternion *q0, Quaternion q1, Quaternion q2);
void add(Quaternion q0, Quaternion q1, Quaternion q2);

#endif /* QUATERNION_H_ */
