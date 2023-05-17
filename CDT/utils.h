/*
 * utils.h
 *
 *  Created on: 11/10/2016
 *      Author: Alexandre
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "plot3d.h"
#include "utils.h"

#define true 	1
#define false 	0

#define ESC		27

#define PI		3.14159265358979323846

// Colors

#define BLK		0
#define NAVY	1
#define BLUE	2
#define MAROON	3
#define PURPLE	4
#define RED 	5
#define MAGENTA	6
#define GREEN	7
#define TEAL	8
#define OLIVE	9
#define GRAY	10
#define LIME	11
#define ORANGE	12
#define CYAN	13
#define YELLOW	14
#define WHT 	15
#define SILVER	16
#define PALE    17
#define VANILLA 18
#define BOX		PURPLE

#define NCOLORS	19

typedef struct { int x, y, z; } Tuple;

/// Functions ///

void delay(unsigned int mseconds);
int string_length(char *s);
int DOT(int *u, int *v);
int MOD2(int *v);
void scale3d(float *t, int s);
void cross3d(float v1[3], float v2[3], float *v3);
void normalize(float *v);
void reset3d(float *v);
void add3d(float *a, float b[3]);
void sub3d(float *a, float b[3]);

#endif /* UTILS_H_ */
