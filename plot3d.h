/*
 * plot3d.h
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

extern Vector3d position;      	// view reference point
extern Vector3d direction; 		// camera axis
extern Vector3d attitude;  		// view-up direction

extern boolean showAxes, showGrid;

extern DWORD *pixels;
extern unsigned long begin;
extern Brick *pri0;
extern pthread_mutex_t mutex;
extern boolean input_changed;
extern boolean img_changed;
extern int scenario;
extern const char *scenarios[];
extern boolean splash;

/// Functions ///

void initPlot();
void clearBuffer();
void updatePlot();
void *DisplayLoop();
void flipMode();
void flipBox();
void drawChar(double x, double y, double z, char color, char ch);
void addMarker(Tuple xyz);

#endif /* PLOT3D_H_ */
