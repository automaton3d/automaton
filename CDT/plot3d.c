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

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "engine.h"
#include "common.h"
#include "vector3d.h"
#include "plot3d.h"
#include "mouse.h"
#include "text.h"
#include "main3d.h"
#include "gadget.h"
#include "simulation.h"
#include "bresenham.h"
#include "utils.h"

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
extern Tensor *lattice0;

boolean showAxes  = true;
boolean showModel = true;

unsigned long timer = 0;
unsigned long begin;

// Colors

char gridcolor;
char X, Y, Z;

// Markers

Vector3d *marks;
int nmark;

int yy[WIDTH];

extern int i0, i1, i2, i3, i4, i5;

boolean rebuild = true;

extern pthread_mutex_t mutex;
extern pthread_barrier_t barrier;

Vector3d t1[] =
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

Vector3d t2[] =
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
	marks = malloc(TRACEBUF * sizeof(Vector3d));
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
void addPoint(Vector3d p)
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
	//
	newView2();
	newProjection();
	newView3();
	//
	Vector3d center = getPerspective();
	int x0 = floor(center.x/20)*20;
	int y0 = floor(center.y/20)*20;
	//
	// Grid
	//
	Vector3d pos;
	pos.z = 0;
	for(pos.x = x0-1000; pos.x < x0+1000; pos.x+=20)
		for(pos.y = y0-1000; pos.y < y0+1000; pos.y++)
			plot(pos, gridcolor);
	for(pos.y = y0-1000; pos.y < y0+1000; pos.y+=20)
		for(pos.x = x0-1000; pos.x < x0+1000; pos.x++)
			plot(pos, gridcolor);
	pos.x = 0; pos.y = 0;
	for(pos.z = 0; pos.z < +100; pos.z++)
		plot(pos, YELLOW);
}

void drawMark(Vector3d p)
{
	Vector3d t1, t2;
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

int driftx = SIDE / 2;
int drifty = SIDE / 2;
int driftz = SIDE / 2;

void drawEspacito(int x0, int y0, int z0, Tensor *espacito)
{
	Vector3d xyz;
	for(int z = 0; z < SIDE; z++)
		for(int y = 0; y < SIDE; y++)
			for(int x = 0; x < SIDE; x++)
			{
				xyz.x = WIDE * (SIDE * (x0 + driftx) + x - DRIFT);
				xyz.y = WIDE * (SIDE * (y0 + drifty) + y - DRIFT);
				xyz.z = WIDE * (SIDE * (z0 + driftz) + z - DRIFT);
				if(espacito->f > 0)
					putVoxel(xyz, RED);
				else if(espacito->flash)
					putVoxel(xyz, YELLOW);
				else if(SIDE <= 4)
					putVoxel(xyz, PALE);
				espacito++;
			}
}

void drawModel()
{
	Tensor *espacito = lattice0;
	for(int z = 0; z < SIDE; z++)
		for(int y = 0; y < SIDE; y++)
			for(int x = 0; x < SIDE; x++)
			{
				drawEspacito(x, y, z, espacito);
				espacito += SIDE3;
			}
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
	Vector3d position, direction, attitude;
	getCamera(&position, &direction, &attitude);
	//
	// Prepare matrix
	//
	Vector3d tpos = direction;
	normalize(&tpos);
	scale3d(&tpos, -30);
	setViewPort(0.0, 0.2, 0.0, 0.2);
	setWindow(-40, 40, -40, 40);
	setCamera(tpos, direction, attitude);
	newView2();
	newView3();
	//
	Vector3d point;
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

void voxelize()
{
	if(showModel)
		drawModel();
}

void update2d()
{
	showCheckbox(20, 120, "messengers");
	showCheckbox(20, 140, "spin");
	showCheckbox(20, 160, "plane");
	showCheckbox(20, 180, "box");
	showCheckbox(20, 200, "track");
	showCheckbox(20, 220, "momentum");
	showCheckbox(20, 240, "model");
	showCheckbox(20, 260, "centers");
	//
 	char *s;
    //
    asprintf(&s, "Automaton %d", SIDE);
    vprints(20, 20, s);
    //
 	asprintf(&s, "light: %lu tick: %lu", timer / LIGHT, timer);
	vprints(20, 60, s);
    //
    asprintf((char **)&s, "S: pause/resume");
    vprints(650, 700, s);
    //
    asprintf((char **)&s, "G: grid on-off");
    vprints(650, 720, s);
    //
    asprintf((char **)&s, "X: box on-off");
    vprints(650, 740, s);
	//
    setTextColor(ORANGE);
    if(isParallel())
    	asprintf((char **)&s, "PARALLEL");
    else
    	asprintf((char **)&s, "PERSPECTIVE");
    vprints(350, 740, s);
    setTextColor(WHT);
    //
    if(stop)
    {
    	asprintf((char **)&s, "[PAUSED]");
    	vprints(370, 395, s);
    }
    //
	asprintf((char **)&s, "msgr=%d f1xf1=%d", i0, i1);
    vprints(20, 620, s);
	asprintf((char **)&s, "strong=%d weak=%d", i2, i3);
    vprints(20, 640, s);
	asprintf((char **)&s, "elemag=%d inertia=%d", i4, i5);
    vprints(20, 660, s);
    //
    // Camera position
    //
    Vector3d pos, dir, att;
    getCamera(&pos, &dir, &att);
	asprintf((char **)&s, "[%.0f, %.0f, %.0f]", pos.x, pos.y, pos.z);
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
	fflush(stdout);
	//
	initPlot();
	flipBuffers();	// forces init of curr
 	begin = GetTickCount();
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
            usleep(80000);
    	}
    	//
    	// Update elapsed time display
    	//
     	char *s;
     	unsigned long millis = GetTickCount() - begin;
        asprintf(&s, "Elapsed %.1fs ", millis / 1000.0);
        vprints(20, 40, s);
    	SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
    	BitBlt(hdc, 20, 35, 160, 20, myCompatibleDC, 20, 35, SRCCOPY);
    }
}

