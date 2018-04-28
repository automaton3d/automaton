/*
 * mouse.h
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include "vector3d.h"

extern Vector3d position, _position;
extern Vector3d direction, _direction;
extern double radius;

/// Functions ///

int mouse(char op, int x, int y);

#endif /* MOUSE_H_ */
