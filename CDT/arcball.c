#include <stdio.h>
#include <math.h>
#include "view.h"
#include "utils.h"

extern View view;

void projectP(float x, float y, float *res)
{
    float r = 1;
	float s = min(view.width, view.height);
	x = +(2 * x - view.width + 1) / s;
	y = +(2 * y - view.height + 1) / s;
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
    view.beginx = x;
    view.beginy = y;
    view.dragged = true;
}

void mousemove(int x, int y)
{
    if (!view.dragged)
        return;
    float a[3], b[3];
    projectP(view.beginx, view.beginy, a);
    projectP(x, y, b);
    fromBetweenVector(a, b, view.currQ);
}

void mouseup()
{
    if (!view.dragged)
        return;
    mulQuat(view.currQ, view.lastQ, view.lastQ);
    view.currQ[0] = 1;
    view.currQ[1] = 0;
    view.currQ[2] = 0;
    view.currQ[3] = 0;
    view.dragged = false;
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
