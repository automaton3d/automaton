/*
 * plot3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 *
 *      Beep(1000, 100);
 * Implements a 3d graphics pipeline.
 * This fast and simple engine is only capable of projecting isolated points.
 */

#include "plot3d.h"

#define true 1

// Global variables

extern BITMAPINFO bmInfo;
extern HDC myCompatibleDC;
extern HBITMAP myBitmap;
extern HDC hdc;

extern HWND g_hBitmap;
extern HDC hdc;

extern DWORD *pixels;

boolean showAxes  = true;
unsigned long timer = 0;
unsigned long begin;
char gridcolor;
boolean rebuild = true;
Trackball view;

void initPlot()
{
  initScreen();
  initEngine(DISTANCE);
  //
  setBackground(BLK);
  gridcolor = NAVY;
  //
  clearBuffer();
  //
  SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
  BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
  SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
}

void drawModel()
{
	float t1[3], t2[3];
	for(int i = 0; i < 100; i++)
	{
		t1[0] = 0;
		t1[1] = 0;
		t1[2] = 0;
		t2[0] = rand() % 1500;
		t2[1] = rand() % 1500;
		t2[2] = rand() % 1500;
		line3d(t1, t2, GRAY);
	}
	t1[0] = -2500;
	t1[1] = 0;
	t1[2] = 0;
	t2[0] = +2500;
	t2[1] = 0;
	t2[2] = 0;
	line3d(t1, t2, RED);
	t1[1] = -2500;
	t1[0] = 0;
	t2[1] = +2500;
	t2[0] = 0;
	line3d(t1, t2, GREEN);
	t1[2] = -2500;
	t1[0] = 0;
	t1[1] = 0;
	t2[2] = +2500;
	t2[1] = 0;
	line3d(t1, t2, BLUE);

	// Cube

	t1[0] = -200;
	t1[1] = -200;
	t1[2] = -200;
	t2[0] = -200;
	t2[1] = 200;
	t2[2] = -200;
	line3d(t1, t2, NAVY);

	t1[0] = -200;
	t1[1] = 200;
	t1[2] = -200;
	t2[0] = +200;
	t2[1] = +200;
	t2[2] = -200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = +200;
	t1[2] = -200;
	t2[0] = +200;
	t2[1] = -200;
	t2[2] = -200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = -200;
	t1[2] = -200;
	t2[0] = -200;
	t2[1] = -200;
	t2[2] = -200;
	line3d(t1, t2, NAVY);
	t1[0] = -200;
	t1[1] = -200;
	t1[2] = -200;
	t2[0] = -200;
	t2[1] = -200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = -200;
	t1[1] = -200;
	t1[2] = +200;
	t2[0] = -200;
	t2[1] = +200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = -200;
	t1[1] = +200;
	t1[2] = +200;
	t2[0] = +200;
	t2[1] = +200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = +-200;
	t1[2] = +200;
	t2[0] = +200;
	t2[1] = -200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = -200;
	t1[2] = +200;
	t2[0] = -200;
	t2[1] = -200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = -200;
	t1[1] = +200;
	t1[2] = -200;
	t2[0] = -200;
	t2[1] = +200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = +200;
	t1[2] = +200;
	t2[0] = +200;
	t2[1] = +-200;
	t2[2] = -200;
	line3d(t1, t2, NAVY);
	t1[0] = +200;
	t1[1] = -200;
	t1[2] = -200;
	t2[0] = +200;
	t2[1] = -200;
	t2[2] = +200;
	line3d(t1, t2, NAVY);
}
