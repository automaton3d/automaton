#include "tests.h"

namespace automaton
{
/*
 * sanity.cpp
 *
 *  Created on: 11 de jul. de 2023
 *      Author: Alexandre
 */
bool sanity(Cell *grid)
{
  // Check othogonality of momentum

  int n = 0;
  bool paired[SIDE6] = {false}; // Initialize flag array
  Cell *ptr1 = grid;
  for (int i = 0; i < SIDE6; i++, ptr1++)
  {
    Cell *ptr2 = grid + i + 1;
    for (int j = i + 1; j < SIDE6; j++, ptr2++)
    {
      if (!ZERO(ptr1->p) && !ZERO(ptr2->p) && ANTI(ptr1->p, ptr2->p))
      {
        // Check if ptr1 and ptr2 have not been paired yet
        if (!paired[i] && !paired[j])
        {
          n++;
          paired[i] = true; // Mark ptr1 as paired
          paired[j] = true; // Mark ptr2 as paired
          break;
        }
      }
    }
  }

  /*

  It not possible to antialign these vectors:
  5,6,0
  6,5,1
  6,5,-1
  5,5,2
  5,5,-2
  5,5,3
  5,5,-3
  -5,6,0
  */

  // Check spins

  n = 0;
  Cell *ptr = grid;
  for (int i = 0; i < SIDE6; i++)
  {
    if(!ZERO(ptr->s))
      n++;
    ptr++;
  }
  return n == SIDE3;
}

bool alignment(Cell *grid)
{
  int limit = (int)( 5.3491902637676 * exp(1.42641924824217 * ORDER) + 0.5);
  int n = 0;
  Cell *ptr1 = grid;
  for (int i = 0; i < SIDE6; i++)
  {
    if(!ZERO(ptr1->s))
    {
      Cell *ptr2 = grid;
      for (int j = i; j < SIDE6; j++)
   	  {
   	    if(!ZERO(ptr2->s))
   	    {
   	      if (antialigned(ptr1->s, ptr2->s))
   	      {
   	    	  n++;
   	      }
   	    }
   	    ptr2++;
   	  }
    }
    ptr1++;
  }
  return n == limit;
}

}
