/*
 * mouse.c
 */

#include "mouse.h"
#include <stdio.h>      // debug
#include <math.h>
#include "common.h"
#include "plot3d.h"
#include "vector3d.h"
#include "quaternion.h"

Quaternion q, qstart;
double radius = 0.4 * HEIGHT;
int fDraw = false;
boolean input_changed = false;

Vector3d p0, p1;

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

int mouse(char op, int x, int y)
{
	switch(op)
	{
		case 'd':
        {
        	fDraw = true;
        	pick(&p0, x, y);
        	qstart.x = direction.x;
        	qstart.y = direction.y;
        	qstart.z = direction.z;
        	qstart.w = 0;
        	break;
        }
		case 'u':
			fDraw = false;
			break;
		case 'm':
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
				Quaternion qnow;
				mul(&qnow, q, qstart);
				_direction.x = qnow.x;
				_direction.y = qnow.y;
				_direction.z = qnow.z;
				norm3d(&_direction);
				double hy = sqrt(position.x * position.x + position.y * position.y + position.z * position.z);
				_position.x = -_direction.x * hy;
				_position.y = -_direction.y * hy;
				_position.z = -_direction.z * hy;
				input_changed = true;
			}
			break;
		case 'p':
		case 'h':
			for(int i = 0; i < NSCENES; i++)
    	    	if(x > 320 && y > 258 + 15*i && y < 272 + 15*i)
    	    		return i;
			break;
	}
	return -1;
}

