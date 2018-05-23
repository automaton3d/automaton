/*
 * common.h
 */

#ifndef COMMON_H_
#define COMMON_H_

#define true 1
#define false 0
#define null 0

#define WIDTH   800
#define HEIGHT  800

// Colors

#define BLK     0
#define WHT     1
#define RR		2
#define GG		3
#define BB		4
#define GRAY    5
#define BOX		6

// Special keys

#define ESC		27

// Macros

#define pwm(n) (n%SQRT < n/SQRT)

// Other symbols

#define NSCENES	20

#endif /* COMMON_H_ */
