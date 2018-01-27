/*
 * plot3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include <pthread.h>
#include "common.h"
#include "tile.h"
#include "vector3d.h"

#define GRID 20

#define WHITEBG  0x00ffffff
#define BLACKBG  0x00000000

extern Vector3d position;      	// view reference point
extern Vector3d direction; 		// camera axis
extern Vector3d attitude;  		// view-up direction

extern boolean showAxes, showGrid;

extern DWORD *pixels;
extern unsigned long begin;
extern Tile *pri0;
extern pthread_mutex_t mutex;
extern boolean input_changed;

/// Functions ///

void initPlot();
void updatePlot();
void *DisplayLoop();
void flipMode();
void flipBox();

#endif /* PLOT3D_H_ */
