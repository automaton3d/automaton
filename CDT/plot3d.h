/*
 * plot3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#pragma once

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include "simulation.h"
#include "main3d.h"
#include "bresenham.h"
#include "utils.h"

#define WHITEBG 0x00ffffff
#define BLACKBG 0x00000000

// Offset of 3d bitmap

#define BMAPX   300
#define BMAPY   100

#define WIDE        (1024/SIDE2)
#define DRIFT       ((SIDE2+SIDE)/2)

#define BOXMIN      (-WIDE*DRIFT)
#define BOXMAX      (+WIDE*(DRIFT-SIDE))

#define DEV         12
#define DISTANCE    312
#define TRACEBUF    50000

#define LIMITX		0.7
#define LIMITY		0.7

// Checkboxes

#define FRONT		0
#define TRACK		1
#define MOMENTUM	2
#define PLANE		3
#define CUBE		4
#define LATTICE		5
#define MODE0		6
#define MODE1		7
#define MODE2		8

/// Functions ///

void *DisplayLoop();
void mouse(UINT msg, WPARAM wparam, LPARAM lparam);

#endif /* PLOT3D_H_ */
