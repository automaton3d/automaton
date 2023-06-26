#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "graphics.h"

extern float radius;
extern Quaternion rotation;
extern float width, height;
extern float dx, dy;

extern Quaternion currQ, lastQ;
extern RECT bitrect;
extern Vector start;

Quaternion Quaternion_multiply(Quaternion q1, Quaternion q2)
{
    Quaternion result;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    return result;
}

Quaternion Quaternion_fromBetweenVectors(Vector a, Vector b)
{
    // Normalize the input vectors
    double lengthA = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    double lengthB = sqrt(b.x * b.x + b.y * b.y + b.z * b.z);

    Vector v1 = { a.x / lengthA, a.y / lengthA, a.z / lengthA };
    Vector v2 = { b.x / lengthB, b.y / lengthB, b.z / lengthB };

    // Calculate the dot product and cross product
    double dotProduct = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    Vector crossProduct = {
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    };

    // Calculate the angle between vectors a and b
    double angle = acos(dotProduct);

    // Calculate the quaternion components
    Quaternion result;
    result.w = cos(angle / 2.0);
    double s = sin(angle / 2.0);
    result.x = s * crossProduct.x;
    result.y = s * crossProduct.y;
    result.z = s * crossProduct.z;

    return result;
}

void zoom(float wheelDelta, Quaternion* currQ, Quaternion* lastQ)
{
    float scalingFactor = wheelDelta > 0 ? 1.1f : 0.9f;  // Determine the scaling factor based on wheel delta

    // Apply scaling to currQ and lastQ
    currQ->w *= scalingFactor;
    currQ->x *= scalingFactor;
    currQ->y *= scalingFactor;
    currQ->z *= scalingFactor;

    lastQ->w *= scalingFactor;
    lastQ->x *= scalingFactor;
    lastQ->y *= scalingFactor;
    lastQ->z *= scalingFactor;

    // Normalize currQ and lastQ to maintain quaternion properties
    float currQLength = sqrt(currQ->w * currQ->w + currQ->x * currQ->x + currQ->y * currQ->y + currQ->z * currQ->z);
    currQ->w /= currQLength;
    currQ->x /= currQLength;
    currQ->y /= currQLength;
    currQ->z /= currQLength;

    float lastQLength = sqrt(lastQ->w * lastQ->w + lastQ->x * lastQ->x + lastQ->y * lastQ->y + lastQ->z * lastQ->z);
    lastQ->w /= lastQLength;
    lastQ->x /= lastQLength;
    lastQ->y /= lastQLength;
    lastQ->z /= lastQLength;
}


Vector rotateVector(Vector v, Quaternion q)
{
    // Perform quaternion rotation on vector v
    Quaternion conjugate = { q.w, -q.x, -q.y, -q.z };
    Quaternion result = Quaternion_multiply(q, Quaternion_multiply((Quaternion){0.0, v.x, v.y, v.z}, conjugate));
    Vector rotated;
    rotated.x = result.x;
    rotated.y = result.y;
    rotated.z = result.z;
    return rotated;
}

void projLine(HDC hdc, Vector point1, Vector point2)
{
	Vector rotated = rotateVector(point1, rotation);
    MoveToEx(hdc, WIDTH/2 + rotated.x/2 + dx, HEIGHT/2 + rotated.y/2 + dy, NULL);
	rotated = rotateVector(point2, rotation);
	LineTo(hdc, WIDTH/2 + rotated.x/2 + dx, HEIGHT/2 + rotated.y/2 + dy);
}

Vector project(double x, double y)
{
    double res = fmin(width, height) - 1.0;

    // Map x and y to the range -1 to 1
    x = (2.0 * x - width - 1.0) / res;
    y = (2.0 * y - height - 1.0) / res;

    double r = radius;
    double z;

    // Calculate the z-coordinate based on the radius and x, y values
    if (x * x + y * y <= r * r / 2.0)
        z = sqrt(r * r - x * x - y * y);
    else
        z = r * r / (2.0 * sqrt(x * x + y * y));

    Vector result;
    result.x = x;
    result.y = -y;
    result.z = z;

    return result;
}

void init()
{
    // Set initial orientations
    lastQ.w = 1; // Example initial values
    lastQ.x = 0;
    lastQ.y = 0;
    lastQ.z = 0;
    currQ.w = 1.0;
    currQ.x = 0.0;
    currQ.y = 0.0;
    currQ.z = 0.0;
    start.x = 0.0;
    start.y = 0.0;
}

void mousedown(double x, double y)
{
    start.x = x;
    start.y = y;
}

void mousemove(double x, double y, HWND hwnd)
{
	if (start.x == 0.0 && start.y == 0.0)
		return;

	Vector a = project(start.x, start.y);
	Vector b = project(x, y);

	// Calculate the rotation between vectors a and b

	currQ = Quaternion_fromBetweenVectors(a, b);
	normalize_quat(&currQ);
    InvalidateRect(hwnd, &bitrect, TRUE);
}

void mouseup(double x, double y)
{
	if (start.x == 0.0 && start.y == 0.0)
		return;

	lastQ = Quaternion_multiply(currQ, lastQ);
	currQ.w = 1.0;
	currQ.x = 0.0;
	currQ.y = 0.0;
	currQ.z = 0.0;
	start.x = 0.0;
	start.y = 0.0;
}