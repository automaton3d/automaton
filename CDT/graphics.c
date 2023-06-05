#define _GNU_SOURCE

#include "graphics.h"
#include "main3d.h"

#include <windows.h>
#include <stdio.h>
#include <math.h>

extern HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk, axes_chk;
extern HWND single_rad, partial_rad, full_rad, xy_rad, yz_rad, zx_rad, iso_rad, rand_rad;
extern boolean momentum, wavefront, mode0, mode1, mode2, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;
extern HPEN xPen, yPen, zPen, boxPen;
extern HWND hwnd;
extern pthread_mutex_t mutex;
extern float currQ[4], lastQ[4];
extern unsigned long timer;
extern boolean stop;
extern Cell *latt0;
extern float rotation[4];
extern float scale;

COLORREF voxels[SIDE6];

/////////////// LINEAR ALGEBRA //////////////

void vzero(float *v)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}

void vset(float *v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

void vsub(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}

void vcopy(const float *v1, float *v2)
{
    register int i;
    for (i = 0 ; i < 3 ; i++)
        v2[i] = v1[i];
}

void vcross(const float *v1, const float *v2, float *cross)
{
    float temp[3];
    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy(temp, cross);
}

float vlength(const float *v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void vscale(float *v, float div)
{
    v[0] *= div;
    v[1] *= div;
    v[2] *= div;
}

void vnormal(float *v)
{
    vscale(v, 1.0/vlength(v));
}

float vdot(const float *v1, const float *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void vadd(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] + src2[0];
    dst[1] = src1[1] + src2[1];
    dst[2] = src1[2] + src2[2];
}

void axis_to_quat(float a[3], float phi, float q[4])
{
    vcopy(a, q + 1);
    vnormal(q + 1);
    vscale(q + 1, sin(phi / 2.0));
    q[0] = cos(phi / 2.0);
}

void trackball(float q[4], float p1x, float p1y, float p2x, float p2y)
{
    float a[3]; /* Axis of rotation */
    float phi;  /* how much to rotate about axis */
    float p1[3], p2[3], d[3];
    float t;
    if (p1x == p2x && p1y == p2y)
    {
        vzero(q);
        q[0] = SIDE2;
        return;
    }
    vset(p1, p1x, p1y, tb_project_to_sphere(TRACKBALLSIZE, p1x, p1y));
    vset(p2, p2x, p2y, tb_project_to_sphere(TRACKBALLSIZE, p2x, p2y));
    vcross(p2, p1, a);
    vsub(p1, p2, d);
    t = vlength(d) / (2.0 * TRACKBALLSIZE);
    if (t > 1.0) t = SIDE2;
    if (t < -1.0) t = -SIDE2;
    phi = 2.0 * asin(t);
    axis_to_quat(a, phi, q);
}

/*
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
float tb_project_to_sphere(float r, float x, float y)
{
    float d, t, z;

    d = sqrt(x * x + y * y);
    r = r * 0.7;
    if (d < r)
    {
        // Inside sphere
        z = sqrt(r * r - d * d);
    }
    else
    {
        // On hyperbola
        t = r / 1.41421356237309504880;
        z = (t * t) / d;
    }
    return z;
}

void add_quats(float q1[4], float q2[4], float dest[4])
{
	for (register int i = 0; i < 4; i++) dest[i] = q1[i] + q2[i];
}

void normalize_quat(float q[4])
{
    float mag = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    for (int i = 0; i < 4; i++) q[i] /= mag;
}

void scaleQuat(float quat[4])
{
    for (register int i = 0; i < 4; i++)
        quat[i] *= scale;
}

/*
 * q[0] corresponds to w.
 */
void build_rotmatrixbubu(float m[4][4], float q[4])
{
    m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
    m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
    m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
    m[0][3] = 0.0;

    m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
    m[1][1] = 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
    m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
    m[1][3] = 0.0;

    m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
    m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
    m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = SIDE2;
}

void build_rotmatrix(float m[4][4], float q[4])
{
    float w = q[0];
    float x = q[1];
    float y = q[2];
    float z = q[3];

    // Compute matrix elements
    float xx = x * x;
    float xy = x * y;
    float xz = x * z;
    float xw = x * w;
    float yy = y * y;
    float yz = y * z;
    float yw = y * w;
    float zz = z * z;
    float zw = z * w;

    // Fill in the rotation matrix
    m[0][0] = 1.0f - 2.0f * (yy + zz);
    m[0][1] = 2.0f * (xy - zw);
    m[0][2] = 2.0f * (xz + yw);
    m[0][3] = 0.0f;

    m[1][0] = 2.0f * (xy + zw);
    m[1][1] = 1.0f - 2.0f * (xx + zz);
    m[1][2] = 2.0f * (yz - xw);
    m[1][3] = 0.0f;

    m[2][0] = 2.0f * (xz - yw);
    m[2][1] = 2.0f * (yz + xw);
    m[2][2] = 1.0f - 2.0f * (xx + yy);
    m[2][3] = 0.0f;

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

void mul(float* r, float* a, float* b)
{
    r[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];  // w component
    r[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];  // x component
    r[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];  // y component
    r[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];  // z component
}

void rotateVector(float *rotated, float *rotation, float *object)
{
    float q_object[4] = { 0, object[0], object[1], object[2] };
    float q_conjugate[4] = { rotation[0], -rotation[1], -rotation[2], -rotation[3] };
    float temp1[4], temp2[4];
    mul(temp1, rotation, q_object);
    mul(temp2, temp1, q_conjugate);
    rotated[0] = temp2[1];
    rotated[1] = temp2[2];
    rotated[2] = temp2[3];
}

////////////// AUXILIARY ROUTINES ////////////////

void projLine(HDC hdc, float point1[3], float point2[3])
{
	float rotated[3];
	rotateVector(rotated, rotation, point1);
    MoveToEx(hdc, WIDTH/2 + rotated[0]/2, HEIGHT/2 + rotated[1]/2, NULL);
	rotateVector(rotated, rotation, point2);
	LineTo(hdc, WIDTH/2 + rotated[0]/2, HEIGHT/2 + rotated[1]/2);
}

boolean isCentralPoint(int i)
{
	int x = i % SIDE2 - SIDE_2;
	int y = (i / SIDE2) % SIDE2 - SIDE_2;
	int z = (i / SIDE4) % SIDE2 - SIDE_2;
	if(x < 0 || y < 0 || z < 0)
		return false;
	return x % SIDE == 0 && y % SIDE == 0 && z % SIDE == 0;
}

boolean isPartial(int i)
{
	int x = i % SIDE2 - SIDE_2;
	int y = (i / SIDE2) % SIDE2 - SIDE_2;
	int z = (i / SIDE4) % SIDE2 - SIDE_2;
	for (int j = -1; j < 2; j++)
		for (int k = -1; k < 2; k++)
			for (int l = -1; l < 2; l++)
			{
				if(x + j < 0 || y + k < 0 || z + l < 0)
					return false;
				if((x + j) % SIDE == 0 && (y + k) % SIDE == 0 && (z + l) % SIDE == 0)
					return true;
			}
	return false;
}

void putVoxel(float v[3], COLORREF color, HDC hdc)
{
    float rotated[3];
    rotateVector(rotated, rotation, v);
    SetPixel(hdc, WIDTH/2 + rotated[0], HEIGHT/2 + rotated[1], color);
}

void projPoint(float v[3], int *x, int *y)
{
    float rotated[3];
    rotateVector(rotated, rotation, v);
    *x = WIDTH/2 + rotated[0]/2;
    *y = HEIGHT/2 + rotated[1]/2;
}

HWND CreateCheckBox(HWND hwndParent, int x, int y, int width, int height, int id, LPCWSTR text)
{
	return CreateWindowA("BUTTON", (char *)text, WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, width, height, hwndParent, NULL, NULL, NULL);
}

void DrawLabel(HDC hdc, int x, int y, const TCHAR* labelText)
{
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = rect.left + 200; // Adjust the width as needed
    rect.bottom = rect.top + 20; // Adjust the height as needed
    DrawText(hdc, labelText, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
}

void keyboard(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(wparam) { case 'S': stop = !stop; InvalidateRect(hwnd, NULL, TRUE); }
}

void setView(int view, float *quat)
{
	switch(view)
	{
		case ISO_VIEW:
			float angle = M_PI / 8.0;  // 45/2 degrees
			quat[0] = cosf(angle);
			quat[1] = sinf(angle);
			quat[2] = sinf(angle);
			quat[3] = sinf(angle);
			break;
		case XY_VIEW:
			quat[0] = 1;
			quat[1] = 0;
			quat[2] = 0;
			quat[3] = 0;
			break;
		case YZ_VIEW:
			quat[0] = 0;
			quat[1] = 0.707;
			quat[2] = 0;
			quat[3] = 0.707;
			break;
		case ZX_VIEW:
			quat[0] = 0;
			quat[1] = 0;
			quat[2] = 0.707;
			quat[3] = 0.707;
			break;
	}
    normalize_quat(quat);
}

////////////////// TOP LEVEL ROUTINES ///////////////////

void updateBuffer()
{
	pthread_mutex_lock(&mutex);
	Cell *stb = latt0;
	for(int i = 0; i < SIDE6; i++, stb++)
	{
		boolean ok = mode2 || (mode1 && isPartial(i)) || (mode0 && isCentralPoint(i)) || (rnd && rand() % 100 == 0);
	    if(!ZERO(stb->p) && momentum && ok)
	      voxels[i] = RGB(255,0,0);
	    else if(!ZERO(stb->s) && wavefront && ok)
	      voxels[i] = RGB(0, 255,0);
	    else if (lattice && isCentralPoint(i))
	      voxels[i] = RGB(150,150,150);
	    else
	      voxels[i] = RGB(0,0,0);
	}
	pthread_mutex_unlock(&mutex);
}

void drawGUI(HDC hdc)
{
	wavefront = SendMessage(front_chk, BM_GETCHECK, FRONT, 0);
	track     = SendMessage(track_chk, BM_GETCHECK, TRACK, 0);
	momentum  = SendMessage(p_chk,     BM_GETCHECK, MOMENTUM, 0);
	plane     = SendMessage(plane_chk, BM_GETCHECK, PLANE, 0);
	cube      = SendMessage(cube_chk,  BM_GETCHECK, CUBE, 0);
	lattice   = SendMessage(latt_chk,  BM_GETCHECK, LATTICE, 0);
	axes      = SendMessage(axes_chk,  BM_GETCHECK, AXES, 0);
	mode0     = SendMessage(single_rad, BM_GETCHECK, MODE0, 0);
	mode1     = SendMessage(partial_rad, BM_GETCHECK, MODE1, 0);
	mode2     = SendMessage(full_rad, BM_GETCHECK, MODE2, 0);
	rnd       = SendMessage(rand_rad, BM_GETCHECK, MODE2, 0);
	xy        = SendMessage(xy_rad, BM_GETCHECK, XY_VIEW, 0);
	yz        = SendMessage(yz_rad, BM_GETCHECK, YZ_VIEW, 0);
	zx        = SendMessage(zx_rad, BM_GETCHECK, ZX_VIEW, 0);
	iso       = SendMessage(iso_rad, BM_GETCHECK, ISO_VIEW, 0);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));
    char* s = NULL;
    asprintf(&s, "light: %lu tick: %lu", timer / LIGHT, timer);
    TextOut(hdc, 20, 20, s, strlen(s));
    asprintf(&s, "SIDE: %d", SIDE);
    TextOut(hdc, 700, 20, s, strlen(s));
    asprintf(&s, "S: pause / resume");
    TextOut(hdc, 650, 740, s, strlen(s));
    if (stop)
    {
        asprintf(&s, "[PAUSED]");
        TextOut(hdc, 370, 395, s, strlen(s));
    }
}

void drawModel(HDC hdc)
{
  mul(rotation, currQ, lastQ);
  float p[3];
  for (int i = 0; i < SIDE6; i++)
  {
	  COLORREF color = voxels[i];
	  if (color != RGB(0,0,0))
	  {
		  int x = i % SIDE2;
		  int y = (i / SIDE2) % SIDE2;
		  int z = (i / SIDE4) % SIDE2;
		  p[0] = (x - SIDE2/2);
		  p[1] = (y - SIDE2/2);
		  p[2] = (z - SIDE2/2);
		  putVoxel(p, color, hdc);
	  }
  }
  if (axes)
  {
	  int x, y;
	  SelectObject(hdc, xPen);
	  float p1[3] = { 0, 0, 0 };
	  float p2[3] = { 2*SIDE2, 0, 0 };
	  projLine(hdc, p1, p2);
	  projPoint(p2, &x, &y);
      TextOut(hdc, x, y, "x", 1);
	  SelectObject(hdc, yPen);
	  float p3[3] = { 0, 2*SIDE2, 0 };
	  projLine(hdc, p1, p3);
	  projPoint(p3, &x, &y);
      TextOut(hdc, x, y, "y", 1);
	  SelectObject(hdc, zPen);
	  float p4[3] = { 0, 0, 2*SIDE2 };
	  projLine(hdc, p1, p4);
	  projPoint(p4, &x, &y);
      TextOut(hdc, x, y, "z", 1);
  }
  if (cube)
  {
	  SelectObject(hdc, boxPen);
	  unsigned long shift, b[3] = { 0x00be6b3eUL, 0x0069635fUL, 0x0010b088UL };
	  float p[2][3];
	  for (int i = 0; i < 72; i++, shift >>= 1)
	  {
		  if(i % 24 == 0) shift = b[i/24];
		  p[(i / 3) % 2][i % 3] = (shift & 1) == 0 ? +SIDE2 : -SIDE2;
		  if((i + 1) % 6 == 0) projLine(hdc, p[0], p[1]);
	  }
  }
}
