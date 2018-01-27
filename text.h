/*
 * text.h
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#ifndef TEXT_H_
#define TEXT_H_

#include "tile.h"

extern Tile *pri0;
extern unsigned long timer;
extern DWORD colors[];
extern void vprints(int x, int y, char *str);

/// Functions ///

void vprint(int x, int y, char ascii);
void printLattice();

#endif /* TEXT_H_ */
