/*
 * plot3d.c
 *
 * Created on: 4 de mar de 2017
 * Author: Alexandre
 * Beep(1000, 100);
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

char voxels[SIDE6];

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
	float p[3];
	for(int i = 0; i < SIDE6; i++)
	{
		if (voxels[i])
		{
			int n = i / SIDE3;
			int x0 = n % SIDE;
			int y0 = (n / SIDE) % SIDE;
			int z0 = (n / SIDE2) % SIDE;

			int m = (i % SIDE3);
			int x = m % SIDE;
			int y = (m / SIDE) % SIDE;
			int z = (m / SIDE2) % SIDE;
			p[0] = (x + SIDE * x0) * SEP;
			p[1] = (y + SIDE * y0) * SEP;
			p[2] = (z + SIDE * z0) * SEP;
			putVoxel(p, voxels[i]);
		}
	}

	float t1[3], t2[3];

#ifdef RAYS
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
#endif

#define AXES
#ifdef AXES

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

#endif

//#define CUBE 1
#ifdef CUBE

	// Cube

	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = -WIDE;
	t2[2] = -WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = +WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = -WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = +WIDE;
	t2[2] = -WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = -WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = -WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = +WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = -WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = -WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = +WIDE;
	t2[0] = -WIDE;
	t2[1] = -WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = +WIDE;
	t1[2] = +WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = +WIDE;
	t2[0] = +WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = -WIDE;
	t1[2] = +WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = +WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = -WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = -WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = +WIDE;
	t2[2] = -WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = -WIDE;
	t1[1] = +WIDE;
	t1[2] = -WIDE;
	t2[0] = -WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);
	t1[0] = +WIDE;
	t1[1] = +WIDE;
	t1[2] = -WIDE;
	t2[0] = +WIDE;
	t2[1] = +WIDE;
	t2[2] = +WIDE;
	line3d(t1, t2, GREEN);

#endif
}
