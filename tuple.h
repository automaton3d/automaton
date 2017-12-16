/*
 * tuple.h
 *
 *  Created on: 13/01/2016
 *      Author: Alexandre
 */

#ifndef TUPLE_H_
#define TUPLE_H_

#include <windows.h>
#include "params.h"

typedef struct { int x, y, z; } Tuple;

extern Tuple dirs [6];

void initDirs();
void rectify(Tuple *v);
boolean isNull(Tuple t);
boolean isEqual(Tuple t1, Tuple t2);
boolean isOpposite(Tuple t1, Tuple t2);
void invertTuple(Tuple *t);
void addTuples(Tuple *a, Tuple b);
void subTuples(Tuple *a, Tuple b);
void subTuples3(Tuple *r, Tuple a, Tuple b);
double modTuple(Tuple *v);
double mod2Tuple(Tuple *v);
void normalizeTuple(Tuple *t);
char *tuple2str(Tuple *t);
void tupleCross(Tuple v1, Tuple v2, Tuple *v3);
int compareTuples(Tuple *a, Tuple *b);
int tupleDot(Tuple *a, Tuple *b);
void tupleAbs(Tuple *t);
void tupleCopy(Tuple *a, Tuple b);
void resetTuple(Tuple *t);
void scaleTuple(Tuple *t, int s);

#endif /* TUPLE_H_ */
