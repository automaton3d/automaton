/*
 * utils.c
 *
 *  Created on: 10/10/2016
 *Author: Alexandre
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

/*
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
*/

double rnd()
{
	return rand()/(double)RAND_MAX;
}

int rndSign()
{
	return rnd()<0.5 ? -1 : +1;
}

double sign(double d)
{
	return d / fabs(d);
}

Tuple toTuple(Vector3d v)
{
	Tuple t;
	t.x = v.x;
	t.y = v.y;
	t.z = v.z;
	return t;
}

Vector3d toVec(Tuple t)
{
	Vector3d v;
	v.x = t.x;
	v.y = t.y;
	v.z = t.z;
	return v;
}

/*
 * Random point on sphere.
 *
 * Marsaglia (1972).
 */
Vector3d rndVector()
{
	double x1, x2;
	do
	{
		x1 = rnd();
		x2 = rnd();
	} while(x1*x1 + x2*x2 >= 1);
	//
	Vector3d v;
	v.x = 2*rndSign()*x1*sqrt(1-x1*x1-x2*x2);
	v.y = 2*rndSign()*x2*sqrt(1-x1*x1-x2*x2);
	v.z = rndSign()*(1 - 2*(x1*x1 + x2*x2));
	return v;
}

/*
 * Used to generate a random sequence of ints;
 */
void rndInt(int *array, int max)
{
	int cont = 0;
	LOOP:
	while(cont < max)
	{
		int n = rand() % max;
		for(int i = 0; i < cont; i++)
		{
			if(n == array[i])
				goto LOOP;
		}
		array[cont++] = n;
	}
}

Vector3d gravxpreon(Vector3d p0, Vector3d p1, Vector3d cen, double r)
{
	double cx = cen.x;
	double cy = cen.y;
	double cz = cen.z;

	double px = p0.x;
	double py = p0.y;
	double pz = p0.z;

	double vx = p1.x - px;
	double vy = p1.y - py;
	double vz = p1.z - pz;

	double A = vx * vx + vy * vy + vz * vz;
	double B = 2.0 * (px * vx + py * vy + pz * vz - vx * cx - vy * cy - vz * cz);
	double C = px * px - 2 * px * cx + cx * cx + py * py - 2 * py * cy + cy * cy +
	     pz * pz - 2 * pz * cz + cz * cz - r * r;
	//
	Vector3d solution;
	solution.x = solution.y = solution.z = 0;
	//
	// Discriminant
	//
	double D = B * B - 4 * A * C;
	if(D < 0)
		return solution;
	//
	double t1 = (-B - sqrt (D)) / (2.0 * A);
	solution.x = p0.x * (1 - t1) + t1 * p1.x;
	solution.y = p0.y * (1 - t1) + t1 * p1.y;
	solution.z = p0.z * (1 - t1) + t1 * p1.z;
	if(D == 0 || rand()%2 == 0)
	    return solution;
	//
	double t2 = (-B + sqrt(D)) / (2.0 * A);
	solution.x = p0.x * (1 - t2) + t2 * p1.x;
	solution.y = p0.y * (1 - t2) + t2 * p1.y;
	solution.z = p0.z * (1 - t2) + t2 * p1.z;
	return solution;
}

int string_length(char *s)
{
	int c = 0;
	while(s[c] != '\0')
		c++;
	return c;
}

void COPY(int *u, int *v)
{
	u[0] = v[0];
	u[1] = v[1];
	u[2] = v[2];
}
