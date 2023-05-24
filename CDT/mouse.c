/*
 * mouse.c
 */

#include <windowsx.h>
#include <math.h>
#include "plot3d.h"
#include "engine.h"

extern Trackball *tb;
extern float direction[3];		  // camera axis
extern float attitude[3];   	  // view-up direction

void projectP(float x, float y, float *res)
{
	int width = tb->box.right - tb->box.left;
	int height = tb->box.top - tb->box.bottom;
    float r = 1;
	float s = min(width, height);
	x = +(2 * x - width + 1) / s;
	y = +(2 * y - height + 1) / s;
    float z;
    if(x * x + y * y <= (r *r / 2))
	    z = sqrt(r * r - x * x - y * y);
	else
	    z = (r * r / 2) / sqrt(x * x + y * y);
    res[0] = x;
    res[1] = -y;
    res[2] = z;
}

void mulQuat(float *a, float *b, float *c)
{
    c[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
    c[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
    c[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
    c[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
}

void fromBetweenVector(float *a, float *b, float *c)
{
	c[0] = 1.0;  // Initialize with identity quaternion
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    float mp = sqrt((a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) * (b[0] * b[0] + b[1] * b[1] + b[2] * b[2]));
    float theta = acos(dot / mp);

    // Calculate the axis of rotation
    float axis[3];
    axis[0] = (a[1] * b[2]) - (a[2] * b[1]);
    axis[1] = (a[2] * b[0]) - (a[0] * b[2]);
    axis[2] = (a[0] * b[1]) - (a[1] * b[0]);

    // Calculate the axis magnitude
    float axisMagnitude = sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

    // Check if axisMagnitude is non-zero
    if (axisMagnitude != 0.0)
    {
        // Normalize the axis of rotation
        axis[0] /= axisMagnitude;
        axis[1] /= axisMagnitude;
        axis[2] /= axisMagnitude;

        // Calculate the quaternion components
        c[0] = cos(theta / 2);
        c[1] = axis[0] * sin(theta / 2);
        c[2] = axis[1] * sin(theta / 2);
        c[3] = axis[2] * sin(theta / 2);
    }
    else
    {
        // Handle the case when axisMagnitude is zero (avoid division by zero)
        // You can choose an appropriate action here, such as setting c to identity quaternion [1, 0, 0, 0]
        c[0] = 1;
        c[1] = 0;
        c[2] = 0;
        c[3] = 0;
    }
}

void mousedown(int x, int y)
{
	tb->beginx = x;
	tb->beginy = y;
	tb->dragged = true;
}

void mousemove(int x, int y)
{
    if (!tb->dragged)
        return;
    float a[3], b[3];
    projectP(tb->beginx, tb->beginy, a);
    projectP(x, y, b);
    fromBetweenVector(a, b, tb->p);
	mulQuat(tb->p, tb->q, tb->rotation);
	rotateVectorX(tb->rotation, direction, direction);
	rotateVectorX(tb->rotation, attitude, attitude);
}

void mouseup()
{
    if (!tb->dragged)
        return;
    mulQuat(tb->p, tb->q, tb->q);
    tb->p[0] = 1;
    tb->p[1] = 0;
    tb->p[2] = 0;
    tb->p[3] = 0;
    tb->dragged = false;
}

void rotateVectorX(float *rotation, float *object, float *rotated)
{
    float q0 = rotation[0];  // Quaternion components
    float q1 = rotation[1];
    float q2 = rotation[2];
    float q3 = rotation[3];

    // Quaternion multiplication
    float t1 = 2 * (q1 * object[0] + q2 * object[1] + q3 * object[2]);
    float t2 = 2 * q0;
    float t3 = 2 * (q0 * object[0] + q2 * object[2] - q3 * object[1]);
    float t4 = 2 * (q0 * object[1] - q1 * object[0] + q3 * object[2]);
    float t5 = 2 * (q0 * object[2] + q1 * object[1] - q2 * object[0]);
    float t6 = 2 * (q2 * object[0] - q1 * object[1] - q0 * object[2]);
    float t7 = 2 * (q1 * object[2] + q3 * object[0] - q0 * object[1]);
    float t8 = 2 * (q0 * object[0] - q1 * object[1] - q2 * object[2]);

    // Quaternion rotation
    rotated[0] = q0 * t1 + q1 * t2 + q2 * t3 - q3 * t4;
    rotated[1] = q0 * t5 + q1 * t6 + q2 * t7 + q3 * t2;
    rotated[2] = q0 * t8 - q1 * t7 + q2 * t6 + q3 * t1;
}

void mouse(UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x = GET_X_LPARAM(lparam);
    int y = GET_Y_LPARAM(lparam);
	if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT)
		return;
	int r = min(WIDTH, HEIGHT);
	if (tb->rotH)
	{
		y = HEIGHT / 2;
	}
	else if (tb->rotV)
	{
		x = WIDTH / 2;
	}
	else if (x * x + y * y > r * r)
	{
		float sine = y / (double)x;
		x = r * sine;
		y = sqrt(r * r - x * x);
	}
	switch(msg)
	{
		case WM_LBUTTONDOWN:
        {
        	if (x < WIDTH / 4 || x > 3 * WIDTH / 4)
        		tb->rotV = true;
        	else if (y < HEIGHT / 4 || y > 3 * HEIGHT / 4)
        		tb->rotH = true;
        	mousedown(x, y);
        	break;
        }
		case WM_LBUTTONUP:
    		tb->rotV = false;
    		tb->rotH = false;
   			mouseup();
			break;
		case WM_MOUSEMOVE:

			if(tb->dragged)
			{
				mousemove(x, y);
			}
			break;
	}
}
