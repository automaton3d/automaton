/*
 * plot3d.c
 *
 * Implements a 3d graphics pipeline.
 * This fast and simple engine is only capable of projecting isolated points.
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "common.h"
#include "params.h"
#include "vector3d.h"
#include "plot3d.h"

#include "brick.h"
#include "main.h"
#include "text.h"

// Constants
//
#define PARALLEL    0
#define PERSPECTIVE 1

#define ROUNDOFF  	1e-15
#define VERYLARGE 	1e+15

// Transformation matrix
//
double m00, m01, m02;
double m10, m11, m12;
double m20, m21, m22;
double m30, m31, m32;
//
// 3d fields
//
Vector3d center;	      	// center of projection
Vector3d pc;         	 	// transformed center of projection
Vector3d pen;        		// plotting pen

double distance;        	// view distance
double frontDistance;		// clipping plane
double backDistance;		// clipping plane
int ucs;					// UCS icon flag
//
// Window
//
double wxl, wyl;
double wxh, wyh;
double vxl, vyl;
double vxh, vyh;
double wsx, wsy;
//
boolean parallel = true;
int clipping;
double scale = 1100;

boolean showAxes = true, showGrid = false, showBox = true;

Vector3d position, _position;      	// view reference point
Vector3d direction, _direction;		// camera axis
Vector3d attitude;   				// view-up direction
unsigned long timer = 0;
unsigned long begin;

// Rotation

double theta = 0;
BOOL rot = FALSE;

// Colors

DWORD colors[7 + NPREONS];
char background, gridcolor;
char X, Y, Z;

char imgbuf[3][SIDE3];
char *draft, *clean, *snap;

typedef struct M
{
	Tuple xyz;
	struct M *next;
} Marker;

Marker *markers;

void addMarker(Tuple xyz)
{
	if(markers == NULL)
	{
		markers = malloc(sizeof(Marker));
		markers->xyz = xyz;
	}
	else
	{
		Marker *m = markers;
		while(m->next)
			m = m->next;
		m->next = malloc(sizeof(Marker));
		m->next->xyz = xyz;
	}
}

void initPalette()
{
        X = RR;
        Y = GG;
        Z = BB;
        //
        background = BLK;
        gridcolor = GRAY;
        //
        colors[0] = 0x00000000;		// BLK
        colors[1] = 0x00ffffff;		// WHT
        colors[2] = 0x00ff0000;		// RR
        colors[3] = 0x0000ff00;		// GG
        colors[4] = 0x000000ff;		// BB
        colors[5] = 0x00444444;		// GRAY
        colors[6] = 0x00555555;		// BOX
        //
        // Preon colors
        //
        int i;
        for(i = 0; i < NPREONS; i++)
        {
        	int r = 127 + (127 * (long) rand()) / RAND_MAX;
        	int g = 127 + (127 * (long) rand()) / RAND_MAX;
        	int b = 127 + (127 * (long) rand()) / RAND_MAX;
            colors[7 + i] = r<<16 | g<<8 | b;
        }
}

void newTransform3()
{
	m00 = 1;
	m01 = 0;
	m02 = 0;
	//
	m10 = 0;
	m11 = 1;
	m12 = 0;
	//
	m20 = 0;
	m21 = 0;
	m22 = 1;
	//
	m30 = 0;
	m31 = 0;
	m32 = 0;
}

void translate(double tx, double ty, double tz)
{
	m30 += tx;
	m31 += ty;
	m32 += tz;
}

void rotateX(double s, double c)
{
	double t;
	t = m01 * c - m02 * s;
	m02 = m01 * s + m02 * c;
	m01 = t;
	//
	t = m11 * c - m12 * s;
	m12 = m11 * s + m12 * c;
	m11 = t;
	//
	t = m21 * c - m22 * s;
	m22 = m21 * s + m22 * c;
	m21 = t;
	//
	t = m31 * c - m32 * s;
	m32 = m31 * s + m32 * c;
	m31 = t;
}

void rotateY(double s, double c)
{
	double t;
	t = m00 * c + m02 * s;
	m02 = -m00 * s + m02 * c;
	m00 = t;
	//
	t = m10 * c + m12 * s;
	m12 = -m10 * s + m12 * c;
	m10 = t;
	//
	t = m20 * c + m22 * s;
	m22 = -m20 * s + m22 * c;
	m20 = t;
	//
	t = m30 * c + m32 * s;
	m32 = -m30 * s + m32 * c;
	m30 = t;
}

void rotateZ(double s, double c)
{
	double t;
	t = m00 * c - m01 * s;
	m01 = m00 * s + m01 * c;
	m00 = t;
	//
	t = m10 * c - m11 * s;
	m11 = m10 * s + m11 * c;
	m10 = t;
	//
	t = m20 * c - m21 * s;
	m21 = m20 * s + m21 * c;
	m20 = t;
	//
	t = m30 * c - m31 * s;
	m31 = m30 * s + m31 * c;
	m30 = t;
}

void viewPlaneTransform(Vector3d *p)
{
	double x = p->x * m00 + p->y * m10 + p->z * m20 + m30;
	double y = p->x * m01 + p->y * m11 + p->z * m21 + m31;
	double z = p->x * m02 + p->y * m12 + p->z * m22 + m32;
	p->x = x;
	p->y = y;
	p->z = z;
}

void makePerspectiveTransformation()
{
	pc.x = center.x;
	pc.y = center.y;
	pc.z = center.z;
	viewPlaneTransform(&pc);
	if(pc.z < 0)
		perror("Center of Projection behind View Plane.");
}

void makeViewPlaneTransformation()
{
	// Start with the identity matrix
	//
	newTransform3();
	//
	// Translate so that view plane center is new origin
	//
	translate(-(position.x + direction.x * distance),
				  -(position.y + direction.y * distance),
			  -(position.z + direction.z * distance));
	//
	// Rotate so that view plane normal is z axis
	//
	double v = sqrt(direction.y * direction.y + direction.z * direction.z);
	if(v > ROUNDOFF)
		rotateX(-direction.y / v, -direction.z / v);
	rotateY(direction.x, v);
	//
	// Determine the view-up direction in these new coordinates
	//
	double xup_vp = attitude.x * m00 + attitude.y * m10 + attitude.z * m20;
	double yup_vp = attitude.x * m01 + attitude.y * m11 + attitude.z * m21;
	//
	// Determine rotation needed to make view-up vertical
	//
	double rup = sqrt(xup_vp * xup_vp + yup_vp * yup_vp);
	if(rup < ROUNDOFF)
		perror("set-view-up along view-plane normal");
	rotateZ(xup_vp / rup, yup_vp / rup);
	//
	// Transform the center of projection
	//
	if(!parallel)
		makePerspectiveTransformation();
}

void setWindow(double _wxl, double _wxh, double _wyl, double _wyh)
{
	if(_wxl >= _wxh || _wyl >= _wyh)
		perror("bad window");
	wxl = _wxl;
	wxh = _wxh;
	wyl = _wyl;
	wyh = _wyh;
}

void setViewDepth(double _frontDistance, double _backDistance)
{
	if(frontDistance > backDistance)
		perror("Frontal Plane behind Back Plane");
		//
	frontDistance = _frontDistance;
	backDistance = _backDistance;
}

void setViewPort(double _vxl, double _vxh, double _vyl, double _vyh)
{
	if(_vxl >= _vxh || _vyl >= _vyh)
		perror("bad viewport");
	vxl = _vxl;
	vxh = _vxh;
	vyl = _vyl;
	vyh = _vyh;
}

void flipMode()
{
	parallel = !parallel;
}

void flipBox()
{
	showBox = !showBox;
}

void newView2()
{
	wsx = (vxh - vxl) / (wxh - wxl);
	wsy = (vyh - vyl) / (wyh - wyl);
}

void newView3()
{
	makeViewPlaneTransformation();
	newView2();
}

void setViewDistance(double _distance)
{
	distance = _distance;
}

void setPerspective(double x, double y, double z)
{
	center.x = x;
	center.y = y;
	center.z = z;
}

void clearBuffer()
{
	int i;
	DWORD *ptr = pixels;
	for(i = 0; i < WIDTH * HEIGHT; i++)
		*ptr++ = background;
}

void parallelTransform(Vector3d *p)
{
	p->x -= pc.x;
	p->y -= pc.y;
	p->z = 0;
}

void perspectiveTransform(Vector3d *p)
{
	double d = pc.z - p->z;
	if(fabs(d) < ROUNDOFF)
	{
		p->x = (p->x - pc.x) * VERYLARGE;
		p->y = (p->y - pc.y) * VERYLARGE;
		p->z = VERYLARGE;
	}
	else
	{
		p->x = (p->x * pc.z - pc.x * p->z) / d;
		p->y = (p->y * pc.z - pc.y * p->z) / d;
		p->z /= d;
	}
}

void enter(int color)
{
	// Transform the point to camera space
	//
	viewPlaneTransform(&pen);
	//
	// Clipping against clipping planes
	//
	if(clipping && (pen.z > -frontDistance || pen.z < -backDistance))
		return;
	//
	if(parallel)
		parallelTransform(&pen);
	else
		perspectiveTransform(&pen);
	//
	// Clipping against frustum sides
	//
	if(pen.x >= wxl && pen.x <= wxh && pen.y >= wyl && pen.y <= wyh)
	{
		// Convert to screen coordinates
		//
		int xi = (int) (((pen.x - wxl) * wsx + vxl) * WIDTH + .5);
		int yi = (int) (((pen.y - wyl) * wsy + vyl) * HEIGHT + .5);
		//
		// Save point in output buffer
		//
		int pos = WIDTH - xi + (HEIGHT - yi - 1) * WIDTH;
		if(pos > 0 && pos < WIDTH * HEIGHT)
			pixels[pos] = colors[color];
	}
}

void plot(double x, double y, double z, char color)
{
	pen.x = x;
	pen.y = y;
	pen.z = z;
	enter(color);
}

void marker(Tuple xyz)
{
	double dx = (xyz.x - SIDE/2);
	double dy = (xyz.y - SIDE/2);
	double dz = (xyz.z - SIDE/2);
	plot(dx, dy, dz, WHT);
}

void setParallel(double dx, double dy, double dz)
{
	if((fabs(dx) + fabs(dy) + fabs(dz)) < ROUNDOFF)
		perror("No direction of projection");
}

/*
 * Called by the main3d.c Windows interface.
 */
void initPlot()
{
	double h = sqrt(225) / (1.5 * GRID);
	position.x = (int)(10.0 / h);
	position.y = (int)(5.0 / h);
	position.z = (int)(10.0 / h);
	//
	direction.x = -position.x;		// camera axis
	direction.y = -position.y;
	direction.z = -position.z;
	norm3d(&direction);
	//
	attitude.x = 0; 				// view-up direction
	attitude.y = 0;
	attitude.z = -1;
	//
	setViewPort(0.0, 1.0, 0.0, 1.0);
	setWindow(-SIDE, SIDE, -SIDE, SIDE);
	newView2();
	setViewDistance(0);
	if(!parallel)
	{
		double xc = position.x - direction.x * scale;
		double yc = position.y - direction.y * scale;
		double zc = position.z - direction.z * scale;
		setPerspective(xc, yc, zc);
	}
	newView3();
	setViewDepth(0, 10000);
	clearBuffer();
}

void drawVoxel(double dx, double dy, double dz, char color)
{
	int N = SIDE / 2 + 1;
	double d = 2.0 / (3 * SIDE);	// ????
	int M = SIDE *d / 4;
	dx-=M; dy-=M; dz-=M;
	int x, y, z;
	for(x = 0; x < N; x++)
		for(y = 0; y < N; y++)
			plot(dx + x*d, dy + y*d, dz, color);
	for(y = 0; y < N; y++)
		for(z = 0; z < N; z++)
			plot(dx, dy + y*d, dz + z*d, color);
	for(z = 0; z < N; z++)
		for(x = 0; x < N; x++)
			plot(dx + x*d, dy, dz + z*d, color);
	//
	for(x = 0; x < N; x++)
		for(y = 0; y < N; y++)
			plot(dx + x*d, dy + y*d, dz + N*d, color);
	for(y = 0; y < N; y++)
		for(z = 0; z < N; z++)
			plot(dx+N*d, dy + y*d, dz + z*d, color);
	for(z = 0; z < N; z++)
		for(x = 0; x < N; x++)
			plot(dx + x*d, dy+N*d, dz + z*d, color);
}

void drawLattice()
{
	char *voxels = snap;
	int x, y, z;
	for(x = 0; x < SIDE; x++)
		for(y = 0; y < SIDE; y++)
			for(z = 0; z < SIDE; z++)
			{
				char color = voxels[SIDE2*x + SIDE*y + z];
				double dx = (x - SIDE/2);
				double dy = (y - SIDE/2);
				double dz = (z - SIDE/2);
				//
				if(color != gridcolor && color != background)
					drawVoxel(dx, dy, dz, color);
				else if(showGrid)
					plot(dx, dy, dz, gridcolor);
			}
	Marker *m = markers;
	while(m)
	{
		marker(m->xyz);
		m = m->next;
	}
}

void drawBox()
{
	double L = SIDE / 2;
	double incr = L / 200;
	double dx = -L;
	while(dx < L)
	{
		plot(dx, L, L, BOX);
		plot(dx, -L, L, BOX);
		plot(dx, L, -L, BOX);
		plot(dx, -L, -L, BOX);
		dx +=incr;
	}
	double dy = -L;
	while(dy < L)
	{
		plot(L, dy, L, BOX);
		plot(-L, dy, L, BOX);
		plot(L, dy, -L, BOX);
		plot(-L, dy, -L, BOX);
		dy +=incr;
	}
	double dz = -L;
	while(dz < L)
	{
		plot(L, L, dz, BOX);
		plot(-L, L, dz, BOX);
		plot(L, -L, dz, BOX);
		plot(-L, -L, dz, BOX);
		dz +=incr;
	}
}

void drawChar(double x, double y, double z, char color, char ch)
{
	pen.x = x;
	pen.y = y;
	pen.z = z;

	// Transform the point to camera space
	//
	viewPlaneTransform(&pen);
	//
	// Clipping against clipping planes
	//
	if(clipping && (pen.z > -frontDistance || pen.z < -backDistance))
		return;
	//
	if(parallel)
		parallelTransform(&pen);
	else
		perspectiveTransform(&pen);
	//
	// Clipping against frustum sides
	//
	if(pen.x >= wxl && pen.x <= wxh && pen.y >= wyl && pen.y <= wyh)
	{
		// Convert to screen coordinates
		//
		int xi = (int) (((pen.x - wxl) * wsx + vxl) * WIDTH + .5);
		int yi = (int) (((pen.y - wyl) * wsy + vyl) * HEIGHT + .5);
		vprint(WIDTH - xi, yi, ch);
	}
}

void drawAxes()
{
	setViewPort(0.0, 0.2, 0.0, 0.2);
	setWindow(-40, 40, -40, 40);
	newView2();
	setViewDistance(0);
	if(!parallel)
	{
		double xc = position.x - direction.x * scale;
		double yc = position.y - direction.y * scale;
		double zc = position.z - direction.z * scale;
		setPerspective(xc, yc, zc);
	}
	newView3();
	for(int i = 0; i < 35; i++)
	{
		double p = i;
		if(i < 30)
		{
			plot(p, 0, 0, X);
			plot(p, +0.3, 0, X);
			plot(p, -0.3, 0, X);
			plot(p, 0, +0.3, X);
			plot(p, 0, -0.3, X);
			plot(0, p, 0, Y);
			plot(.3, p, 0, Y);
			plot(-.3, p, 0, Y);
			plot(0, p, +0.3, Y);
			plot(0, p, -0.3, Y);
			plot(0, 0, p, Z);
			plot(0, +0.3, p, Z);
			plot(0, -0.3, p, Z);
			plot(+0.3, 0, p, Z);
			plot(-0.3, 0, p, Z);
		}
		else if(i == 34)
		{
			drawChar(p, 0, 0, X, 'x');
			drawChar(0, p, 0, Y, 'y');
			drawChar(0, -0.3, p, Z, 'z');
		}
	}
	setViewPort(0.0, 1.0, 0.0, 1.0);
	setWindow(-SIDE, SIDE, -SIDE, SIDE);
	newView2();
	setViewDistance(0);
	if(!parallel)
	{
		double xc = position.x - direction.x * scale;
		double yc = position.y - direction.y * scale;
		double zc = position.z - direction.z * scale;
		setPerspective(xc, yc, zc);
	}
	newView3();
}

/*
 * Called by automaton.c Loop3d thread
 */
void updatePlot()
{
	// Camera follows a circle and points to center
	//
	if(rot)
	{
		position.x = 1.5 * GRID * cos(theta);
		position.y = 1.5 * GRID * sin(theta);
		position.z = 0;
		//
		direction.x = -cos(theta);
		direction.y = -sin(theta);
		direction.z = 0;
		//
		attitude.x = 0;
		attitude.y = 0;
		attitude.z = 1;
		//
		theta += 0.01;
	}
	//
	// Create a new 2d view
	//
	newView2();
	//
	// Move the center of projection
	//
	if(!parallel)
	{
		setPerspective(position.x - scale * direction.x,
					   position.y - scale * direction.y,
					   position.z - scale * direction.z);
	}
	//
	// Create a new 3d view
	//
	newView3();
	//
	// Recreate pixels
	//
	clearBuffer();
	drawLattice();
	if(showBox)
		drawBox();
	if(showAxes)
		drawAxes();
}

void updateCamera()
{
        position.x = _position.x;
        position.y = _position.y;
        position.z = _position.z;
        //
        direction.x = _direction.x;
        direction.y = _direction.y;
        direction.z = _direction.z;
}

void visualize()
{
	if(pri0->p1 % 2 == 0)
	{
		updatePlot();
		//
		// Graphic text
		//
	 	char *s;
	    asprintf(&s, "Automaton %dx%dx%dx%d", SIDE, SIDE, SIDE, SIDE);
	    vprints(20, 20, s);
	    //
	    asprintf(&s, "Scenario: %s", scenarios[scenario]);
	    vprints(330, 20, s);
	    //
	    asprintf(&s, "Elapsed: %lu ms", GetTickCount() - begin);
	    vprints(20, 40, s);
	    //
	    asprintf(&s, "Views:");
	    vprints(20, 70, s);
	    //
	    asprintf(&s, "0: isometric");
	    vprints(25, 90, s);
	    //
	    asprintf(&s, "1: xy");
	    vprints(25, 110, s);
	    //
	    asprintf(&s, "2: yz");
	    vprints(25, 130, s);
	    //
	    asprintf(&s, "3: zx");
	    vprints(25, 150, s);
	    //
	 	asprintf(&s, "ls=%lu  clk=%lu", timer / (2 * DIAMETER), timer);
		vprints(20, 740, s);
		//
	    asprintf((char **)&s, "A: axes on/off");
	    vprints(620, 680, s);
	    //
	    asprintf((char **)&s, "S: pause/resume");
	    vprints(620, 700, s);
	    //
	    asprintf((char **)&s, "G: grid on/off");
	    vprints(620, 720, s);
	    //
	    asprintf((char **)&s, "X: box on/off");
	    vprints(620, 740, s);
		//
	    if(stop)
	    {
	    	asprintf((char **)&s, "[PAUSED]");
	    	vprints(370, 395, s);
	    }
		//
		// Update screen
		//
		SetDIBits(dc, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
		BitBlt(hdc, 0, 0, WIDTH, HEIGHT, dc, 0, 0, SRCCOPY);
	}
}

void *DisplayLoop()
{
 	begin = GetTickCount();
    pthread_detach(pthread_self());
    pthread_mutex_unlock(&mutex);
	initPalette();
    while(true)
    {
    	if(splash)
    	{
    		clearBuffer();
    		//
    	 	char *s;
    	 	if(scenario == -1)
    	 	{
        	    asprintf(&s, "Select scenario:");
        	    vprints(310, 310, s);
        	    for(int i = 0; i < 8; i++)
        	    {
        	    	asprintf(&s, "%d - %s", i+1, scenarios[i]);
        	    	vprints(330, 330 + 15*i, s);
        	    }
    	 	}
    	 	else
    	 	{
    	    	asprintf((char **)&s, "[WAIT...]");
    	    	vprints(370, 395, s);
    	 	}
    		SetDIBits(dc, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
    		BitBlt(hdc, 0, 0, WIDTH, HEIGHT, dc, 0, 0, SRCCOPY);
            usleep(200000);
    	}
    	else
    	{
        	if(input_changed)
        	{
            	pthread_mutex_lock(&mutex);
        		updateCamera();
        		input_changed = false;
        	    pthread_mutex_unlock(&mutex);
        	}
       		visualize();
            //
            // Swap snap <--> clean
            //
       		if(img_changed)
       		{
       	    	pthread_mutex_lock(&mutex);
       	    	char *flip = snap;
       	    	snap = clean;
       	    	clean = flip;
       	    	img_changed = false;
       		    pthread_mutex_unlock(&mutex);
       		}
    	}
    }
}
