/*
 * tuple.h
 *
 * Discrete vectors management.
 *
 *  Created on: 13/01/2016
 *      Author: Alexandre
 */

#ifndef TUPLE_H_
#define TUPLE_H_

#include <windows.h>
#include "params.h"

typedef struct { int x, y, z; } Tuple;

// Exported variables

extern Tuple V0;
extern const Tuple dirs[];

/// Functions ///

boolean isNull(Tuple t);
boolean isEqual(Tuple t1, Tuple t2);
boolean isOpposite(Tuple t1, Tuple t2);
void invertTuple(Tuple *t);
void addTuples(Tuple *a, Tuple b);
void addRectify(Tuple *a, Tuple b);
void subTuples(Tuple *a, Tuple b);
void subTuples3(Tuple *r, Tuple a, Tuple b);
void subRectify(Tuple *a, Tuple b);
void resetTuple(Tuple *t);
void scaleTuple(Tuple *t, int s);
void tupleCross(Tuple v1, Tuple v2, Tuple *v3);
unsigned imod(Tuple v);
unsigned imod2(Tuple v);
double modTuple(Tuple *v);
double mod2Tuple(Tuple *v);
int tupleDot(Tuple *a, Tuple *b);
int dot(Tuple t1, Tuple t2);
char *tuple2str(Tuple *t);

#endif /* TUPLE_H_ */
