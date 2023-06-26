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
#define RAND        10
#define XY_VIEW     11
#define YZ_VIEW     12
#define ZX_VIEW     13
#define ISO_VIEW    14

#define BMAPX   300
#define BMAPY   100

void drawGUI();
void DeleteAutomaton();
DWORD WINAPI SimulateThread(LPVOID lpParam);

#endif /* MAIN3D_H_ */