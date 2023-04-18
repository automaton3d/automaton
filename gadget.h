/*
 * gadget.h
 *
 *  Created on: 23 de set de 2019
 *      Author: Alexandre
 */

#ifndef GADGET_H_
#define GADGET_H_

#define FRONT		0
#define MESSENGER	1
#define SPIN		2
#define MOMENTUM	3
#define MODE0		4
#define MODE1		5
#define MODE2		6
#define PLANE		7
#define CUBE		8

#define NTICKS		9

// Functions

void showCheckbox(int x, int y, char *label);
boolean testCheckbox(int x, int y);

#endif /* GADGET_H_ */
