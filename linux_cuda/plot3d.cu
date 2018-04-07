/*
 * plot3d.cu
 *
 * Implements a 3d graphics pipeline.
 * This fast and simple engine is only capable of projecting isolated points.
 */
#include "plot3d.h"
#include <math.h>
#include <sys/time.h>
#include "brick.h"
#include "common.h"
#include "utils.h"
#include "text.h"
#include "params.h"
#include "automaton.h"
#include "jpeg.h"

pthread_t display;
boolean img_changed = true;

JSAMPLE *image_buffer;

// Transformation matrix
//
static double m00, m01, m02;
static double m10, m11, m12;
static double m20, m21, m22;
static double m30, m31, m32;
//
// 3d fields
//
static Vector3d center;		// center of projection
static Vector3d pc;		// transformed center of projection
static Vector3d pen;		// plotting pen

static double distance;		// view distance
static double frontDistance;	// clipping plane
static double backDistance;	// clipping plane
//
// Window
//
static double wxl, wyl;
static double wxh, wyh;
static double vxl, vyl;
static double vxh, vyh;
static double wsx, wsy;
//
static int parallel = true;
static int clipping;
static double scale = 1100;//0.8;

boolean showAxes = true, showGrid, showBox = true;

Vector3d position, _position;	// view reference point
Vector3d direction, _direction;	// camera axis
Vector3d attitude;		// view-up direction
struct timeval begin;

// Rotation

static double theta = 0;
static int rot = false;

// Colors

char colors [3 * NPREONS + 21];
static char gridcolor = 24;
static char X, Y, Z;
static char BOX;

void initPalette()
{
	X = R;
	Y = G;
	Z = B;
	//
	BOX = 18;
	gridcolor = GRAY;
	//
	colors[0] = 0;
	colors[1] = 0;
	colors[2] = 0;
	//
	colors[3] = 0xff;
	colors[4] = 0xff;
	colors[5] = 0xff;
	//
	colors[6] = 0xff;
	colors[7] = 0;
	colors[8] = 0;
	//
	colors[9] = 0;
	colors[10] = 0xff;
	colors[11] = 0;
	//
	colors[12] = 0;
	colors[13] = 0;
	colors[14] = 0xff;
	//
	colors[15] = 0x44;
	colors[16] = 0x44;
	colors[17] = 0x44;
	//
	colors[18] = 0x99;
	colors[19] = 0x99;
	colors[20] = 0x99;
	//
	// Preon colors
	//
	int i;
	for(i = 0; i < 3 * NPREONS;)
	{
		char pos = i % 3;
		switch(pos)
		{
			case 0:
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				break;
			case 1: 
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				break;
			case 2:
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				colors[21 + i++] = 127 + (127 * (long) rand()) / RAND_MAX;
				break;
		}
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

void setParallel(double dx, double dy, double dz)
{
        if((fabs(dx) + fabs(dy) + fabs(dz)) < ROUNDOFF)
                perror("No direction of projection");
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
	JSAMPLE *ptr = image_buffer;
	for(i = 0; i < 3 * WIDTH * HEIGHT; i++)
		*ptr++ = 0;
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
		if(xi < 0 || yi < 0 || xi >=WIDTH || yi >= HEIGHT)
			return;
		//
		// Save point in output buffer
		//
		int pos = WIDTH - xi + yi * WIDTH; 
		JSAMPLE *ptr2 = image_buffer + 3 * pos;
                *ptr2++ = colors[color];
	        *ptr2++ = colors[color + 1];
		*ptr2   = colors[color + 2];
	}
}

void plot(double x, double y, double z, int color)
{
	pen.x = x;
	pen.y = y;
	pen.z = z;
	enter(color);
}

/*
 * Initializes the projection engine.
 */
void initPlot()
{
	gettimeofday(&begin, NULL);
//	image_buffer = (JSAMPLE *) malloc(3 * WIDTH * HEIGHT * sizeof(JSAMPLE));
//	initPalette();
	double h = sqrt(225) / (1.5 * GRID);
	position.x = (int)(GD_X*BD_X*10.0 / h);
	position.y = (int)(GD_X*BD_X*5.0 / h);
	position.z = (int)(GD_X*BD_X*10.0 / h);
	//
	direction.x = -position.x;		// camera axis
	direction.y = -position.y;
	direction.z = -position.z;
	norm3d(&direction);
	//
	attitude.x = 0;				// view-up direction
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

void drawMarker(int x, int y, int z)
{
        double dx = (x - SIDE/2);
        double dy = (y - SIDE/2);
        double dz = (z - SIDE/2);
        plot(dx, dy, dz, WHT);
}

void drawBullet(double dx, double dy, double dz, char color)
{
	double d = 1.0 / BURST;
	plot(dx, dy, dz, color);
	plot(dx+d, dy, dz, color);
	plot(dx-d, dy, dz, color);
	plot(dx, dy+d, dz, color);
	plot(dx, dy-d, dz, color);
	plot(dx, dy, dz+d, color);
	plot(dx, dy, dz-d, color);
}

void drawVoxel(double dx, double dy, double dz, char color)
{
	int N = SIDE / 2 + 1;
	double d = 1.0 / BURST;
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

/*
 * Draws the automaton voxels.
 */
void drawLattice()
{
	int x, y, z;
	char *v = voxels; 
	for(x = 0; x < SIDE; x++)
		for(y = 0; y < SIDE; y++)
			for(z = 0; z < SIDE; z++)
			{
				double dx = (x - SIDE/2);
				double dy = (y - SIDE/2);
				double dz = (z - SIDE/2);
				//
				char color = *v;
				if(color != BLK)
				{
					if(SIDE < 128)
						drawVoxel(dx, dy, dz, color);
					else
						drawBullet(dx, dy, dz, color);
				}
				else if(showGrid)
					plot(dx, dy, dz, gridcolor);
				v++;
			}
	drawMarker(0,0,0);
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
		setPerspective(	position.x - scale * direction.x,
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
	updatePlot();
	//
	// Graphic text
	//
	struct timeval now;
	gettimeofday(&now, NULL);
	unsigned long long elapsed = 1000 * (now.tv_sec - begin.tv_sec) + (now.tv_usec - begin.tv_usec) / 1000;
	JSAMPLE *s;
	asprintf((char **)&s, "Automaton %dx%dx%dx%d", SIDE, SIDE, SIDE, NPREONS);
	vprints(20, 20, s);
	//
	asprintf((char **)&s, "Scenario: %s", scenarios[scenario]);
	vprints(330, 20, s);
	//
	asprintf((char **)&s, "%d threads", NTHREADS);
        vprints(20, 60, s);
	//
	asprintf((char **)&s, "Elapsed: %llu ms", elapsed);
        vprints(20, 40, s);
	//
	asprintf((char **)&s, "Views:");
	vprints(20, 90, s);
	//
	asprintf((char **)&s, "0: isometric");
	vprints(25, 110, s);
	//
	asprintf((char **)&s, "1: xy");
	vprints(25, 130, s);
	//
	asprintf((char **)&s, "2: yz");
	vprints(25, 150, s);
	//
	asprintf((char **)&s, "3: zx");
	vprints(25, 170, s);
	//
	asprintf((char **)&s, "ls=%lu  clk=%lu", timer / (2 * DIAMETER), timer);
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
	write_JPEG_file();
}

void *DisplayLoop(void *v)
{
        pthread_detach(pthread_self());
        pthread_mutex_unlock(&cam_mutex);
        while(true)
        {
		if(splash)
		{
			image_buffer = (JSAMPLE *) malloc(3 * WIDTH * HEIGHT * sizeof(JSAMPLE));
		        initPalette();
			clearBuffer();
			//
			JSAMPLE *s;
			if(scenario == -1)
			{
				asprintf((char **)&s, "Select scenario:");
				vprints(310, 310, s);
				for(int i = 0; i < 8; i++)
				{
					asprintf((char **)&s, "%d - %s", i+1, scenarios[i]);
					vprints(330, 330 + 15*i, s);
				}
			}
			else
			{
				asprintf((char **)&s, "[WAIT...]");
				vprints(370, 395, s);
			}
			//
			write_JPEG_file();
			usleep(200000);
		}
		else
		{
	                pthread_mutex_lock(&cam_mutex);
			if(automaton_changed)
			{
				img_changed = true;
				automaton_changed = false;
			}
			pthread_mutex_unlock(&cam_mutex);
			if(input_changed)
			{
				updateCamera();
				input_changed = false;
				img_changed = true;
			}
			if(img_changed)
			{
				visualize();
				img_changed = false;
			}
			usleep(10000);
		}
        }

}

