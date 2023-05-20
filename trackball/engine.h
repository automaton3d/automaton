/*
 * engine.h
 *
 *  Created on: 24 de jan de 2021
 *      Author: Alexandre
 */

#ifndef ENGINE_H_
#define ENGINE_H_

#include <assert.h>
#include <math.h>
#include "utils.h"

#define ROUNDOFF  	1e-15
#define VERYLARGE 	1e+15
#define WIDTH   800
#define HEIGHT  800

#define IMGBUFFER	(1<<24)

// Colors

#define BLK		0
#define GRAY	1
#define RED 	2
#define BLUE	3
#define GREEN	4
#define NAVY	5

#define NCOLORS	6

typedef struct
{
	float normal[3];
	double distance;

} Plane;

typedef struct
{
	Plane sides[4];
	Plane znear;

} Frustum;

typedef struct
{
	float pos[3];
	char color;

} Voxel;

///// Functions /////

void clearBuffer();
void newView2();
void newView3();
void putVoxel(float v[3], char color);
void flipBuffers();
void newProjection();
void initEngine(double zoom);
void setWindow(double _wxl, double _wxh, double _wyl, double _wyh);
void setBackground(char color);
void update3d();

#endif /* ENGINE_H_ */
