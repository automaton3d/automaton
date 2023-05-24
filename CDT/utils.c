/*
 * utils.c
 *
 * Created on: 10/10/2016
 * Author: Alexandre
 */

#include "utils.h"

void scale3d(float *t, int s)
{
	t[0] *= s;
	t[1] *= s;
	t[2] *= s;
}

void cross3d(float v1[3], float v2[3], float *v3)
{
	v3[0] = v1[1] * v2[2] - v1[2] * v2[1];
	v3[1] = v1[2] * v2[0] - v1[0] * v2[2];
	v3[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void normalize(float *v)
{
	double h = sqrt(v[0] * v[0]+ v[1] * v[1] + v[2] * v[2]);
	if(h == 0.0)
	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}
	else
	{
		v[0] /= h;
		v[1] /= h;
		v[2] /= h;
	}
}

void add3d(float *a, float b[3])
{
	a[0] += b[0];
	a[1] += b[1];
	a[2] += b[2];
}

void sub3d(float *a, float b[3])
{
	a[0] -= b[0];
	a[1] -= b[1];
	a[2] -= b[2];
}

void line3d(float t1[3], float t2[3], char color)
{
    int i, err_1, err_2, dx2, dy2, dz2;
    float point[3];
    point[0] = t1[0];
    point[1] = t1[1];
    point[2] = t1[2];
    int dx = (int)(t2[0] - t1[0]);
    int dy = (int)(t2[1] - t1[1]);
    int dz = (int)(t2[2] - t1[2]);
    int x_inc = (dx < 0) ? -1 : 1;
    int l = abs(dx);
    int y_inc = (dy < 0) ? -1 : 1;
    int m = abs(dy);
    int z_inc = (dz < 0) ? -1 : 1;
    int n = abs(dz);
    dx2 = l << 1;
    dy2 = m << 1;
    dz2 = n << 1;
    //
    if((l >= m) && (l >= n))
    {
        err_1 = dy2 - l;
        err_2 = dz2 - l;
        for (i = 0; i < l; i++)
        {
            putVoxel(point, color);
            if (err_1 > 0) {
                point[1] += y_inc;
                err_1 -= dx2;
            }
            if (err_2 > 0) {
                point[2] += z_inc;
                err_2 -= dx2;
            }
            err_1 += dy2;
            err_2 += dz2;
            point[0] += x_inc;
        }
    }
    else if ((m >= l) && (m >= n))
    {
        err_1 = dx2 - m;
        err_2 = dz2 - m;
        for (i = 0; i < m; i++)
        {
            putVoxel(point, color);
            if (err_1 > 0)
            {
                point[0] += x_inc;
                err_1 -= dy2;
            }
            if (err_2 > 0)
            {
                point[2] += z_inc;
                err_2 -= dy2;
            }
            err_1 += dx2;
            err_2 += dz2;
            point[1] += y_inc;
        }
    }
    else
    {
        err_1 = dy2 - n;
        err_2 = dx2 - n;
        for (i = 0; i < n; i++)
        {
            putVoxel(point, color);
            if (err_1 > 0)
            {
                point[1] += y_inc;
                err_1 -= dz2;
            }
            if (err_2 > 0)
            {
                point[0] += x_inc;
                err_2 -= dz2;
            }
            err_1 += dy2;
            err_2 += dx2;
            point[2] += z_inc;
        }
    }
    putVoxel(point, color);
}
