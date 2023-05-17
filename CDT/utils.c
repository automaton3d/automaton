/*
 * utils.c
 *
 * Created on: 10/10/2016
 * Author: Alexandre
 */

#include "utils.h"

/*
 * Simple delay for testing purposes.
 */
void delay(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
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
 * Calculates the length of string.
 */
int string_length(char *s)
{
	int c = 0;
	while(s[c] != '\0')
		c++;
	return c;
}

/*
 * Dot product.
 */
int DOT(int *u, int *v)
{
	return u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
}

/*
 * Module squared.
 */
int MOD2(int *v)
{
	return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
}

/*
 * Calculates conections between cells.
 */
Cell *wires(int i, Cell *ptr, int dir, Cell *latt)
{
  int x = ((i / SIDE) % SIDE) % SIDE;
  int y = (((i / SIDE) % SIDE) / SIDE) % SIDE;
  int z = ((i / SIDE) % SIDE) / (SIDE2);

  switch (dir)
  {
    case 0:
      x = (x + 1) % SIDE;
      break;
    case 1:
      x = (x - 1 + SIDE) % SIDE;
      break;
    case 2:
      y = (y + 1) % SIDE;
      break;
    case 3:
      y = (y - 1 + SIDE) % SIDE;
      break;
    case 4:
      z = (z + 1) % SIDE;
      break;
    case 5:
      z = (z - 1 + SIDE) % SIDE;
      break;
  }
  return latt + (z * SIDE * SIDE + y * SIDE + x) * SIDE3 + (ptr->off % SIDE3);
}

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

void reset3d(float *v)
{
	v[0] = 0;
	v[1] = 0;
	v[2] = 0;
}
