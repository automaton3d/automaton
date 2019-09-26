/*
 * main3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef MAIN3D_H_
#define MAIN3D_H_

#include <windows.h>
#include <pthread.h>
#include "quaternion.h"
#include "vector3d.h"

// Exported variables

extern boolean stop;
extern DWORD* pixels;

// Global variables

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HDC hdc;
pthread_t loop, display;
Vector3d p0, p1;
Quaternion q, qstart;

#endif /* MAIN3D_H_ */
