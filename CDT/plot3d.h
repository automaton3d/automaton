/*
 * plot3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include <windows.h>
#include <pthread.h>
#include "common.h"
#include "tuple.h"
#include "simulation.h"
#include "vec3.h"

#define WHITEBG  0x00ffffff
#define BLACKBG  0x00000000

#define MESSENGER	0
#define SPIN		1
#define PLANE		2
#define CUBE		3
#define TRAJECTORY	4
#define MOMENTUM	5
#define MODEL		6
#define CENTERS		7

#define NTICKS		8

#define WIDE        (512/SIDE2)
#define DRIFT       ((SIDE2+SIDE)/2)
#define BOXMIN      (-WIDE * DRIFT)
#define BOXMAX      (WIDE * SIDE2 - DRIFT - 1)

#define DEV         12
#define DISTANCE    312
#define TRACEBUF    50000

#define LIMITX		0.7
#define LIMITY		0.7

// Exported variables

extern Vec3 position;	      	// view reference point
extern Vec3 direction; 			// camera axis
extern Vec3 attitude;  			// view-up direction
extern boolean showAxes, showGrid;
extern pthread_mutex_t mutex;
extern char gridcolor;
extern unsigned long timer;
extern unsigned long begin;

/// Functions ///

void initPlot();
void *DisplayLoop();
void addPoint(Vec3 p);
void voxelize();

#endif /* PLOT3D_H_ */
