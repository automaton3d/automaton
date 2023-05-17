/*
 * keyboard.c
 *
 *  Created on: 24 de jul de 2017
 *      Author: Alexandre
 */

#include "keyboard.h"

extern boolean showAxes;
extern boolean verbose;
extern char gridcolor;
extern pthread_mutex_t mutex;
extern HWND cube_chk;

boolean stop;
double depth = 2.5;

void keyboard(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(wparam)
	{
		case 'A':
			showAxes = !showAxes;
			break;
		case 'B':
			if(getBackground() == BLK)
			{
				setBackground(WHT);
				gridcolor = YELLOW;
			}
			else
			{
				setBackground(BLK);
				gridcolor = NAVY;
			}
			break;
		case 'P':
			flipMode();
			break;

		case 'X':
	    	pthread_mutex_lock(&mutex);
			UINT currentState = SendMessage(cube_chk, BM_GETCHECK, 0, 0);
		    UINT newState = (currentState == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED;
		    SendMessage(cube_chk, BM_SETCHECK, newState, 0);
	    	pthread_mutex_unlock(&mutex);
		 	break;

		case 'S':
			stop = !stop;
			break;
		case 'V':
			verbose = !verbose;
		    break;
		case '0':
			{
	        	pthread_mutex_lock(&mutex);
				float p[3], d[3], a[3], xaxis[3];
				getCamera(p, d, a);
				cross3d(d, a, xaxis);
				p[0] = sqrt(3);
				p[1] = sqrt(3);
				p[2] = sqrt(3);
				normalize(p);
				d[0] = p[0];
				d[1] = p[1];
				d[2] = p[2];
				scale3d(p, 44);
				scale3d(d, -1);
				cross3d(xaxis, d, a);
				setCamera(p, d, a);
	        	pthread_mutex_unlock(&mutex);
			}
			break;

		case '1':
			{
				float p[3], d[3], a[3];
				p[0] = 44;
				p[1] = 0;
				p[2] = 0;
				//
				d[0] = -1;
				d[1] = 0;
				d[2] = 0;
				//
				a[0] = 0;
				a[1] = 0;
				a[2] = -1;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case '2':
			{
				float p[3], d[3], a[3];
				p[0] = 0;
				p[1] = 44;
				p[2] = 0;
				//
				d[0] = 0;
				d[1] = -1;
				d[2] = 0;
				a[0] = 0;
				//
				a[1] = 0;
				a[2] = 1;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case '3':
			{
				float p[3], d[3], a[3];
				p[0] = 0;
				p[1] = 0;
				p[2] = 44;
				//
				d[0] = 0;
				d[1] = 0;
				d[2] = -1;
				//
				a[0] = 1;
				a[1] = 0;
				a[2] = 0;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case 38:	// ^
        	pthread_mutex_lock(&mutex);
			panV(-8);
        	pthread_mutex_unlock(&mutex);
			break;
		case 40:	// v
        	pthread_mutex_lock(&mutex);
			panV(8);
        	pthread_mutex_unlock(&mutex);
			break;
		case 37:	// >>
        	pthread_mutex_lock(&mutex);
			panH(-8);
        	pthread_mutex_unlock(&mutex);
			break;
		case 39:	// <<
        	pthread_mutex_lock(&mutex);
			panH(8);
        	pthread_mutex_unlock(&mutex);
			break;
		case 33:	// PgUp
			if(isParallel())
			{
				expandWindow();
			}
			else
			{
				float position[3], direction[3], attitude[3];
				pthread_mutex_lock(&mutex);
				getCamera(position, direction, attitude);
				float z[3];
				z[0] = direction[0];
				z[1] = direction[1];
				z[2] = direction[2];
				scale3d(z, 60*(exp(depth)-exp(depth-0.1)));
				add3d(position, z);
				setCamera(position, direction, attitude);
				if(depth > 0.1)
				{
					depth -= 0.1;
				}
				pthread_mutex_unlock(&mutex);
			}
			break;
		case 34:	// PgDwn
			if(isParallel())
			{
				shrinkWindow();
			}
			else
			{
				float position[3], direction[3], attitude[3];
				pthread_mutex_lock(&mutex);
				getCamera(position, direction, attitude);
				float z[3];
				z[0] = direction[0];
				z[1] = direction[1];
				z[2] = direction[2];
				scale3d(z, 60*(exp(depth+0.1)-exp(depth)));
				sub3d(position, z);
				setCamera(position, direction, attitude);
				depth += 0.1;
				pthread_mutex_unlock(&mutex);
			}
			break;
		case ESC:
        	pthread_mutex_lock(&mutex);
			initEngine(DISTANCE);
        	pthread_mutex_unlock(&mutex);
			break;
	}
}
