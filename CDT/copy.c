////////////////////////////////////
//                                //
// -- Software of the universe -- //
//              v0.1              //
//                                //
////////////////////////////////////

#include "simulation.h"

// Current pointers

extern Cell *stb, *drf;

extern pthread_mutex_t mutex;

/**
 * Copy data.
 */
void copy()
{
  pthread_mutex_lock(&mutex);

  // Lattice update

  stb->ch  = drf->ch;   // charges
  stb->a1   = drf->a1;    // affinity
  stb->n   = drf->n;    // ticks
  stb->syn = drf->syn;  // wf synch
  stb->u   = drf->u;    // sine
  stb->pmf = drf->pmf;  // sine PMF
  stb->k   = drf->k;    // kind
  stb->occ = drf->occ;  // occupancy
  stb->obj = drf->obj;  // target
  CP(stb->p, drf->p);   // momentum
  CP(stb->s, drf->s);   // spin
  CP(stb->o, drf->o);   // bubble origin
  CP(stb->po, drf->po); // pole
  CP(stb->pP, drf->pP); // decay
  CP(stb->m, drf->m);   // messenger

  // Eq. 4

  stb->pow = drf->pow;
  stb->den = drf->den;

  pthread_mutex_unlock(&mutex);
}
