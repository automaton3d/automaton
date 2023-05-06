/*
 * gadget.h
 *
 *  Created on: 23 de set de 2019
 *      Author: Alexandre
 */

#ifndef GADGET_H_
#define GADGET_H_

#define FRONT		0
#define TRACK		1
#define MOMENTUM	2
#define MODE0		3
#define MODE1		4
#define MODE2		5
#define PLANE		6
#define CUBE		7

#define NTICKS		8

// Functions

void showCheckbox(int x, int y, char *label);
boolean testCheckbox(int x, int y);

#endif /* GADGET_H_ */
