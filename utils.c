/*
 * utils.c
 *
 *  Created on: 10/10/2016
 *      Author: Alexandre
 */

#include "utils.h"
#include <math.h>
#include "plot3d.h"

void delay(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

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
    return (n % ROOT) < (n / ROOT);
}

/*
 * Integer square root.
 */
unsigned sqr(unsigned long n)
{
    unsigned int c = SIDE/2;
    unsigned int g = SIDE/2;
    for(;;)
    {
        if(g*g > n)
            g ^= c;
        c >>= 1;
        if(c == 0)
            return g;
        g |= c;
    }
    return 0;
}
