/*
 * engine.c
 *
 *  Created on: 24 de jan de 2021
 *      Author: Alexandre
 */

#include "engine.h"

extern Trackball view;
extern unsigned long begin;

Trackball *vu;

// Transformation matrix
//
static double m00, m01, m02;
static double m10, m11, m12;
static double m20, m21, m22;
static double m30, m31, m32;
//
// 3d fields
//
static float center[3];	      	  // center of projection
static float pc[3];         	  // transformed center of projection
static float pen[3];        	  // plotting pen

static double distance;       // view distance
static double frontDistance;  // clipping plane
static double backDistance;	  // clipping plane
//
// Window
//
static double wxl, wyl;
static double wxh, wyh;
static double vxl, vyl;
static double vxh, vyh;
static double wsx, wsy;
//
float position[3];      	  // view reference point
float direction[3];		  // camera axis
float attitude[3];   	  // view-up direction
//
static boolean parallel = false;
static boolean clipping = true;
static char background;
static int scale = 100;
static Frustum f;
static Voxel *curr;

DWORD *pixels, *colors;
Voxel *imgbuf[2], *buff, *clean;

void initScreen()
{
  // Alocate memory for voxel buffers

  imgbuf[0] = malloc(IMGBUFFER*sizeof(Voxel));
  imgbuf[1] = malloc(IMGBUFFER*sizeof(Voxel));
  buff = imgbuf[0];
  clean = imgbuf[1];
  buff->color = -1;
  clean->color = -1;

  begin = GetTickCount64();      // initial milliseconds
  setvbuf(stdout, 0, _IOLBF, 0);

  // Initialize trackball

  vu = &view;
  vu->box.left = 0;
  vu->box.top = 0;
  vu->box.right = WIDTH;
  vu->box.bottom = HEIGHT;
  vu->p[0] = 1;
  vu->p[1] = 0;
  vu->p[2] = 0;
  vu->p[3] = 0;
  vu->q[0] = 1;
  vu->q[1] = 0;
  vu->q[2] = 0;
  vu->q[3] = 0;
  vu->rotation[0] = 1;
  vu->rotation[1] = 0;
  vu->rotation[2] = 0;
  vu->rotation[3] = 0;
  vu->dragged = false;
  vu->rotH = false;
  vu->rotV = false;

  Sleep(4);
}

/*
 * Define colors.
 */
void initPalette()
{
    colors[0]  = 0x00000000; // BLK
    colors[1] = 0x00404040; // GRAY
    colors[2]  = 0x00ff0000; // RED
	colors[3]  = 0x00008000; // GREEN
    colors[4]  = 0x000000ff; // BLUE
	colors[5]  = 0x00000080; // NAVY
}

// Scale is the value you multiply x and y with before you divide
// them by z. (usually I use 256 for this value).

void initFrustum(Frustum *f)
{
	float angle_horizontal =  atan2(WIDTH/2, scale) - 0.0001;
	float angle_vertical   =  atan2(HEIGHT/2, scale) - 0.0001;
	float sh               =  sin(angle_horizontal);
	float sv               =  sin(angle_vertical);
	float ch               =  cos(angle_horizontal);
	float cv               =  cos(angle_vertical);
	//
	// left
	//
	f->sides[0].normal[0] = ch;
	f->sides[0].normal[1] = 0;
	f->sides[0].normal[2] = sh;
	f->sides[0].distance = 0;
	//
	// right
	//
	f->sides[1].normal[0] = -ch;
	f->sides[1].normal[1] = 0;
	f->sides[1].normal[2] = sh;
	f->sides[1].distance = 0;
	//
	// top
	//
	f->sides[2].normal[0] = 0;
	f->sides[2].normal[1] = cv;
	f->sides[2].normal[2] = sv;
	f->sides[2].distance = 0;
	//
	// bottom
	//
	f->sides[3].normal[0] = 0;
	f->sides[3].normal[1] = -cv;
	f->sides[3].normal[2] = sv;
	f->sides[3].distance = 0;
	//
	// z-near clipping plane
	//
	f->znear.normal[0] = 0;
	f->znear.normal[1] = 0;
	f->znear.normal[2] = 1;
	f->znear.distance = -10;
}

boolean isParallel()
{
	return parallel;
}

char getBackground()
{
	return background;
}

void setBackground(char color)
{
	background = color;
}

void setCamera(float p[3], float d[3], float a[3])
{
	position[0]  = p[0];
	position[1]  = p[1];
	position[2]  = p[2];
	direction[0] = d[0];
	direction[1] = d[1];
	direction[2] = d[2];
	attitude[0]  = a[0];
	attitude[1]  = a[1];
	attitude[2]  = a[2];
}

void getCamera(float *p, float *d, float *a)
{
	p[0] = position[0];
	p[1] = position[1];
	p[2] = position[2];
	d[0] = direction[0];
	d[1] = direction[1];
	d[2] = direction[2];
	a[0] = attitude[0];
	a[1] = attitude[1];
	a[2] = attitude[2];
}

static void newTransform3()
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

static void translate(double x, double y, double z)
{
	m30 += x;
	m31 += y;
	m32 += z;
}

static void rotateX(double s, double c)
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

static void rotateY(double s, double c)
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

static void rotateZ(double s, double c)
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

static void viewPlaneTransform(float *p)
{
	double x = p[0] * m00 + p[1] * m10 + p[2] * m20 + m30;
	double y = p[0] * m01 + p[1] * m11 + p[2] * m21 + m31;
	double z = p[0] * m02 + p[1] * m12 + p[2] * m22 + m32;
	p[0] = x;
	p[1] = y;
	p[2] = z;
}

static void makePerspectiveTransformation()
{
	pc[0] = center[0];
	pc[1] = center[1];
	pc[2] = center[2];
	viewPlaneTransform(pc);
	if(pc[2] < 0)
		perror("Center of Projection behind View Plane.");
}

static void makeViewPlaneTransformation()
{
	// Start with the identity matrix
	//
	newTransform3();
	//
	// Translate so that view plane center is new origin
	//
	translate(-(position[0] + direction[0] * distance),
			  -(position[1] + direction[1] * distance),
			  -(position[2] + direction[2] * distance));
	//
	// Rotate so that view plane normal is z axis
	//
	double v = sqrt(direction[1] * direction[1] + direction[2] * direction[2]);
	if(v > ROUNDOFF)
		rotateX(-direction[1] / v, -direction[2] / v);
	rotateY(direction[0], v);
	//
	// Determine the view-up direction in these new coordinates
	//
	double xup_vp = attitude[0] * m00 + attitude[1] * m10 + attitude[2] * m20;
	double yup_vp = attitude[0] * m01 + attitude[1] * m11 + attitude[2] * m21;
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
	center[0] = x;
	center[1] = y;
	center[2] = z;
}

void clearBuffer()
{
	DWORD color = colors[(int)background];
	int i;
	DWORD *ptr = pixels;
	for(i = 0; i < WIDTH * HEIGHT; i++)
		*ptr++ = color;
}

static void parallelTransform(float *p)
{
	p[0] -= pc[0];
	p[1] -= pc[1];
	p[2] = 0;
}

static void perspectiveTransform(float *p)
{
	double d = pc[2] - p[2];
	if(fabs(d) < ROUNDOFF)
	{
		p[0] = (p[0] - pc[0]) * VERYLARGE;
		p[1] = (p[1] - pc[1]) * VERYLARGE;
		p[2] = VERYLARGE;
	}
	else
	{
		p[0] = (p[0] * pc[2] - pc[0] * p[2]) / d;
		p[1] = (p[1] * pc[2] - pc[1] * p[2]) / d;
		p[2] /= d;
	}
}

static void enter(int color)
{
	// Transform the point to camera space
	//
	viewPlaneTransform(pen);
	//
	// Clipping against clipping planes
	//
	if(clipping && (pen[2] > -frontDistance || pen[2] < -backDistance))
		return;
	//
	if(parallel)
	{
		parallelTransform(pen);
		pen[0] *= 0.06;	// patch
		pen[1] *= 0.06;
	}
	else
		perspectiveTransform(pen);
	//
	// Clipping against frustum sides
	//
	if(pen[0] >= wxl && pen[0] <= wxh && pen[1] >= wyl && pen[1] <= wyh)
	{
		// Convert to screen coordinates
		//
		int xi = (int) (((pen[0] - wxl) * wsx + vxl) * WIDTH + .5);
		int yi = (int) (((pen[1] - wyl) * wsy + vyl) * HEIGHT + .5);
		//
		// Save point in output buffer
		//
		int pos = WIDTH - xi + (HEIGHT - yi - 1) * WIDTH;
		if(pos > 0 && pos < WIDTH * HEIGHT)
		{
			if(color != GRAY || pixels[pos] == 0)
				pixels[pos] = colors[color];
		}
	}
}

void putVoxel(float v[3], char color)
{
	curr->pos[0] = v[0];
	curr->pos[1] = v[1];
	curr->pos[2] = v[2];
	curr->color = color;
	curr++;
	curr->color = -1;	// end of file
}

/**
 * Projects all voxels on the screen.
 */
void update3d()
{
	Voxel *ptr = clean;
	for(int i = 0; ptr->color >= 0; i++, ptr++)
	{
		pen[0] = ptr->pos[0];
		pen[1] = ptr->pos[1];
		pen[2] = ptr->pos[2];
		enter(ptr->color);
	}
}

/*
 * Double buffering.
 */
void flipBuffers()
{
	Voxel *tmp = buff;
	buff = clean;
	clean = tmp;
	curr = buff;
	assert(curr != NULL);
}

/*
 * Direct pixel plotting
 */
void putPixel(int x, int y, int color)
{
	// Clipping against frustum sides
	//
	if(x > 0 && x < WIDTH && y > 0 && y < HEIGHT)
		*(pixels + x + y*WIDTH) = colors[color];
}

/**
 * Called by the main3d.c Windows interface.
 */
void initEngine(double zoom)
{
	colors = malloc(NCOLORS * sizeof(DWORD));
	//
	initPalette();
	initFrustum(&f);
	//
	position[0] = 2500;
	position[1] = 2500;
	position[2] = 2500;
	//
	direction[0] = -position[0];		// camera axis
	direction[1] = -position[1];
	direction[2] = -position[2];
	normalize(direction);
	//
	attitude[0] = 0; 				// view-up direction
	attitude[1] = 0;
	attitude[2] = -1;
	float tmp[3];
	cross3d(attitude, direction, tmp);
	normalize(tmp);
	cross3d(direction, tmp, attitude);
	//
	setViewPort(0.0, 1.0, 0.0, 1.0);
	setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
	newView2();
	setViewDistance(0);
	if(!parallel)
	{
		double xc = position[0] - direction[0] * scale;
		double yc = position[1] - direction[1] * scale;
		double zc = position[2] - direction[2] * scale;
		setPerspective(xc, yc, zc);
	}
	newView3();
	setViewDepth(-WINDOW, 1000000);
	clearBuffer();
}

void newProjection()
{
	if(!parallel)
	{
		double xc = position[0] - direction[0] * scale;
		double yc = position[1] - direction[1] * scale;
		double zc = position[2] - direction[2] * scale;
		setPerspective(xc, yc, zc);
	}
}

