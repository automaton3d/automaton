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
extern char imgbuf[3][SIDE3];
extern char *draft, *clean, *snap;
extern int scene;
extern const char *sceneNames[];
extern void (*scenarios[])();
extern int limit;


/// Functions ///

void initAutomaton();
Brick *addPreon(Tuple p0, int w, Tuple p3, Tuple p4, char p5, char p6, char p7, int p8, unsigned char p9, int p21, unsigned p24, int p25);

#endif /* INIT_H_ */
