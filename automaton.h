/*
 * automaton.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef AUTOMATON_H_
#define AUTOMATON_H_

#include "brick.h"

// Exported variables

extern Brick *pri0, *dual0;
extern Brick *pri, *dual;
extern int limit, period;

/// Macros ///

#define virt(b)			(!(b->g==b->d))
#define leptonic(b)		(b->R==b->G&&b->G==b->B)

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
Brick *getNual(int dir);

#endif /* AUTOMATON_H_ */
