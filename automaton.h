/*
 * automaton.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef AUTOMATON_H_
#define AUTOMATON_H_

#include <pthread.h>

#include "brick.h"

extern Brick *pri0;
extern unsigned long timer;
extern pthread_mutex_t mutex;
extern char imgbuf[3][SIDE3];
extern char *draft, *clean;
extern Tuple V0;

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
Brick *getNual(int dir);

#endif /* AUTOMATON_H_ */
