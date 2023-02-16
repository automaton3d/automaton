/*
 * engine.c
 *
 *  Created on: 24 de jan de 2021
 *      Author: Alexandre
 */

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "common.h"
#include "engine.h"
#include "text.h"
#include "main3d.h"

// Transformation matrix
//
static double m00, m01, m02;
static double m10, m11, m12;
static double m20, m21, m22;
static double m30, m31, m32;
//
// 3d fields
//
static Vector3d center;	      	// center of projection
static Vector3d pc;         	// transformed center of projection
static Vector3d pen;        	// plotting pen

static double distance;        	// view distance
static double frontDistance;	// clipping plane
static double backDistance;		// clipping plane
//
// Window
//
static double wxl, wyl;
static double wxh, wyh;
static double vxl, vyl;
static double vxh, vyh;
static double wsx, wsy;
//
static Vector3d position;      	// view reference point
static Vector3d direction;		// camera axis
static Vector3d attitude;   	// view-up direction
//
static boolean parallel = 0;//false;
static boolean clipping = true;
static char background;
static int scale = 100;
static Frustum f;
static Voxel *curr;

DWORD *pixels, *colors;
Voxel *imgbuf[2], *buff, *clean;

/*
 * Define colors.
 */
void initPalette()
{
    colors[0]  = 0x00000000;		// BLK
	colors[1]  = 0x00000080;		// NAVY
    colors[2]  = 0x000000ff;		// BLUE
    colors[3]  = 0x00800000;		// MAROON
	colors[4]  = 0x00800080;		// PURPLE
    colors[5]  = 0x00ff0000;		// RED
	colors[6]  = 0x00ff00ff;		// MAGENTA
	colors[7]  = 0x00008000;		// GREEN
	colors[8]  = 0x00008080;		// TEAL
	colors[9]  = 0x00808000;		// OLIVE
    colors[10] = 0x00808080;		// GRAY
    colors[11] = 0x0000ff00;		// LIME
	colors[12] = 0x00ffa500;		// ORANGE
    colors[13] = 0x0000ffff;		// CYAN
    colors[14] = 0x00ffff00;		// YELLOW
    colors[15] = 0x00ffffff;		// WHT
	colors[16] = 0x00c0c0c0;		// SILVER
    colors[17] = 0x00111100;		// PALE
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
	f->sides[0].normal.x = ch;
	f->sides[0].normal.y = 0;
	f->sides[0].normal.z = sh;
	f->sides[0].distance = 0;
	//
	// right
	//
	f->sides[1].normal.x = -ch;
	f->sides[1].normal.y = 0;
	f->sides[1].normal.z = sh;
	f->sides[1].distance = 0;
	//
	// top
	//
	f->sides[2].normal.x = 0;
	f->sides[2].normal.y = cv;
	f->sides[2].normal.z = sv;
	f->sides[2].distance = 0;
	//
	// bottom
	//
	f->sides[3].normal.x = 0;
	f->sides[3].normal.y = -cv;
	f->sides[3].normal.z = sv;
	f->sides[3].distance = 0;
	//
	// z-near clipping plane
	//
	f->znear.normal.x = 0;
	f->znear.normal.y = 0;
	f->znear.normal.z = 1;
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

void setCamera(Vector3d p, Vector3d d, Vector3d a)
{
	position  = p;
	direction = d;
	attitude  = a;
}

void getCamera(Vector3d *p, Vector3d *d, Vector3d *a)
{
	*p = position;
	*d = direction;
	*a = attitude;
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

static void viewPlaneTransform(Vector3d *p)
{
	double x = p->x * m00 + p->y * m10 + p->z * m20 + m30;
	double y = p->x * m01 + p->y * m11 + p->z * m21 + m31;
	double z = p->x * m02 + p->y * m12 + p->z * m22 + m32;
	p->x = x;
	p->y = y;
	p->z = z;
}

static void makePerspectiveTransformation()
{
	pc.x = center.x;
	pc.y = center.y;
	pc.z = center.z;
	viewPlaneTransform(&pc);
	if(pc.z < 0)
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

void shrinkWindow()
{
	wxl *= 0.85;
	wxh *= 0.85;
	wyl *= 0.95;
	wyh *= 0.85;
}

void expandWindow()
{
	wxl *= 1.15;
	wxh *= 1.15;
	wyl *= 1.15;
	wyh *= 1.15;
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

Vector3d getPerspective()
{
	return center;
}

void clearBuffer()
{
	DWORD color = colors[(int)background];
	int i;
	DWORD *ptr = pixels;
	for(i = 0; i < WIDTH * HEIGHT; i++)
		*ptr++ = color;
}

static void parallelTransform(Vector3d *p)
{
	p->x -= pc.x;
	p->y -= pc.y;
	p->z = 0;
}

static void perspectiveTransform(Vector3d *p)
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

static void enter(int color)
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

void putVoxel(Vector3d v, char color)
{
	curr->pos = v;
	curr->color = color;
	curr++;
	curr->color = -1;	// end of file
}

void plot(Vector3d v, char color)
{
	pen = v;
	enter(color);
}

/**
 * Projects all voxels on the screen.
 * Origin: clean buffer.
 */
void update3d()
{
	Voxel *ptr = clean;
	for(int i = 0; ptr->color >= 0; i++, ptr++)
	{
		pen = ptr->pos;
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

/*
static void setParallel(Vector3d d)
{
	if((fabs(d.x) + fabs(d.y) + fabs(d.z)) < ROUNDOFF)
		perror("No direction of projection");
}
*/

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
	position.x = 3*zoom;
	position.y = 2*zoom;
	position.z = 2.5*zoom;
	//
	direction.x = -position.x;		// camera axis
	direction.y = -position.y;
	direction.z = -position.z;
	normalize(&direction);
	//
	attitude.x = 0; 				// view-up direction
	attitude.y = 0;
	attitude.z = -1;
	Vector3d tmp;
	cross3d(attitude, direction, &tmp);
	normalize(&tmp);
	cross3d(direction, tmp, &attitude);
	//
	setViewPort(0.0, 1.0, 0.0, 1.0);
	setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
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
	setViewDepth(-WINDOW, 1000000);
	clearBuffer();
}

/*
 * Print a character in 3d space.
 */
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

void newProjection()
{
	if(!parallel)
	{
		double xc = position.x - direction.x * scale;
		double yc = position.y - direction.y * scale;
		double zc = position.z - direction.z * scale;
		setPerspective(xc, yc, zc);
	}
}

/*
 * Acts on camera focus.
 */
void zoom(int delta)
{
	scale += delta/20;
	if(scale < 70)
		scale = 70;
	if(scale > 260)
		scale = 260;
}

static float rotationMatrix[4][4];
static float inputMatrix[4][1]  = {{0.0}, {0.0}, {0.0}, {0.0}};
static float outputMatrix[4][1] = {{0.0}, {0.0}, {0.0}, {0.0}};

static void multiplyMatrix()
{
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 1; j++)
        {
            outputMatrix[i][j] = 0;
            for(int k = 0; k < 4; k++)
                outputMatrix[i][j] += rotationMatrix[i][k] * inputMatrix[k][j];
        }
    }
}

static void setUpRotationMatrix(float angle, float u, float v, float w)
{
    float L = (u*u + v * v + w * w);
    angle = angle * PI / 180.0; //converting to radian value
    float u2 = u * u;
    float v2 = v * v;
    float w2 = w * w;

    rotationMatrix[0][0] = (u2 + (v2 + w2) * cos(angle)) / L;
    rotationMatrix[0][1] = (u * v * (1 - cos(angle)) - w * sqrt(L) * sin(angle)) / L;
    rotationMatrix[0][2] = (u * w * (1 - cos(angle)) + v * sqrt(L) * sin(angle)) / L;
    rotationMatrix[0][3] = 0.0;

    rotationMatrix[1][0] = (u * v * (1 - cos(angle)) + w * sqrt(L) * sin(angle)) / L;
    rotationMatrix[1][1] = (v2 + (u2 + w2) * cos(angle)) / L;
    rotationMatrix[1][2] = (v * w * (1 - cos(angle)) - u * sqrt(L) * sin(angle)) / L;
    rotationMatrix[1][3] = 0.0;

    rotationMatrix[2][0] = (u * w * (1 - cos(angle)) - v * sqrt(L) * sin(angle)) / L;
    rotationMatrix[2][1] = (v * w * (1 - cos(angle)) + u * sqrt(L) * sin(angle)) / L;
    rotationMatrix[2][2] = (w2 + (u2 + v2) * cos(angle)) / L;
    rotationMatrix[2][3] = 0.0;

    rotationMatrix[3][0] = 0.0;
    rotationMatrix[3][1] = 0.0;
    rotationMatrix[3][2] = 0.0;
    rotationMatrix[3][3] = 1.0;
}

/**
 * Arc ball.
 */
Vector3d arcball(Vector3d points, Vector3d axis, double angle)
{
    inputMatrix[0][0] = points.x;
    inputMatrix[1][0] = points.y;
    inputMatrix[2][0] = points.z;
    inputMatrix[3][0] = 1.0;
    setUpRotationMatrix(angle, axis.x, axis.y, axis.z);
    multiplyMatrix();
    Vector3d result;
    result.x = outputMatrix[0][0];
    result.y = outputMatrix[1][0];
    result.z = outputMatrix[2][0];
    return result;
}

void panH(int offset)
{
	Vector3d xaxis;
	cross3d(attitude, direction, &xaxis);
	normalize(&xaxis);
	scale3d(&xaxis, offset);
	add3d(&position, xaxis);
}

void panV(int offset)
{
	Vector3d yaxis = attitude;
	normalize(&yaxis);
	scale3d(&yaxis, offset);
	add3d(&position, yaxis);
}

void point2d(int x, int y, int color)
{
	pixels[x + y*WIDTH] = colors[color];
}

/**
 * Draws a 2d line.
 */
void line2d(int x0, int y0, int x1, int y1, int color)
{
	int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
    if(dy < 0)
    {
    	dy = -dy;  stepy = -1;
    }
    else
    {
    	stepy = 1;
    }
    if(dx < 0)
    {
    	dx = -dx;  stepx = -1;
    }
    else
    {
    	stepx = 1;
    }
    dy <<= 1;                                         // dy is now 2*dy
    dx <<= 1;                                         // dx is now 2*dx
    pixels[x0 + y0*WIDTH] = color;
    if(dx > dy)
    {
    	int fraction = dy - (dx >> 1);                  // same as 2*dy - dx
    	while (x0 != x1)
    	{
    		if(fraction >= 0)
    		{
    			y0 += stepy;
    			fraction -= dx;                             // same as fraction -= 2*dx
    		}
    		x0 += stepx;
    		fraction += dy;                               // same as fraction -= 2*dy
    		pixels[x0 + y0*WIDTH] = colors[color];
    	}
    }
    else
    {
    	int fraction = dx - (dy >> 1);
    	while(y0 != y1)
    	{
    		if(fraction >= 0)
    		{
    			x0 += stepx;
    			fraction -= dy;
    		}
    		y0 += stepy;
    		fraction += dx;
    		pixels[x0 + y0*WIDTH] = colors[color];
    	}
    }
}

static boolean clip(Line *line, Plane plane)
{
	double dist1 = dot3d(line->p1, plane.normal);
	double dist2 = dot3d(line->p2, plane.normal);
	if(dist1 < 0 && dist2 < 0)
		return false;
	else if(!(dist1 > 0 && dist2 > 0))
	{
		double s = dist1 / (dist1 - dist2);
		Vector3d intersect;
		intersect.x = line->p1.x + s*(line->p2.x - line->p1.x);
		intersect.y = line->p1.y + s*(line->p2.y - line->p1.y);
		intersect.z = line->p1.z + s*(line->p2.z - line->p1.z);
		if(dist1 < 0)
			line->p1 = intersect;
		else
			line->p2 = intersect;
	}
	return true;
}

Plane buildPlane(Vector3d a, Vector3d b, Vector3d c)
{
	Plane p;
	//
	// Build normal vector
	//
	Vector3d q, v;
	q.x = b.x - a.x;    v.x = b.x - c.x;
	q.y = b.y - a.y;    v.x = b.y - c.y;
	q.z = b.y - a.y;    v.x = b.z - c.z;
	cross3d(q, v, &p.normal);
	normalize(&p.normal);
	//
	// Calculate distance to origin
	//
	p.distance = dot3d(p.normal, a);  // you could also use b or c
	return p;
}

void clipFrustum(Line line)
{
	int n = 0;
	Frustum f;
	for(int i = 0; i < 4 && n < 2; i++)
	{
		if(clip(&line, f.sides[0]))
			n++;
	}
	clip(&line, f.znear);
}
