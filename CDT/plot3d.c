/*
 * plot3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 *
 *    	Beep(1000, 100);
 * Implements a 3d graphics pipeline.
 * This fast and simple engine is only capable of projecting isolated points.
 */

#define _GNU_SOURCE
#define PTW32_STATIC_LIB

#include <windows.h>
#include <assert.h>

#include "engine.h"
#include "plot3d.h"
#include "mouse.h"
#include "text.h"
#include "main3d.h"
#include "bresenham.h"
#include "utils.h"
#include "test/test.h"

// Global variables

extern BITMAPINFO bmInfo;
extern HDC myCompatibleDC;
extern HBITMAP myBitmap;
extern HDC hdc;

// Constants
//
#define PARALLEL    0
#define PERSPECTIVE 1

extern boolean input_changed;
extern Cell *latt0;
extern HWND g_hBitmap;
extern HDC hdc;
extern HWND mode0_rad, mode1_rad, mode2_rad;

boolean showAxes  = true;
boolean showModel = true;
boolean showGrid  = false;
unsigned long timer = 0;
unsigned long begin;

// Colors

char gridcolor;
char X, Y, Z;

// Markers

Cell *marks;
int nmark;

int yy[WIDTH];

boolean rebuild = true;

int driftx = 0;
int drifty = 0;
int driftz = 0;

boolean momentum = true, wavefront = true, mode0, mode1, mode2, track, cube, plane;

extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;
extern HWND front_chk, track_chk, p_chk, plane_chk, cube_chk;

View view;

Vec3 t1[] =
{
	{ BOXMIN, BOXMIN, BOXMIN },
	{ BOXMIN, BOXMIN, BOXMIN },
	{ BOXMIN, BOXMIN, BOXMIN },

	{ BOXMAX, BOXMIN, BOXMIN },
	{ BOXMAX, BOXMIN, BOXMIN },

	{ BOXMIN, BOXMAX, BOXMIN },
	{ BOXMIN, BOXMAX, BOXMIN },

	{ BOXMIN, BOXMIN, BOXMAX },
	{ BOXMIN, BOXMIN, BOXMAX },

	{ BOXMAX, BOXMAX, BOXMAX },
	{ BOXMAX, BOXMAX, BOXMAX },
	{ BOXMAX, BOXMAX, BOXMAX },
};

Vec3 t2[] =
{
	{ BOXMAX, BOXMIN, BOXMIN },
	{ BOXMIN, BOXMAX, BOXMIN },
	{ BOXMIN, BOXMIN, BOXMAX },

	{ BOXMAX, BOXMAX, BOXMIN },
	{ BOXMAX, BOXMIN, BOXMAX },

	{ BOXMAX, BOXMAX, BOXMIN },
	{ BOXMIN, BOXMAX, BOXMAX },

	{ BOXMAX, BOXMIN, BOXMAX },
	{ BOXMIN, BOXMAX, BOXMAX },

	{ BOXMIN, BOXMAX, BOXMAX },
	{ BOXMAX, BOXMIN, BOXMAX },
	{ BOXMAX, BOXMAX, BOXMIN },
};

void initPlot()
{
	initScreen();
	initEngine(DISTANCE);
	//
    X = RED;
    Y = LIME;
    Z = CYAN;
    //
    setBackground(BLK);
    gridcolor = NAVY;
    //
	marks = malloc(TRACEBUF * sizeof(Cell));
    clearBuffer();
    char *s;
	asprintf((char **)&s, "Wait, please...");
    vprints(350, 380, s);
    //
	SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
	BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
    SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
}

void addCell(Cell *c)
{
	if(nmark < TRACEBUF)
		marks[nmark++] = *c;
}

void addPoint2d(int x, int y)
{
	assert(y > 0);
	yy[x] = y;
}

void drawGroundPlane()
{
	LRESULT result = SendMessage(WindowFromDC(hdc), BM_GETCHECK, PLANE, 0);
	if (result == BST_UNCHECKED)
		return;
	Vec3 p1, p2;
	p1.x = -1000;
	p1.y = -1000;
	p1.z = 0;
	p2.x = +1000;
	p2.y = -1000;
	p2.z = 0;
	for(int i = 0; i < 100; i++)
	{
		line3d(p1, p2, gridcolor);
		p1.y += 20; p2.y += 20;
	}
	p1.x = -1000;
	p1.y = -1000;
	p2.x = -1000;
	p2.y = +1000;
	for(int i = 0; i < 100; i++)
	{
		line3d(p1, p2, gridcolor);
		p1.x += 20; p2.x += 20;
	}
	p1.x = 0;
	p1.y = 0;
	p2.x = 0;
	p2.y = 0;
	p2.z = 500;
	line3d(p1, p2, YELLOW);
}

void drawMark(Cell cell)
{
	if(cell.oE > 0)
		return;
	int x0 = cell.oL % SIDE;
	int z0 = cell.oL / SIDE2;
	int y0 = cell.oL / SIDE - SIDE * z0;

	int x = cell.oE % SIDE;
	int z = cell.oE / SIDE2;
	int y = cell.oE / SIDE - SIDE * z;
	Vec3 p;
	p.x = WIDE * (SIDE * (x0 + driftx) + x - DRIFT);
	p.y = WIDE * (SIDE * (y0 + drifty) + y - DRIFT);
	p.z = WIDE * (SIDE * (z0 + driftz) + z - DRIFT);
	Vec3 t1, t2;
	t1 = p;
	t2 = p;
	t1.x += DEV;
	t2.x -= DEV;
	line3d(t1, t2, ORANGE);
	t1 = p;
	t2 = p;
	t1.y += DEV;
	t2.y -= DEV;
	line3d(t1, t2, ORANGE);
	t1 = p;
	t2 = p;
	t1.z += DEV;
	t2.z -= DEV;
	line3d(t1, t2, ORANGE);
}

void drawMarks()
{
	Cell *ms = marks;
	for(int i = 0; i < nmark; i++)
	{
		drawMark(*ms);
		ms++;
	}
}

void putBlob(Vec3 xyz, int color)
{
	#define BLOB
	#ifdef BLOB
	putVoxel(xyz, color);
	xyz.x++;
	putVoxel(xyz, color);
	xyz.x -= 2;
	putVoxel(xyz, color);
	xyz.x++;
	xyz.y++;
	putVoxel(xyz, color);
	xyz.y -= 2;
	putVoxel(xyz, color);
	xyz.y++;
	xyz.z++;
	putVoxel(xyz, color);
	xyz.z -= 2;
	putVoxel(xyz, color);
	#else
	putVoxel(xyz, color);
	#endif
}

void drawCell(Tuple *t0, Tuple *t, Cell *cell)
{
	Vec3 xyz;
	xyz.x = WIDE * (SIDE * (t0->x + driftx) + t->x - DRIFT);
	xyz.y = WIDE * (SIDE * (t0->y + drifty) + t->y - DRIFT);
	xyz.z = WIDE * (SIDE * (t0->z + driftz) + t->z - DRIFT);

	if(momentum && !ZERO(cell->p) && BUSY(cell))
		putBlob(xyz, CYAN);
	else if(wavefront && BUSY(cell) && cell->oE==0)
		putBlob(xyz, YELLOW);
	else if(showGrid)
		putBlob(xyz, PALE);
	else
		putBlob(xyz, BLK);
}

void drawEspacito(Tuple *t0, Cell *esp)
{
	Tuple t;
	for(t.z = 0; t.z < SIDE; t.z++)
		for(t.y = 0; t.y < SIDE; t.y++)
			for(t.x = 0; t.x < SIDE; t.x++)
			{
				if(mode0)
					drawCell(t0, &t, esp);
				else if(mode1 && esp->oE == 0)//a)
					drawCell(t0, &t, esp);
				else if(mode2 && t.x < 2 && t.y < 2 && t.z < 2)
					drawCell(t0, &t, esp);
				esp++;
			}
}

int visit = 0;	// TEST_TREE

void drawModel()
{
#ifdef TEST_TREE

  visit = 0;
  int root[3];
  root[0] = 0;
  root[1] = 0;
  root[2] = 0;
  explore(root, 0);
//  assert(visit == (2 * SIDE - 2) * (2 * SIDE - 2) * (2 * SIDE - 2));
  printf("visit=%d\n", visit);

#else

  wavefront = SendMessage(front_chk, BM_GETCHECK, FRONT, 0);
  track     = SendMessage(track_chk, BM_GETCHECK, TRACK, 0);
  momentum  = SendMessage(p_chk,     BM_GETCHECK, MOMENTUM, 0);
  plane     = SendMessage(plane_chk, BM_GETCHECK, PLANE, 0);
  cube      = SendMessage(cube_chk,  BM_GETCHECK, CUBE, 0);
  mode0     = SendMessage(mode0_rad, BM_GETCHECK, MODE0, 0);
  mode1     = SendMessage(mode1_rad, BM_GETCHECK, MODE1, 0);
  mode2     = SendMessage(mode2_rad, BM_GETCHECK, MODE2, 0);

  Tuple t;
  Cell *espacito = latt0;
  for(t.z = 0; t.z < SIDE; t.z++)
    for(t.y = 0; t.y < SIDE; t.y++)
      for(t.x = 0; t.x < SIDE; t.x++)
      {
    	  drawEspacito(&t, espacito);
		  espacito += SIDE3;
	  }

#endif
  if (track)
	  drawMarks();
}

void drawBox()
{
	if(cube)
		for(int i = 0; i < 12; i++)
			line3d(t1[i], t2[i], BOX);
}

void drawAxes()
{
	if(!showAxes)
		return;
	//
	// Save status
	//
	Vec3 position, direction, attitude;
	getCamera(&position, &direction, &attitude);
	//
	// Prepare matrix
	//
	Vec3 tpos = direction;
	normalize(&tpos);
	scale3d(&tpos, -30);
	setViewPort(0.0, 0.2, 0.0, 0.2);
	setWindow(-40, 40, -40, 40);
	setCamera(tpos, direction, attitude);
	newView2();
	newView3();
	//
	Vec3 point;
	for(int i = 0; i < 35; i++)
	{
		double p = i;
		if(i < 30)
		{
			point.x = p; point.y = 0; point.z = 0;
			plot(point, X);
			point.x = p; point.y = 0.3; point.z = 0;
			plot(point, X);
			point.x = p; point.y = -0.3; point.z = 0;
			plot(point, X);
			point.x = p; point.y = 0; point.z = 0.3;
			plot(point, X);
			point.x = p; point.y = 0; point.z = -0.3;
			plot(point, X);
			point.x = 0; point.y = p; point.z = 0;
			plot(point, Y);
			point.x = 0.3; point.y = p; point.z = 0;
			plot(point, Y);
			point.x = -0.3; point.y = p; point.z = 0;
			plot(point, Y);
			point.x = 0; point.y = p; point.z = 0.3;
			plot(point, Y);
			point.x = 0; point.y = p; point.z = -0.3;
			plot(point, Y);
			point.x = 0; point.y = 0; point.z = p;
			plot(point, Z);
			point.x = 0; point.y = 0.3; point.z = p;
			plot(point, Z);
			point.x = 0; point.y = -0.3; point.z = p;
			plot(point, Z);
			point.x = 0.3; point.y = 0; point.z = p;
			plot(point, Z);
			point.x = -0.3; point.y = 0; point.z = p;
			plot(point, Z);
		}
		else if(i == 34)
		{
			drawChar(p, 0, 0, X, 'x');
			drawChar(0, p, 0, Y, 'y');
			drawChar(0, -0.3, p, Z, 'z');
		}
	}
	//
	// Restore standard configuration
	//
	setViewPort(0.0, 1.0, 0.0, 1.0);
	setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
	setCamera(position, direction, attitude);
}

void drawVectors()
{
	Vec3 p0;
	reset3d(&p0);
	Vec3 position, direction, attitude;
	getCamera(&position, &direction, &attitude);
	Vec3 di, at;
	at.x = 0; at.y = 0; at.z = -1;
	di.x = 1; di.y = 0; di.z = 0;
	setCamera(p0, di, at);
//	newView2();
//	newView3();
	setCamera(position, direction, attitude);
}

void voxelize()
{
	if(showModel)
		drawModel();
}

void update2d()
{
 	char *s = NULL;
    //
 	asprintf(&s, "Automaton %d", SIDE);
    vprints(20, 20, s);
    //
    asprintf(&s, "light: %lu tick: %lu", timer / LIGHT, timer);
	vprints(20, 60, s);
    //
	asprintf(&s, "S: pause/resume");
    vprints(650, 700, s);
    //
    asprintf(&s, "G: grid on-off");
    vprints(650, 720, s);
    //
    asprintf(&s, "X: box on-off");
    vprints(650, 740, s);
	//
    setTextColor(ORANGE);
    if(isParallel())
    	asprintf(&s, "PARALLEL");
    else
    	asprintf(&s, "PERSPECTIVE");
    vprints(350, 740, s);
    setTextColor(WHT);
    //
    if(stop)
    {
    	asprintf(&s, "[PAUSED]");
    	vprints(370, 395, s);
    }
    //
    // Camera position
    //
    Vec3 pos, dir, att;
    getCamera(&pos, &dir, &att);
    asprintf(&s, "[%.0f, %.0f, %.0f]", pos.x, pos.y, pos.z);
	int n = string_length(s);
    vprints(800-10*n, 150, s);
}

/**
 * Updates the screen continuously.
 */
void *DisplayLoop()
{
    pthread_detach(pthread_self());
	printf("Running...\n");
	//
	initPlot();
	flipBuffers();	// forces init of curr
 	begin = GetTickCount64();
    pthread_barrier_wait(&barrier);
    while(true)
    {
    	pthread_mutex_lock(&mutex);
    	if(input_changed || rebuild)
    	{
        	if(rebuild && showModel)
        	{
        		drawModel();
        		flipBuffers();
        	}
        	//
       		// Voxels to pixels
       		//
       		clearBuffer();
       		setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
      		newView2();
       		newProjection();
       		newView3();
       		update3d();
       		//
       		drawBox();
       		drawAxes();
   			drawGroundPlane();
   			drawVectors();
        	//
        	update2d();
        	//
        	// Update screen
        	//
        	SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
        	BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
        	//
    		input_changed = false;
    		rebuild = false;
        	pthread_mutex_unlock(&mutex);
            SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);	// PATCH
    	}
    	else
    	{
        	pthread_mutex_unlock(&mutex);
            Sleep(80);
    	}
    	//
    	// Update elapsed time display
    	//
    	HWND hwnd = WindowFromDC(hdc);
    	RECT rect;
        GetClientRect(hwnd, &rect);
        InvalidateRect(hwnd, &rect, TRUE);
    }
}

