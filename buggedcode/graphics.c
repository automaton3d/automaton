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
extern unsigned long timer;
extern boolean stop;
extern Cell *latt0;
extern Quaternion rotation;
extern boolean active;
extern RECT bitrect;
extern Quaternion currQ, lastQ;
extern float scale;
extern float dx, dy;
extern HWND stopButton;
extern int np;

COLORREF voxels[SIDE6];

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

void putVoxel(Vector v, COLORREF color, HDC hdc)
{
    Vector rotated = rotateVector(v, rotation);
    SetPixel(hdc, WIDTH/2 + rotated.x + dx, HEIGHT/2 + rotated.y + dy, color);
}

void projPoint(Vector v, int *x, int *y)
{
    Vector rotated = rotateVector(v, rotation);
    *x = WIDTH/2 + rotated.x/2 + dx;
    *y = HEIGHT/2 + rotated.y/2 + dy;
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
    rect.right = rect.left + 200;
    rect.bottom = rect.top + 20;
    DrawText(hdc, labelText, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
}

void keyboard(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(wparam)
	{
		case 'S':
			stop = !stop;
			if (stop)
				SetWindowText(stopButton, TEXT("Resume"));
			else
				SetWindowText(stopButton, TEXT("Pause"));
			InvalidateRect(hwnd, &bitrect, TRUE);
			break;
		case VK_UP:
			dy += 2;
			InvalidateRect(hwnd, &bitrect, TRUE);
			break;
		case VK_DOWN:
			dy -= 2;
			InvalidateRect(hwnd, &bitrect, TRUE);
			break;
		case VK_RIGHT:	// right arrow
			dx -= 2;
			InvalidateRect(hwnd, &bitrect, TRUE);
			break;
		case VK_LEFT:
			dx += 2;
			InvalidateRect(hwnd, &bitrect, TRUE);
			break;
	}
}

void normalize_quat(Quaternion *quat)
{
    float magnitude = sqrtf(quat->x * quat->x + quat->y * quat->y + quat->z * quat->z + quat->w * quat->w);
    if (magnitude != 0.0f)
    {
        quat->x /= magnitude;
        quat->y /= magnitude;
        quat->z /= magnitude;
        quat->w /= magnitude;
    }
}

void setView(int view, Quaternion *quat)
{
	switch(view)
	{
		case ISO_VIEW:
			float angle = M_PI / 8.0;  // 45/2 degrees
			quat->w = cosf(angle);
			quat->x = sinf(angle);
			quat->y = sinf(angle);
			quat->z = sinf(angle);
			break;
		case XY_VIEW:
			quat->w = 1;
			quat->x = 0;
			quat->y = 0;
			quat->z = 0;
			break;
		case YZ_VIEW:
			quat->w = 0;
			quat->x = 0.707;
			quat->y = 0;
			quat->z = 0.707;
			break;
		case ZX_VIEW:
			quat->w = 0;
			quat->x = 0;
			quat->y = 0.707;
			quat->z = 0.707;
			break;
	}
    normalize_quat(quat);
}

////////////////// TOP LEVEL ROUTINES ///////////////////

void updateBuffer()
{
	if (!active)
		return;
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
    asprintf(&s, "NP: %d", np);
    TextOut(hdc, 20, 740, s, strlen(s));
    if (stop)
    {
        asprintf(&s, "[PAUSED]");
        TextOut(hdc, 370, 395, s, strlen(s));
    }
    if (!active)
    {
        asprintf(&s, "[BACKGROUND]");
        TextOut(hdc, 370, 395, s, strlen(s));
    }
}

void drawModel(HDC hdc)
{
  if (!active)
	  return;
  rotation = Quaternion_multiply(currQ, lastQ);
  Vector p;
  for (int i = 0; i < SIDE6; i++)
  {
	  COLORREF color = voxels[i];
	  if (color != RGB(0,0,0))
	  {
		  int x = i % SIDE2;
		  int y = (i / SIDE2) % SIDE2;
		  int z = (i / SIDE4) % SIDE2;
		  p.x = (x - SIDE2/2)*scale;
		  p.y = (y - SIDE2/2)*scale;
		  p.z = (z - SIDE2/2)*scale;
		  putVoxel(p, color, hdc);
	  }
  }
  if (axes)
  {
	  int x, y;
	  SelectObject(hdc, xPen);
	  Vector p1 = { 0, 0, 0 };
	  Vector p2 = { 2*SIDE2*scale, 0, 0 };
	  projLine(hdc, p1, p2);
	  projPoint(p2, &x, &y);
      TextOut(hdc, x, y, "x", 1);
	  SelectObject(hdc, yPen);
	  Vector p3 = { 0, 2*SIDE2*scale, 0 };
	  projLine(hdc, p1, p3);
	  projPoint(p3, &x, &y);
      TextOut(hdc, x, y, "y", 1);
	  SelectObject(hdc, zPen);
	  Vector p4 = { 0, 0, 2*SIDE2*scale };
	  projLine(hdc, p1, p4);
	  projPoint(p4, &x, &y);
      TextOut(hdc, x, y, "z", 1);
  }
  if (cube)
  {
	  SelectObject(hdc, boxPen);
	  rotation = Quaternion_multiply(currQ, lastQ);
	  unsigned long shift, b[3] = { 0x00be6b3eUL, 0x0069635fUL, 0x0010b088UL };
	  Vector v[2];
	  for (int i = 0; i < 72; i++, shift >>= 1)
	  {
		  if(i % 24 == 0) shift = b[i/24];
		  int index = (i / 3) % 2;
		  int xyz = (shift & 1) == 0 ? +SIDE2*scale : -SIDE2*scale;
		  switch(i % 3)
		  {
		  	  case 0: v[index].x = xyz; break;
		  	  case 1: v[index].y = xyz; break;
		  	  case 2: v[index].z = xyz; break;
		  }
		  if((i + 1) % 6 == 0) projLine(hdc, v[0], v[1]);
	  }
  }
}
