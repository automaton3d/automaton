/*
 * mouse.h
 *
 *  Created on: 24 de jul de 2017
 *      Author: Alexandre
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include <windows.h>
#include "plot3d.h"

typedef struct
{
	int width, height;
	int lastx, lasty, lastt;
	int dx, dy;
    float quat[4];
    float rot_axis[4];

} View;

/// Functions ///

void mouse(UINT msg, WPARAM wparam, LPARAM lparam);

#endif /* MOUSE_H_ */
