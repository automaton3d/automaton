/*
 * model.c
 */

#include "simulation.h"

extern Cell *stb, *drf;

extern int np;

void empty(Cell *c)
{
  RSET(drf->p);
  RSET(drf->s);
  SAT(drf->o);
  drf->syn = 0;
}

/**
 * Copy data.
 * (massive transfer between lattices)
 */
void transfer()
{
  // Clock tick.

  drf->n++;

  // Massive copy.

  stb->n   = drf->n;    // ticks
  stb->syn = drf->syn;  // wf synch
  CP(stb->p, drf->p);   // momentum
  CP(stb->s, drf->s);   // spin
  CP(stb->o, drf->o);   // bubble origin



  if (!ZERO(stb->p))
    np++;
}

/*
 * Information spreading.
 */
void spread()
{
  int role;
  if(!ZERO(stb->p))
    role = SEED;
  else if(!ZERO(stb->s))
    role = WAVE;
  else
    role =  EMPTY;

  // If cell is empty, does nothing.

  if(role == EMPTY)
    return;
  int code = 0;
  if (stb->n * stb->n < stb->syn)
    code |= RAW_IN;

  // Test wrapping.

  Cell *resort = NULL;

  // Spread information.

  for(int dir = 0; dir < NDIR; dir++)
  {
    // Calculate the address of the next neighbor.

    Cell* nei = neighbor(drf, dir);

    // The first part of this block propagates info
    // asynchronously (TRAVELLER processing).

    int axis = dir >> 1;

    // The second part of this block deals with
    // spherical synchronization alone (SEED and WAVE processing).

    // Is wavefront synchronized?
    // (see Ref. [14])

    if (code & RAW_IN)
      continue;

    // Calculate new origin vector.

    int org[3];
    CP(org, stb->o);
    org[axis] += (dir % 2 == 0) ? +1 : -1;

    // Spatially illegal move?

    if (abs(org[axis]) < abs(stb->o[axis]) || (abs(org[axis]) > SIDE_2))
      continue;

    // Ultimate cell?

    if (abs(org[0]) == SIDE_2 && abs(org[1]) == SIDE_2 && abs(org[2]) == SIDE_2)
    {
      if (role == SEED)
      {
        // Reissue from the ultimate cell.

    	nei->n   = 0;
    	nei->syn = 0;            // ready for immediate expansion 1
        RSET(nei->o);	         // ready for immediate expansion 2
    	CP(nei->p, stb->p);      // momentum
        CP(nei->s, stb->s);      // spin

    	code |= WRAP_OUT;
        continue;
      }
      else
      {
        // Not bearing momentum.

        code |= CLASH_OUT;
        continue;
      }
    }

    // Transmit superluminal info

    nei->n   = stb->n;    // ticks

    // Propagate exclusive spherical info.

    CP(nei->s, stb->s);   // spin
    CP(nei->o, org);      // bubble origin
    RSET(nei->p);         // default for momentum

    // Spherical synchronism.

    nei->syn = LIGHT2 * MAG(org);

    // From now on consider momentum itself.

    if (role != SEED)
    {
      code |= WF_OUT;
      continue;
    }

    // Keep a pointer to candidate cell to receive
    // momentum, in case the alignment test below fails.

    resort = nei;

    // Select momentum destination (Sect. 3.1).
    // Test alignment of |o> with |s>.

    if ((code & TX_OUT) == 0 &&
        DOT(org, stb->s)*DOT(org, stb->s) == MAG(org)*MAG(stb->s))
    {
      CP(nei->p, stb->p);  // propagate momentum
      code |= TX_OUT;
    }

  } //// End of for loop ////

  // Traveller processing.

  if (code & (POLE_OUT | TRAV_OUT | VISIT_OUT))
  {
    // Always clean left cell.

    empty(drf);
  }

  // Test maturity.

  else if (code & RAW_IN)
  {
    // Wavefront not ready, do nothing.
  }

  // Did the bubble collapse in on itself?

  else if (code & WRAP_OUT)
  {
    empty(drf);
  }

  // Momentum transfer concluded?

  else if (code & TX_OUT)
  {
    // Per definition of GRID:

    RSET(drf->p);     // no seed
    RSET(drf->s);     // no wavefront
    empty(drf);//debug
  }

  // Appeal to the last resort to avoid momentum loss.

  else if (role == SEED)
  {
    // Transfer momentum forcibly.

    assert(resort != NULL);

    CP(resort->p, stb->p);

    // Clean up the abandoned cell.

    empty(drf);
  }

  // WAVE propagated or
  // not bearing momentum in wrapping?

  else if (code & (WF_OUT | CLASH_OUT))
  {
    // No info left in cell.

    empty(drf);
  }
}
