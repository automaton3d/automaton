/*
 * utils.c
 *
 *  Created on: 12 de jun. de 2023
 *      Author: Alexandre
 */

#include "simulation.h"

extern Cell *drf;

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
    RSET(drf->p);
    RSET(drf->s);
    drf->a1 = 0;
    drf->a2 = 0;
    SAT(drf->o);
    drf->syn = 0;
    drf->occ = 0;
    drf->u = 0;
    RSET(drf->pP);
    SAT(drf->m);
    SAT(drf->po);
    drf->obj = SIDE3;
    drf->k = NONE;
}

/*
int getrole(Cell *c)
{
	if (!ZERO(c->p) && ZERO(c->po))
  		return SEED;

	if (!ZERO(c->s) && ZERO(c->po))
  		return WAVE;

	if (!ISSAT(c->o) && ISSAT(c->po))
  		return GRID;

	if ((!ZERO(c->po) && !ISSAT(c->po)) || (c)->obj<SIDE3)
  		return TRVLLR;

	if (ISSAT(c->o) && ISSAT(c->po) && c->obj==SIDE3)
  		return EMPTY;
	assert(0);
	return -1;
}
 */
