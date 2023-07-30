/*
 * tests.h
 *
 *  Created on: 11 de jul. de 2023
 *      Author: Alexandre
 */

#ifndef TESTS_TESTS_H_
#define TESTS_TESTS_H_

#include <iostream>
#include "window.h"

namespace automaton
{
  bool sanity(Cell *grid);
  bool alignment(Cell *grid);
  void printCell(Cell *cell);
}

#endif /* TESTS_TESTS_H_ */
