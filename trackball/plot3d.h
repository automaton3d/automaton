/*
 * plot3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#pragma once

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include "engine.h"
#include "simulation.h"

#define WINDOW 100

#define WHITEBG 0x00ffffff
#define BLACKBG 0x00000000

#define WIDTH  800
#define HEIGHT 800

// Offset of 3d bitmap

#define BMAPX   300
#define BMAPY   100

#define DEV         12
#define DISTANCE    312
#define TRACEBUF    50000

#define LIMITX		0.7
#define LIMITY		0.7

/// Functions ///

void *DisplayLoop();
void mouse(UINT msg, WPARAM wparam, LPARAM lparam);
void rotateVectorX(float *rotation, float *object, float *rotated);
void drawModel();
void initPlot();
void keyboard(UINT msg, WPARAM wparam, LPARAM lparam);

#endif /* PLOT3D_H_ */
