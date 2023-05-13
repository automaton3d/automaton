/*
 * bresemham.c
 *
 *  Created on: 21 de jan de 2021
 *      Author: Alexandre
 */

#include "engine.h"

static void drawCircle(int xc, int yc, int x, int y, char color)
{
    putPixel(xc+x, yc+y, color);
    putPixel(xc-x, yc+y, color);
    putPixel(xc+x, yc-y, color);
    putPixel(xc-x, yc-y, color);
    putPixel(xc+y, yc+x, color);
    putPixel(xc-y, yc+x, color);
    putPixel(xc+y, yc-x, color);
    putPixel(xc-y, yc-x, color);
}

void circle2d(int xc, int yc, int r, char color)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    drawCircle(xc, yc, x, y, color);
    while (y >= x)
    {
        x++;
        if(d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        drawCircle(xc, yc, x, y, color);
    }
}

void line3d(Vec3 t1, Vec3 t2, char color)
{
    int i, err_1, err_2, dx2, dy2, dz2;
    Vec3 point = t1;
    int dx = (int)(t2.x - t1.x);
    int dy = (int)(t2.y - t1.y);
    int dz = (int)(t2.z - t1.z);
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
                point.y += y_inc;
                err_1 -= dx2;
            }
            if (err_2 > 0) {
                point.z += z_inc;
                err_2 -= dx2;
            }
            err_1 += dy2;
            err_2 += dz2;
            point.x += x_inc;
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
                point.x += x_inc;
                err_1 -= dy2;
            }
            if (err_2 > 0)
            {
                point.z += z_inc;
                err_2 -= dy2;
            }
            err_1 += dx2;
            err_2 += dz2;
            point.y += y_inc;
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
                point.y += y_inc;
                err_1 -= dz2;
            }
            if (err_2 > 0)
            {
                point.x += x_inc;
                err_2 -= dz2;
            }
            err_1 += dy2;
            err_2 += dx2;
            point.z += z_inc;
        }
    }
    putVoxel(point, color);
}
