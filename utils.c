/*
 * utils.c
 *
 *  Created on: 10/10/2016
 *      Author: Alexandre
 */

#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "tuple.h"
#include "vector3d.h"

/*
 * Calculates the signum function.
 */
int signum(int a)
{
	if(a > 0)
		return 1;
	if(a < 0)
		return -1;
	return 0;
}

/*
 * Voronoi cell intersection test.
 */
BOOL voronoi(double x, double y, double z, Tuple *cell)
{
	Vector3d min;
	min.x = cell->x - 0.5;
	min.y = cell->y - 0.5;
	min.z = cell->z - 0.5;
	Vector3d max;
	max.x = cell->x + 0.5;
	max.y = cell->y + 0.5;
	max.z = cell->z + 0.5;
	//
	double EPSILON = 1e-300;
	Vector3d d;
	d.x = x * 0.5;
	d.y = y * 0.5;
	d.z = z * 0.5;
	Vector3d e;
	e.x = (max.x - min.x) * 0.5;
	e.y = (max.y - min.y) * 0.5;
	e.z = (max.z - min.z) * 0.5;
	Vector3d c;
	c.x = d.x - (min.x + max.x) * 0.5;
	c.y = d.y - (min.y + max.y) * 0.5;
	c.z = d.z - (min.z + max.z) * 0.5;
	Vector3d ad;
	ad.x = d.x;
	ad.y = d.y;
	ad.z = d.z;
	absV3d(&ad);
	if(abs(c.x) > e.x + ad.x)
		return FALSE;
	if(abs(c.y) > e.y + ad.y)
		return FALSE;
	if(abs(c.z) > e.z + ad.z)
		return FALSE;
	if(abs(d.y * c.z - d.z * c.y) > e.y * ad.z + e.z * ad.y + EPSILON)
		return FALSE;
	if(abs(d.z * c.x - d.x * c.z) > e.z * ad.x + e.x * ad.z + EPSILON)
		return FALSE;
	if(abs(d.x * c.y - d.y * c.x) > e.x * ad.y + e.y * ad.x + EPSILON)
		return FALSE;
    return TRUE;
}





