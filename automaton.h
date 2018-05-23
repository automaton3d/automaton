/*
 * automaton.h
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
extern const Tuple dirs[];
extern char orgcolor;
extern boolean showAxes, showGrid, showOrgs;
extern boolean img_changed;

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
boolean isP(Brick *t1, Brick *t2);

#endif /* AUTOMATON_H_ */
