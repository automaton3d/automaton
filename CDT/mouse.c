/*
 * mouse.c
 */

#include "mouse.h"

boolean fDraw = false;
boolean input_changed = true;

static Vec3 pos, dir, att;

boolean pan = false;
static int xx0, yy0;

Quaternion currQ, lastQ, rotation;

int lastx, lasty;
boolean start = false;

extern View *vu;
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

void project(Vec3 *vec, float x, float y)
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
      vec->x = -x;
      vec->y = -y;
      vec->z = z;
}

void mouse(UINT msg, WPARAM wparam, LPARAM lparam)
{
	int delta;
	int y = HIWORD(lparam);
	int x = LOWORD(lparam);
	if (x < 300 || x > 300 + WIDTH || y > 600)
		return;
//	pthread_mutex_lock(&mutex);
	switch(msg)
	{
		case WM_LBUTTONDOWN:
        {
       		start = true;
       		lastx = x;
       		lasty = y;
           	fDraw = true;
   			getCamera(&pos, &dir, &att);

   			// TRACKBALL

   			vu->lastx = x;
   			vu->lasty = y;

   			float di[3];
   			di[0] = 1;
   			di[1] = 1;
   			di[2] = -1;
   			vnormal(di);
   			axis_to_quat(di, 0, vu->quat);
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
			if(start)
			{
				mulQ(&lastQ, currQ, lastQ);
				identityQ(&currQ);
				start = false;
			}
			pan = false;
			break;
		case WM_MOUSEMOVE:
			if(fDraw && !input_changed)
			{
				if(start)
				{
					Vec3 a;
					project(&a, lastx, lasty);
					normalize(&a);
					Vec3 b;
					project(&b, x, y);
					normalize(&b);
					fromBetweenVectors(&currQ, a, b);
					normalise(&currQ);
					mulQ(&rotation, currQ, lastQ);
					rotateVector(&dir, rotation, dir);
					normalize(&dir);
					//
					rotateVector(&att, rotation, att);
					normalize(&att);

					// TRACKBALL

				    float dquat[4];
				    trackball (dquat,
				    	(2.0*vu->lastx -         WIDTH) / WIDTH,
						(       HEIGHT - 2.0*vu->lasty) / HEIGHT,
						(        2.0*x -         WIDTH) / WIDTH,
						(       HEIGHT -         2.0*y) / HEIGHT);

				    vu->dx = x - vu->lastx;
				    vu->dy = y - vu->lasty;
				    add_quats (dquat, vu->quat, vu->quat);
				    float mat[4][4];
				    build_rotmatrix(mat, dquat);
				    float di[4], at[4];
				    di[0] = dir.x;
				    di[1] = dir.y;
				    di[2] = dir.z;
				    at[0] = att.x;
				    at[1] = att.y;
				    at[2] = att.z;
				    m4MultVec(di, mat, di);
				    m4MultVec(at, mat, at);
					//
				    dir.x = di[0];
				    dir.y = di[1];
				    dir.z = di[2];
				    att.x = at[0];
				    att.y = at[1];
				    att.z = at[2];
				    //
					double hy = sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
					pos.x = -dir.x * hy;
					pos.y = -dir.y * hy;
					pos.z = -dir.z * hy;
					setCamera(pos, dir, att);
					lastx = x; lasty = y;

					// TRACKBALL

					vu->lastx = x; vu->lasty = y;
				}
				input_changed = true;
			}
			break;
		case WM_MOUSEWHEEL:
			delta = GET_WHEEL_DELTA_WPARAM(wparam);
			zoom(delta);
			break;
	}
//	pthread_mutex_unlock(&mutex);
}
