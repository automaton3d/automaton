/*
 * automaton.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef AUTOMATON_H_
#define AUTOMATON_H_

#include <pthread.h>
#include "tile.h"

extern Tile *pri0;
extern unsigned long timer;
//extern char voxels[], _voxels[];
extern pthread_mutex_t mutex;
extern char imgbuf[3][SIDE3];
extern char *draft, *clean;

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
void delay(unsigned int mseconds);
int sgn(int n);
Tile *getNual(int dir);
void reemit();
void burst(boolean cp);

#endif /* AUTOMATON_H_ */
