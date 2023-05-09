/*
 * keyboard.c
 *
 *  Created on: 24 de jul de 2017
 *      Author: Alexandre
 */

#include <math.h>
#include <pthread.h>
#include "common.h"
#include "engine.h"
#include "keyboard.h"
#include "plot3d.h"

extern boolean showAxes;
extern boolean showModel;
extern boolean verbose;
extern boolean input_changed;
extern char gridcolor;
extern pthread_mutex_t mutex;

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
	    	pthread_mutex_lock(&mutex);
			input_changed = true;
	    	pthread_mutex_unlock(&mutex);
			break;
		case 'M':
			showModel = !showModel;
	    	pthread_mutex_lock(&mutex);
			input_changed = true;
	    	pthread_mutex_unlock(&mutex);
			break;
		case 'S':
			stop = !stop;
	    	pthread_mutex_lock(&mutex);
			input_changed = true;
	    	pthread_mutex_unlock(&mutex);
			break;
		case 'V':
			verbose = !verbose;
		    break;
		case '0':
			{
	        	pthread_mutex_lock(&mutex);
				Vec3 p, d, a, xaxis;
				getCamera(&p, &d, &a);
				cross3d(d, a, &xaxis);
				p.x = sqrt(3);
				p.y = sqrt(3);
				p.z = sqrt(3);
				normalize(&p);
				d = p;
				scale3d(&p, 44);
				scale3d(&d, -1);
				cross3d(xaxis, d, &a);
				setCamera(p, d, a);
				input_changed = true;
	        	pthread_mutex_unlock(&mutex);
			}
			break;

		case '1':
			{
				Vec3 p, d, a;
				p.x = 44;
				p.y = 0;
				p.z = 0;
				//
				d.x = -1;
				d.y = 0;
				d.z = 0;
				//
				a.x = 0;
				a.y = 0;
				a.z = -1;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
				input_changed = true;
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case '2':
			{
				Vec3 p, d, a;
				p.x = 0;
				p.y = 44;
				p.z = 0;
				//
				d.x = 0;
				d.y = -1;
				d.z = 0;
				a.x = 0;
				//
				a.y = 0;
				a.z = 1;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
				input_changed = true;
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case '3':
			{
				Vec3 p, d, a;
				p.x = 0;
				p.y = 0;
				p.z = 44;
				//
				d.x = 0;
				d.y = 0;
				d.z = -1;
				//
				a.x = 1;
				a.y = 0;
				a.z = 0;
	        	pthread_mutex_lock(&mutex);
				setCamera(p, d, a);
				input_changed = true;
	        	pthread_mutex_unlock(&mutex);
			}
			break;
		case 38:	// ^
        	pthread_mutex_lock(&mutex);
			panV(-8);
			input_changed = true;
        	pthread_mutex_unlock(&mutex);
			break;
		case 40:	// v
        	pthread_mutex_lock(&mutex);
			panV(8);
			input_changed = true;
        	pthread_mutex_unlock(&mutex);
			break;
		case 37:	// >>
        	pthread_mutex_lock(&mutex);
			panH(-8);
			input_changed = true;
        	pthread_mutex_unlock(&mutex);
			break;
		case 39:	// <<
        	pthread_mutex_lock(&mutex);
			panH(8);
			input_changed = true;
        	pthread_mutex_unlock(&mutex);
			break;
		case 33:	// PgUp
			if(isParallel())
			{
				expandWindow();
				pthread_mutex_lock(&mutex);
				input_changed = true;
				pthread_mutex_unlock(&mutex);
			}
			else
			{
				Vec3 position, direction, attitude;
				pthread_mutex_lock(&mutex);
				getCamera(&position, &direction, &attitude);
				Vec3 z = direction;
				scale3d(&z, 60*(exp(depth)-exp(depth-0.1)));
				add3d(&position, z);
				setCamera(position, direction, attitude);
				if(depth > 0.1)
				{
					depth -= 0.1;
					input_changed = true;
				}
				pthread_mutex_unlock(&mutex);
			}
			break;
		case 34:	// PgDwn
			if(isParallel())
			{
				shrinkWindow();
				input_changed = true;
			}
			else
			{
				Vec3 position, direction, attitude;
				pthread_mutex_lock(&mutex);
				getCamera(&position, &direction, &attitude);
				Vec3 z = direction;
				scale3d(&z, 60*(exp(depth+0.1)-exp(depth)));
				sub3d(&position, z);
				setCamera(position, direction, attitude);
				depth += 0.1;
				input_changed = true;
				pthread_mutex_unlock(&mutex);
			}
			break;
		case ESC:
        	pthread_mutex_lock(&mutex);
			initEngine(DISTANCE);
			input_changed = true;
        	pthread_mutex_unlock(&mutex);
			break;
	}
}
