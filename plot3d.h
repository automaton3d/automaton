/*
 * plot3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include <pthread.h>

#include "brick.h"
#include "common.h"
#include "vector3d.h"

#define GRID 20

#define WHITEBG  0x00ffffff
#define BLACKBG  0x00000000

// Exported variables

extern char *draft, *clean, *snap;
extern Vector3d position, _position;      	// view reference point
extern Vector3d direction, _direction; 		// camera axis
extern Vector3d attitude;  					// view-up direction
extern boolean showAxes, showGrid;
extern pthread_mutex_t mutex;
extern char background, gridcolor;
extern unsigned long timer;
extern unsigned long begin;
extern char imgbuf[3][SIDE3];
extern DWORD colors[];

/// Functions ///

void initPlot();
void updatePlot();
void *DisplayLoop();
void flipMode();
void flipBox();
void drawChar(double x, double y, double z, char color, char ch);
void line(int x0, int y0, int x1, int y1, int color);

#endif /* PLOT3D_H_ */
