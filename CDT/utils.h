/*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <windows.h>
#include "tuple.h"
#include "vec3.h"

/// Functions ///

void delay(unsigned int mseconds);
double distance3d(Vec3 v, Vec3 b);
int opposite(int dir);
int rndSign();
int signum(int a);
double sign(double d);
boolean pwm(int n);
unsigned sqr(unsigned long n);
Tuple toTuple(Vec3 v);
Vec3 rndVector();
Vec3 toVec(Tuple t);
double rnd();
void rndInt(int *array, int max);
Vec3 gravxpreon(Vec3 p0, Vec3 p1, Vec3 cen, double r);
int string_length(char *s);
int DOT(int *u, int *v);
int MOD2(int *v);

#endif /* UTILS_H_ */
