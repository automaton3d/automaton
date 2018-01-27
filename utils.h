//*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <windows.h>
#include "vector3d.h"

extern double radius;

void pick(Vector3d *p, LPARAM lparam);
double distance3d(Vector3d v, Vector3d b);
int opposite(int dir);
int rndSignal();
unsigned rndCoord();
int signum(int a);

#endif /* UTILS_H_ */
