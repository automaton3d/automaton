/*
 * mouse.c
 */

#include "plot3d.h"
#include "arcball.h"
#include "view.h"

boolean pan = false;
static int xx0, yy0;

extern View view;

//extern pthread_mutex_t mutex;

void m4MultVec(float *vf, float mat[4][4], float *vi)
{
	float tmp[4];
	for (int i = 0; i < 4; i++)
		tmp[i] = 0;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			tmp[j] += mat[j][i] * vi[i];
	for (int i = 0; i < 4; i++)
		vf[i] = tmp[i];
}

void project(float *vec, float x, float y)
{
      float r = 1;
      float res = min(WIDTH, HEIGHT) - 1;
      //
      // map to -1 to 1
      //
      x = (2 * x - WIDTH + 1) / res;
      y = (2 * y - HEIGHT + 1) / res;

      if (fabs(x) > LIMITX)
    	  y = 0;
      if (fabs(y) > LIMITY)
    	  x = 0;
      float dist2 = x * x + y * y;
      double z;
      if (dist2 <= r * r / 2)
        z = sqrt(r * r - dist2);
      else
        z = (r * r / 2) / sqrt(dist2);
      vec[0] = -x;
      vec[1] = -y;
      vec[2] = z;
}

void mouse(UINT msg, WPARAM wparam, LPARAM lparam)
{
	int delta;
	int x = LOWORD(lparam) - BMAPX;
	int y = HIWORD(lparam) - BMAPY;
	if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT)
		return;
	if (x < WIDTH / 4 || x > 3 * WIDTH / 4)
		y = HEIGHT / 2;
	else if (y < HEIGHT / 4 || y > 3 * HEIGHT / 4)
		x = WIDTH / 2;

//	pthread_mutex_lock(&mutex);
	switch(msg)
	{
		case WM_LBUTTONDOWN:
        {
   			mousedown(x, y);
        	break;
        }
		case WM_LBUTTONUP:
   			mouseup();
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
			if(view.dragged)
			{
				mousemove(x, y);
				mulQuat(view.currQ, view.lastQ, view.rotation);
			}
			break;
		case WM_MOUSEWHEEL:
			delta = GET_WHEEL_DELTA_WPARAM(wparam);
			zoom(delta);
			break;
	}
//	pthread_mutex_unlock(&mutex);
}
