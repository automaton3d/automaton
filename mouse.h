/*
 * mouse.h
 *
 *  Created on: 24 de jul de 2017
 *      Author: Alexandre
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include <windows.h>

// Exported variables

extern boolean input_changed;

/// Functions ///

void mouse(char op, int x, int y);

#endif /* MOUSE_H_ */
