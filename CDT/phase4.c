/*
 * model.c
 */

#include "simulation.h"

extern Cell *stb, *drf;

/*
 * Information spreading.
 */
void spread()
{
  // If cell is empty, does nothing.

  int role = GET_ROLE(stb);
  if(role == EMPTY || role == GRID)
    return;

  // Test maturity.

  int code = 0;
  if (stb->n * stb->n < stb->syn)
    code |= RAW_IN;

  // Spread information.
  // Roles possible: SEED, WAVE, TRAVELLER.

  Cell *resort = NULL;
  for(int dir = 0; dir < NDIR; dir++)
  {
    // Calculate the address of the next neighbor.

    Cell* nei = neighbor(drf, dir);

    // The first part of this block propagates info
    // asynchronously (TRAVELLER processing).

    int axis = dir >> 1;
    if (role == TRVLLR)
    {
      // Do not touch wavefront.

      if (BUSY(nei))
        continue;

      // Destiny already visited?

      if (nei->occ > 0)
      {
        code |= VISIT_OUT;
      }
      else
      {
        // Transmit superluminal info

        nei->occ = SIDE_2 - 1; // TODO: check if this value cover all regions
        nei->n   = stb->n;     // used to calculate skew
        nei->obj = stb->obj;   // used for collapse
        code |= TRAV_OUT;
        if (!ZERO(stb->po))
        {
          // The seed is transported superluminaly.
          // TODO: Guarantee that nei->o will be reset at destiny!

          code |= POLE_OUT;     // encode this case

          nei->ch  = stb->ch;   // charges
          nei->a1  = stb->a1;   // affinity
          nei->a2  = stb->a2;   // affinity
          nei->syn = stb->syn;  // wf synch
          nei->u   = stb->u;    // sine
          nei->k   = stb->k;    // kind
          CP(nei->p, stb->p);   // used to find pole
          CP(nei->s, stb->s);   // spin
          CP(nei->pP, stb->pP); // decay
          CP(nei->m, stb->m);   // messenger

          // Shrink pole toward re-emission cell.

          CP(nei->po, stb->po); // pole
          if (nei->po[axis] > 0)
            nei->po[axis] -= (dir % 2 == 0) ? +1 : -1;
        }
      }
      continue;
    }

    // The second part of this block deals with
    // spherical synchronization (SEED and WAVE processing).

    // Is wavefront synchronized?
    // (see Ref. [14])

    if (code & RAW_IN)
      continue;

    // Calculate new origin vector.

    int org[3];
    CP(org, stb->o);
    org[axis] += (dir % 2 == 0) ? +1 : -1;

    // Spatially illegal move?

    if (abs(org[axis]) < abs(stb->o[axis]) || abs(org[axis]) > SIDE_2)
      continue;

    // Test if destiny is busy.

    if (BUSY(nei))
    {
      // Guarantee momentum transfer.

      if (ZERO(stb->p))
      {
        code |= WF_OUT;
        continue;
      }
    }

    // Ultimate cell?

    if (abs(org[0]) == SIDE_2 && abs(org[1]) == SIDE_2 && abs(org[2]) == SIDE_2)
    {
      if (role == SEED)
      {
        nei->ch  = stb->ch;       // charges
        nei->n   = 0;             // reset tick counter
        nei->a1  = stb->off + 1;  // untie 1
        nei->a2  = 0;             // untie 2
        nei->k   = FERMION;       // untie 3
        nei->u   = 0;             // reset sine
        nei->obj = SIDE3;         // no target
        nei->occ = SIDE_2 - 1;    // crest cell
        nei->syn = 0;             // immediate expansion 1
        SAT(nei->m);              // no messenger role
        RSET(nei->o);             // immediate expansion 2
        RSET(nei->po);            // immediate expansion 3
        RSET(nei->pP);            // no empodion role
        CP(nei->s, stb->s);       // transfer spin
        CP(nei->p, stb->p);       // transfer momentum
        code |= WRAP_OUT;
      }
      else
      {
        code |= CLASH_OUT;
      }
      continue;
    }

    // Transmit superluminal info

    nei->n   = stb->n;    // ticks
    nei->obj = stb->obj;  // target
    nei->ch  = stb->ch;   // charges
    nei->a1  = stb->a1;   // affinity LO
    nei->a2  = stb->a2;   // affinity HO
    nei->k   = stb->k;    // content
    nei->occ = stb->occ;  // occupation status

    // Propagate exclusive spherical info.

    nei->u = stb->u;      // sine
    CP(nei->m, stb->m);   // messenger status
    CP(nei->s, stb->s);   // spin
    CP(nei->o, org);      // bubble origin
    CP(nei->pP, stb->pP); // decay
    RSET(nei->p);         // default for momentum

    // Spherical synchronism.

    nei->syn = LIGHT2 * MAG(org);

    // Let the neighbor be ready to propagate.

    RSET(nei->po);

    // From now on consider momentum itself.

    if (role != SEED)
    {
      code |= WF_OUT;
      continue;
    }

    resort = nei;

    // Select momentum destination (Sect. 3.1).
    // Test alignment of o with s.

    if ((code & TX_OUT) == 0 &&
        DOT(org,stb->s)*DOT(org,stb->s) == MAG(org)*MAG(stb->s)) // MODULO???
    {
      CP(nei->p, stb->p);    // propagate momentum
      nei->occ = SIDE_2 - 1; // cell is crest now
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

  // Wrapping without momentum.

  else if (code & WRAP_OUT)
  {
    empty(drf);
  }

  // Effective momentum transfer?

  else if (code & TX_OUT)
  {
    // Per definition of GRID:

    RSET(drf->p);     // no seed
    RSET(drf->s);     // no wavefront
    RSET(drf->pP);    // no empodion
    SAT(drf->m);      // no messenger
    SAT(drf->po);     // no traveller (1)
    drf->obj = SIDE3; // no traveller (2)
    drf->k = NONE;
  }

  // Do not let momentum escape.

  else if (role == SEED && resort != NULL)
  {
    // Last resort.

    CP(resort->p, stb->p);
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
