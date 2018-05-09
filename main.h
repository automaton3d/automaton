/*
 * main.h
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <windows.h>

#include "brick.h"

extern boolean stop;
extern Brick *pri0, *dual0;
extern char background;
extern char gridcolor;
extern boolean showOrgs;

BITMAPINFO bmInfo;
HDC dc;
HBITMAP myBitmap;
HDC hdc;

#endif /* MAIN_H_ */
