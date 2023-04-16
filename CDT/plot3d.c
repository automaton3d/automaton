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
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include "engine.h"
#include "common.h"
#include "vec3.h"
#include "plot3d.h"
#include "mouse.h"
#include "text.h"
#include "main3d.h"
#include "gadget.h"
#include "simulation.h"
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
extern boolean ticks[NTICKS];
extern Cell *latt0;

boolean showAxes  = true;
boolean showModel = true;
boolean showGrid  = false;
unsigned long timer = 0;
unsigned long begin;

// Colors

char gridcolor;
char X, Y, Z;

// Markers

Vec3 *marks;
int nmark;

int yy[WIDTH];

boolean rebuild = true;

extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;
//extern int a;

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
	marks = malloc(TRACEBUF * sizeof(Vec3));
    clearBuffer();
    char *s;
	asprintf((char **)&s, "Wait, please...");
    vprints(350, 380, s);
    //
	SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
	BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
}

/*
 * Add a point to trajectory.
 */
void addPoint(Vec3 p)
{
	if(nmark < TRACEBUF)
		marks[nmark++] = p;
}

void addPoint2d(int x, int y)
{
	assert(y > 0);
	yy[x] = y;
}

void drawGroundPlane()
{
	if(!ticks[PLANE])
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

void drawMark(Vec3 p)
{
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

int driftx = 0;
int drifty = 0;
int driftz = 0;

#define BLOB
#ifdef BLOB

void putBlob(Vec3 xyz, int color)
{
	putVoxel(xyz, color);
	xyz.x++;
	putVoxel(xyz, color);
	xyz.y++;
	putVoxel(xyz, color);
	xyz.z++;
	putVoxel(xyz, color);
	xyz.x -= 2;
	putVoxel(xyz, color);
	xyz.y -= 2;
	putVoxel(xyz, color);
	xyz.z -= 2;
	putVoxel(xyz, color);
}

void drawCell(Tuple *t0, Tuple *t, Cell *cell)
{
	Vec3 xyz;
	xyz.x = WIDE * (SIDE * (t0->x + driftx) + t->x - DRIFT);
	xyz.y = WIDE * (SIDE * (t0->y + drifty) + t->y - DRIFT);
	xyz.z = WIDE * (SIDE * (t0->z + driftz) + t->z - DRIFT);
	if(ticks[MESSENGER] && cell->f)
		putBlob(xyz, RED);
	else if(ticks[MOMENTUM] && !ZERO(cell->p) && BUSY(cell))
		putBlob(xyz, CYAN);
	else if(ticks[FRONT] && BUSY(cell))
		putBlob(xyz, YELLOW);
	else if(showGrid)
		putBlob(xyz, PALE);
	else
		putBlob(xyz, BLK);
}

#else

void drawCell(Tuple *t0, Tuple *t, Cell *cell)
{
	Vec3 xyz;
	xyz.x = WIDE * (SIDE * (t0->x + driftx) + t->x - DRIFT);
	xyz.y = WIDE * (SIDE * (t0->y + drifty) + t->y - DRIFT);
	xyz.z = WIDE * (SIDE * (t0->z + driftz) + t->z - DRIFT);
	if(ticks[MESSENGER] && cell->f)
		putVoxel(xyz, RED);
	else if(ticks[MOMENTUM] && !ZERO(cell->p))
		putVoxel(xyz, CYAN);
	else if(ticks[FRONT] && BUSY(cell))
		putVoxel(xyz, YELLOW);
	else if(showGrid)
		putVoxel(xyz, PALE);
	else
		putVoxel(xyz, BLK);
}

#endif

void drawEspacito(Tuple *t0, Cell *esp)
{
	Tuple t;
	for(t.z = 0; t.z < SIDE; t.z++)
		for(t.y = 0; t.y < SIDE; t.y++)
			for(t.x = 0; t.x < SIDE; t.x++)
			{
				if(ticks[MODE0])
					drawCell(t0, &t, esp);
				else if(ticks[MODE1] && esp->a == 0)//a)
					drawCell(t0, &t, esp);
				else if(ticks[MODE2] && t.x < 2 && t.y < 2 && t.z < 2)
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
}

void drawBox()
{
	if(ticks[CUBE])
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
	showCheckbox(20,  80, "wavefront");
	showCheckbox(20, 100, "messengers");
	showCheckbox(20, 120, "spin");
	showCheckbox(20, 140, "momentum");
	showCheckbox(20, 160, "mode 0");
	showCheckbox(20, 180, "mode 1");
	showCheckbox(20, 200, "mode 2");
	showCheckbox(20, 220, "plane");
	showCheckbox(20, 240, "box");
	//
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
    	}
    	else
    	{
        	pthread_mutex_unlock(&mutex);
            Sleep(80);
    	}
    	//
    	// Update elapsed time display
    	//
     	char *s = NULL;
     	unsigned long millis = GetTickCount64() - begin;
     	asprintf(&s, "Elapsed %.1fs ", millis / 1000.0);
        vprints(20, 40, s);
    	SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
    	BitBlt(hdc, 20, 35, 160, 20, myCompatibleDC, 20, 35, SRCCOPY);
    }
}

