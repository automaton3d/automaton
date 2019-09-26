/*
 * gadget.h
 *
 *  Created on: 23 de set de 2019
 *      Author: Alexandre
 */

#ifndef GADGET_H_
#define GADGET_H_

// Exported variables

extern boolean ticks[2];

// Functions

void showCheckbox(int x, int y, char *label);
void checkTick(int x, int y);

#endif /* GADGET_H_ */
