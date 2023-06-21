/*
 * gavin.h
 *
 *  Created on: 26 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <windows.h>

#define WIDTH   800
#define HEIGHT  800

#define TRACKBALLSIZE  (0.7f)
#define RENORMCOUNT 97

typedef struct
{
    double w, x, y, z;

} Quaternion;

// Vector structure

typedef struct
{
    double x, y, z;

} Vector;

typedef struct
{
	int x, y;
} V2;

HWND CreateCheckBox(HWND hwndParent, int x, int y, int width, int height, int id, LPCWSTR text);
void DrawLabel(HDC hdc, int x, int y, const TCHAR* labelText);
//void trackball(float q[4], float p1x, float p1y, float p2x, float p2y);
void mul(float *r, float *a, float *b);
void normalize_quat(Quaternion *quat);
void add_quats(float q1[4], float q2[4], float dest[4]);
void keyboard(UINT msg, WPARAM wparam, LPARAM lparam);
void updateBuffer();
float tb_project_to_sphere(float, float, float);
void scaleQuat(float quat[4]);
void setView(int view, Quaternion *quat);
void vcopy(const float *v1, float *v2);

void mousedown(double x, double y);
void mousemove(double x, double y, HWND hwnd);
void mouseup(double x, double y);
Vector project(double x, double y);
Vector rotateVector(Vector v, Quaternion q);
Quaternion Quaternion_multiply(Quaternion q1, Quaternion q2);
void zoom(float wheelDelta, Quaternion* currQ, Quaternion* lastQ);
void putVoxel(Vector v, COLORREF color, HDC hdc);
void projLine(HDC hdc, Vector point1, Vector point2);
void pan(float deltaX, float deltaY, Quaternion* currQ, Quaternion* lastQ);
Quaternion Quaternion_fromBetweenVectors(Vector a, Vector b);

#endif /* GRAPHICS_H_ */
