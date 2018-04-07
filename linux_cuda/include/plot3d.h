/*
 * plot3d.h
 */

#ifndef PLOT3D_H_
#define PLOT3D_H_

#include <sys/time.h>
#include "brick.h"
#include "common.h"
#include "vector3d.h"
#include "automaton.h"
#include "jpeglib.h"

#define GRID		15

#define WHITEBG		0x00ffffff
#define BLACKBG		0x00000000

#define PARALLEL	0
#define PERSPECTIVE	1
#define ROUNDOFF	1e-15
#define VERYLARGE	1e+15

extern Vector3d position;      		// view reference point
extern Vector3d direction; 		// camera axis
extern Vector3d attitude;  		// view-up direction

extern int showAxes, showGrid;
extern struct timeval begin;
extern Brick *pri0, *dual0;
extern char *voxels, *d_voxels;
extern JSAMPLE *image_buffer;
extern int input_changed, automaton_changed;
extern unsigned long timer;
extern pthread_mutex_t cam_mutex;
extern int scenario;
extern const char *scenarios[];
extern boolean splash;

/// Functions ///

void initPlot();
void updatePlot();
void flipMode();
void flipBox();
void *DisplayLoop(void *v);
void endPlot();


#endif /* PLOT3D_H_ */

