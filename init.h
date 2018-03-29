/*
 * init.h
 */

#ifndef INIT_H_
#define INIT_H_

#include "brick.h"

extern Brick *pri0, *dual0;
extern double K, U1, U2;
extern double wT;
extern unsigned long begin;
extern int timing;
extern int limit;
extern char imgbuf[3][SIDE3];
extern char *draft, *clean, *snap;
extern int scenario;

/// Functions ///

void initAutomaton();
Brick *addPreon(int x, int y, int z, int w, char p3, char p4, unsigned char p5, Tuple p6, int p7, int p17, int p19, unsigned schedule);
void createVacuum();

#endif /* INIT_H_ */
