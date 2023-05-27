/*
 * main3d.h
 *
 *  Created on: 25 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef MAIN3D_H_
#define MAIN3D_H_

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>

#include "graphics.h"
#include "simulation.h"

#define WIDTH   800
#define HEIGHT  800

#define WIDE 8192
#define SEP  (WIDE/SIDE2)

// Checkboxes and radios.

#define FRONT		0
#define TRACK		1
#define MOMENTUM	2
#define PLANE		3
#define CUBE		4
#define LATTICE		5
#define AXES        6
#define MODE0		7
#define MODE1		8
#define MODE2		9

#define BMAPX   300
#define BMAPY   100

void update2d();
void putVoxel(float v[3], COLORREF color, HDC hdc);
void DeleteAutomaton();
void projLine(HDC hdc, float point1[3], float point2[3]);

#endif /* MAIN3D_H_ */
