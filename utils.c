/*
 * utils.c
 *
 *  Created on: 10/10/2016
 *      Author: Alexandre
 */

#include "utils.h"
#include <math.h>
#include "plot3d.h"
#include "rotation.h"

double wT = 2 * PI / SIDE;
double K, U1, U2;

int opposite(int dir)
{
	if(dir % 2 == 0)
		return dir + 1;
	else
		return dir - 1;
}

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

double distance3d(Vector3d v, Vector3d b)
{
	Vector3d ab, av, bv, prod;
	add3d(&ab, b);
	add3d(&av, v);
	if(dot3d(av, ab) <= 0)
	     return module3d(&av);
	add3d(&bv, v);
	sub3d(&bv, b);
	if(dot3d(bv, ab) >= 0.0)
	     return module3d(&bv);
	cross3d(ab, av, &prod);
	return module3d(&prod) / module3d(&ab);
}

/*
 * Voronoi cell intersection test.
 */
boolean voronoi(Vector3d probe, Tuple cell)
{
	return fabs(cell.x - probe.x) <= 0.5 && fabs(cell.y - probe.y) <= 0.5 && fabs(cell.z - probe.z) <= 0.5;
}

unsigned rndCoord()
{
	return (SIDE * (long) rand()) / RAND_MAX;
}

int rndSignal()
{
	return rand() < RAND_MAX / 2 ? -1 : +1;
}

boolean pwm(int n)
{
    return (n % STEP) < (n / NSTEPS);
}

void resetDFO(Brick *t)
{
	t->p11 = 0;
	t->p27a1 = U1;
	t->p27a2 = U2;
}

void incrDFO(Brick *t)
{
	t->p11++;
	int u3 = K * t->p27a2 - t->p27a1;
	t->p27a1 = t->p27a2;
	t->p27a2 = u3;
}
