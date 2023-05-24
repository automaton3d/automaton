/*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "plot3d.h"

#define true 	1
#define false 	0

#define ESC		27

/// Functions ///

void scale3d(float *t, int s);
void cross3d(float v1[3], float v2[3], float *v3);
void normalize(float *v);
void add3d(float *a, float b[3]);
void sub3d(float *a, float b[3]);
void line3d(float t1[3], float t2[3], char color);

#endif /* UTILS_H_ */
