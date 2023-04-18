/*
 * engine.h
 *
 *  Created on: 24 de jan de 2021
 *      Author: Alexandre
 */

#ifndef ENGINE_H_
#define ENGINE_H_

#include <windows.h>

#include "vec3.h"

#define WINDOW 100
#define ROUNDOFF  	1e-15
#define VERYLARGE 	1e+15
#define WIDTH   800
#define HEIGHT  800

#define IMGBUFFER	(1<<24)

typedef struct
{
	Vec3 normal;
	double distance;

} Plane;

typedef struct
{
	Vec3 p1, p2;
	char color;

} Line;

typedef struct
{
	Plane sides[4];
	Plane znear;

} Frustum;

typedef struct
{
	Vec3 pos;
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
void plot(Vec3 v, char color);
void putVoxel(Vec3 v, char color);
void line2d(int x0, int y0, int x1, int y1, int color);
void drawChar(double x, double y, double z, char color, char ch);
void setCamera(Vec3 position, Vec3 direction, Vec3 attitude);
void getCamera(Vec3 *position, Vec3 *direction, Vec3 *attitude);
void zoom(int delta);
void panH(int offset);
void panV(int offset);
void putPixel(int x, int y, int color);
void update3d();
void flipBuffers();
boolean isParallel();
char getBackground();
Vec3 arcball(Vec3 points, Vec3 axis, double angle);
void point2d(int x, int y, int color);
Vec3 getPerspective();
boolean isParallel();
void shrinkWindow();
void expandWindow();


#endif /* ENGINE_H_ */
