/*
 * utils.c
 */

#include "utils.h"
#include <math.h>
#include "jpeg.h"

double wT = 2 * M_PI / SIDE;
__constant__ double K, U1, U2;

__device__ int opposite(int dir)
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
__device__ boolean voronoi(Vector3d probe, Tuple cell)
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

__device__ int pwm(int n)	// boolean
{
    return (n % STEP) < (n / NSTEPS);
}

__device__ void resetDFO(Brick *t)
{
	t->p12 = 0;
	t->p28a1 = U1;
	t->p28a2 = U2;
}

__device__ void incrDFO(Brick *t)
{
	t->p12++;
	int u3 = K * t->p28a2 - t->p28a1;
	t->p28a1 = t->p28a2;
	t->p28a2 = u3;
}

/*
 * Generates a prime number closest to n.
 */
int getPrime(unsigned n)
{
   int i = 3, count = 2, c, prime = 0;

   while(i < SIDE)
   {
      for(c = 2 ; c <= i - 1 ; c++)
      {
         if(i % c == 0)
            break;
      }
      if(c == i)
      {
	  prime = i;
	  count++;
      }
      i++;
   }
   return prime;
}

__device__ int hash(int n)
{
	return ((n + 1) * d_prime) >> (ORDER/2) & (SIDE-1);
}


