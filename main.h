/*
 * main3d.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef MAIN3D_H_
#define MAIN3D_H_

#include <windows.h>
#include "tile.h"

extern boolean stop;
extern Tile *pri0, *dual0;
extern char background;
extern char gridcolor;

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HDC hdc;

#endif /* MAIN3D_H_ */
