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
#include "text.h"

#define WINDOW 100
#define ROUNDOFF  	1e-15
#define VERYLARGE 	1e+15
#define WIDTH   800
#define HEIGHT  800

#define IMGBUFFER	(1<<24)

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

void initEngine(double zoom);
void flipMode();
void setBackground(char color);
void setWindow(double _wxl, double _wxh, double _wyl, double _wyh);
void setViewPort(double _vxl, double _vxh, double _vyl, double _vyh);
void setPerspective(double x, double y, double z);
void setViewDistance(double _distance);
void newView2();
void newView3();
void newProjection();
void clearBuffer();
void plot(float v[3], char color);
void putVoxel(float v[3], char color);
void line2d(int x0, int y0, int x1, int y1, int color);
void drawChar(double x, double y, double z, char color, char ch);
void setCamera(float position[3], float direction[3], float attitude[3]);
void getCamera(float *p, float *d, float *a);
void zoom(int delta);
void panH(int offset);
void panV(int offset);
void putPixel(int x, int y, int color);
void update3d();
void flipBuffers();
void point2d(int x, int y, int color);
void shrinkWindow();
void expandWindow();
boolean isParallel();
boolean isParallel();
char getBackground();
float *getPerspective();

#endif /* ENGINE_H_ */
