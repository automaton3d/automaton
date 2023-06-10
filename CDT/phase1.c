////////////////////////////////////
//                                //
// -- Software of the universe -- //
//              v0.2              //
//                                //
////////////////////////////////////

#include "simulation.h"

// Current pointers

extern Cell *stb, *drf;

/**
 * Copy data.
 * (massive transfer between lattices)
 */
void transfer()
{
  // Clock tick.

  drf->n++;

  // Update occupation status.

  if (drf->occ > 0)
    drf->occ--;

  // Massive copy.

  stb->ch  = drf->ch;   // charges
  stb->a1   = drf->a1;  // affinity
  stb->a2   = drf->a2;  // affinity
  stb->n   = drf->n;    // ticks
  stb->syn = drf->syn;  // wf synch
  stb->u   = drf->u;    // sine
  stb->k   = drf->k;    // kind
  stb->obj = drf->obj;  // target
  stb->occ = drf->occ;  // occupancy
  CP(stb->p, drf->p);   // momentum
  CP(stb->s, drf->s);   // spin
  CP(stb->o, drf->o);   // bubble origin
  CP(stb->po, drf->po); // pole
  CP(stb->pP, drf->pP); // decay
  CP(stb->m, drf->m);   // messenger
}
