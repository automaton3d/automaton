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
