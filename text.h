/*
 * text.h
 */

#ifndef TEXT_H_
#define TEXT_H_

#include "brick.h"

extern Brick *pri0;
extern unsigned long timer;
extern DWORD colors[];
extern void vprints(int x, int y, char *str);

/// Functions ///

void vprint(int x, int y, char ascii);
void printLattice();

#endif /* TEXT_H_ */
