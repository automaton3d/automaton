/*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <windows.h>

#include "brick.h"
#include "vector3d.h"

extern double radius;
extern int prime;

void pick(Vector3d *p, LPARAM lparam);
double distance3d(Vector3d v, Vector3d b);
int opposite(int dir);
int rndSignal();
unsigned rndCoord();
int signum(int a);
void resetDFO(Brick *t);
void incrDFO(Brick *t);
boolean pwm(int n);
boolean voronoi(Vector3d probe, Tuple cell);
int hash(int n);
int getPrime(unsigned n);

#endif /* UTILS_H_ */
