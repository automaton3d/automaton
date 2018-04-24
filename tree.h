/*
 * tree.h
  */

#ifndef TREE_H_
#define TREE_H_

#include "tuple.h"

extern const Tuple dirs[];

/// Functions ///

boolean isAllowed(int dir, Tuple p, unsigned char d0);

#endif /* TREE_H_ */
