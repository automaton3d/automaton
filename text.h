/*
 * text.h
 */

#ifndef TEXT_H_
#define TEXT_H_

#include "brick.h"

extern Brick *pri0;
extern unsigned long timer;
extern DWORD colors[];

/// Functions ///

void vprints(int x, int y, char *str);
void vprint(int x, int y, char ascii);
void printLattice();

#endif /* TEXT_H_ */
