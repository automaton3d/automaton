/*
 * test.h
 *
 *  Created on: 4 de mar. de 2023
 *      Author: Alexandre
 */

#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include <stdio.h>
#include "../simulation.h"

//#define TEST_TREE

void explore(int org[3], int level);
void test_expand();
Cell *getAddress(Cell *latt, int x, int y, int z);
void printCell(Cell *cell);
Cell *isSingular(Cell *latt);
Cell *huntFlash(Cell *latt);
int countMomentum(Cell *latt);
Cell *huntMomentum(Cell *latt);

#endif /* TEST_TEST_H_ */
