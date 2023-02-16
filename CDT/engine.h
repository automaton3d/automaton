/*
 * engine.h
 *
 *  Created on: 24 de jan de 2021
 *      Author: Alexandre
 */

#ifndef ENGINE_H_
#define ENGINE_H_

#include <windows.h>
#include "vector3d.h"

#define WINDOW 100
#define ROUNDOFF  	1e-15
#define VERYLARGE 	1e+15
#define WIDTH   800
#define HEIGHT  800

#define IMGBUFFER	(1<<24)

typedef struct
{
	Vector3d normal;
	double distance;

} Plane;

typedef struct
{
	Vector3d p1, p2;
	char color;

} Line;

typedef struct
{
	Plane sides[4];
	Plane znear;

} Frustum;

typedef struct
{
	Vector3d pos;
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
void plot(Vector3d v, char color);
void putVoxel(Vector3d v, char color);
void line2d(int x0, int y0, int x1, int y1, int color);
void drawChar(double x, double y, double z, char color, char ch);
void setCamera(Vector3d position, Vector3d direction, Vector3d attitude);
void getCamera(Vector3d *position, Vector3d *direction, Vector3d *attitude);
void zoom(int delta);
void panH(int offset);
void panV(int offset);
void putPixel(int x, int y, int color);
void update3d();
void flipBuffers();
boolean isParallel();
char getBackground();
Vector3d arcball(Vector3d points, Vector3d axis, double angle);
void point2d(int x, int y, int color);
Vector3d getPerspective();
boolean isParallel();
void shrinkWindow();
void expandWindow();


#endif /* ENGINE_H_ */
