/*
 * mouse.c
 */

#include <math.h>
#include <stdio.h>//debug

#include "mouse.h"
#include "engine.h"
#include "common.h"
#include "plot3d.h"
#include "vector3d.h"
#include "quaternion.h"
#include "gadget.h"
#include "utils.h"

extern Quaternion q, qstart;
Quaternion pstart;
double radius = 0.4 * HEIGHT;
boolean fDraw = false;
boolean input_changed = true;

static Vector3d p0, p1;
static Vector3d pos, dir, att;

int zoomInt = 1000;

boolean ticks[NTICKS];
boolean pan = false;
int xx0, yy0;

void pick(Vector3d *p, int x, int y)
{
	p->x = (x - HEIGHT / 2) / radius;
	p->y = (y - WIDTH / 2) / radius;
	p->z = 0;
	double r = p->x*p->x + p->y*p->y;
	if(r > 1)
	{
		double s = 1 / sqrt(r);
		p->x *= s;
		p->y *= s;
	}
	else
	{
		p->z = sqrt(1 - r);
	}
}

void mouse(UINT msg, WPARAM wparam, LPARAM lparam)
{
	int delta;
	int x = HIWORD(lparam);
	int y = LOWORD(lparam);
	pthread_mutex_lock(&mutex);
	switch(msg)
	{
		case WM_LBUTTONDOWN:
        {
        	// Test gadgets
        	//
        	if(testCheckbox(y, x))
        	{
        		input_changed = true;
        	}
        	else
        	{
            	fDraw = true;
            	pick(&p0, x, y);
    			getCamera(&pos, &dir, &att);
            	qstart.x = dir.x;
            	qstart.y = dir.y;
            	qstart.z = dir.z;
            	qstart.w = 0;
            	//
            	pstart.x = att.x;
            	pstart.y = att.y;
            	pstart.z = att.z;
            	pstart.w = 0;
        	}
        	break;
        }
		case WM_LBUTTONUP:
			fDraw = false;
			break;
		case WM_RBUTTONDOWN:
			pan = true;
			xx0 = x;
			yy0 = y;
			break;
		case WM_RBUTTONUP:
			pan = false;
			break;
		case WM_MOUSEMOVE:
			if(fDraw && !input_changed)
			{
				pick(&p1, x, y);
				Vector3d p2;
				cross3d(p0, p1, &p2);
				q.x = p2.x;
				q.y = p2.y;
				q.z = p2.z;
				q.w = dot3d(p0, p1);
				//
				Vector3d newDir, newAtt, newPos;
				Quaternion qnow;
				mul(&qnow, q, qstart);
				newDir.x = qnow.x;
				newDir.y = qnow.y;
				newDir.z = qnow.z;
				normalize(&newDir);
				//
				mul(&qnow, q, pstart);
				newAtt.x = qnow.x;
				newAtt.y = qnow.y;
				newAtt.z = qnow.z;
				normalize(&newAtt);
				//
				double hy = sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
				newPos.x = -newDir.x * hy;
				newPos.y = -newDir.y * hy;
				newPos.z = -newDir.z * hy;
				setCamera(newPos, newDir, newAtt);
				input_changed = true;
			}
			else if(pan && !input_changed)
			{
				int dx = (x - xx0) / 10;
				int dy = (y - yy0) / 10;
				printf("pan %d,%d\n", dx, dy);
			}
			break;
		case WM_MOUSEWHEEL:
			delta = GET_WHEEL_DELTA_WPARAM(wparam);
			zoom(delta);
			break;
	}
	pthread_mutex_unlock(&mutex);
}
