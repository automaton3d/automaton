/*
 * utils.c
 *
 *  Created on: 12 de jun. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

boolean isEmpty(Cell *c)
{
  return (c->a1==0 &&
      c->a2==0 &&
      c->syn==0 &&
      c->u==0 &&
      c->k==0 &&
      c->obj==SIDE3 &&
      c->occ==0 &&
      ZERO(c->p) &&
      ZERO(c->s) &&
      ISSAT(c->o) &&
      ISSAT(c->po) &&
      ZERO(c->pP) &&
      ISSAT(c->m));
}

void empty(Cell *c)
{
    c->a1  = 0;
    c->a2  = 0;
    c->syn = 0;
    c->occ = 0;
    c->u   = 0;
    c->obj = SIDE3;
    c->k   = NONE;
    RSET(c->p);
    RSET(c->s);
    SAT(c->o);
    RSET(c->pP);
    SAT(c->m);
    SAT(c->po);
}
