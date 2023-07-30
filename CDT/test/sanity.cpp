/*
 * sanity.cpp
 *
 *  Created on: 11 de jul. de 2023
 *      Author: Alexandre
 */
#include <cstdlib> // Include the necessary header for malloc and free
#include "tests.h"

namespace automaton
{

/**
 * Consistency tests.
 */
bool sanity(Cell* grid)
{
  int ni = 0;
  long x=0, y=0, z=0;

  // Check orthogonality of momentum
  int n = 0;
  bool* paired = static_cast<bool*>(malloc(SIDE6 * sizeof(bool)));
  if (!paired)
    return false;
  for (int i = 0; i < SIDE6; i++)
    paired[i] = false;
  Cell* ptr1 = grid;
  for (int i = 0; i < SIDE6; i++, ptr1++)
  {
    if (!ZERO(ptr1->p))
    {
      Cell* ptr2 = grid + i + 1;
      for (int j = i + 1; j < SIDE6; j++, ptr2++)
      {
        if (paired[j])
          continue;
        if (!ZERO(ptr2->p) && ANTI(ptr1->p, ptr2->p))
        {
          if (!paired[i])
          {
            n++;
            paired[i] = true;
            paired[j] = true;
            break;
          }
        }
      }
      if(!paired[i])
      {
        printf("\ti=%d\n", i);
        printCell(ptr1);
        ni++;
        x += ptr1->p[0];
        y += ptr1->p[1];
        z += ptr1->p[2];
      }
    }
  }
  printf("TOT=%d\n", ni);
  printf("P: %ld,%ld,%ld\n", x, y, z);
  fflush(stdout);
  if (n != SIDE3)
  {
    free(paired);
    return false;
  }

  // Check spins
  n = 0;
  Cell *ptr = grid;
  for (int i = 0; i < SIDE6; i++)
  {
    if (!ZERO(ptr->s))
      n++;
    ptr++;
  }
  free(paired);
  framework::sound();
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
