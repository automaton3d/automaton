/*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <windows.h>
#include "vector3d.h"
#include "tuple.h"

/// Functions ///

void delay(unsigned int mseconds);
double distance3d(Vector3d v, Vector3d b);
int opposite(int dir);
int rndSign();
int signum(int a);
double sign(double d);
boolean pwm(int n);
unsigned sqr(unsigned long n);
Tuple toTuple(Vector3d v);
Vector3d rndVector();
Vector3d toVec(Tuple t);
double rnd();
void rndInt(int *array, int max);
Vector3d gravxpreon(Vector3d p0, Vector3d p1, Vector3d cen, double r);
int string_length(char *s);
void COPY(int *u, int *v);

#endif /* UTILS_H_ */
