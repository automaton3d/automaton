/*
 * model.c
 */

#include "simulation.h"

extern Cell *stb, *drf;
extern Cell *latt0, *latt1;//DEBUG

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

/*
 * Information spreading.
 */
void phase4()
{
  // If cell is empty, does nothing.

  int role = getrole(stb);//GET_ROLE(stb);
  if(role == EMPTY || role == GRID)
    return;

  int code = 0;
  if (ZERO(stb->po))
    code |= POZ;
  else
    code |= PONZ;

  if (ZERO(stb->s))
    code |= SZ;
  else
    code |= SNZ;

  if (ZERO(stb->p))
    code |= PZ;
  else
    code |= PNZ;

  if (stb->n * stb->n < stb->syn)
    code |= RAW;
  else
    code |= READY;

  // Spread information.
  // Roles possible: SEED, WAVE, TRAVELLER.

  for(int dir = 0; dir < NDIR; dir++)
  {
	if(stb->off == 0)
	  printf("dir=%d\n", dir);

    // Calculate the address of the next neighbor.

    Cell* nei = neighbor(drf, dir);

    // Do not touch wavefront.

    if (BUSY(nei))
    {
      code |= BUSY_OUT;
      continue;
    }

    // The first part of this block propagates info
    // asynchronously (TRAVELLER processing).

    if (role == TRVLLR)
    {
      // Destiny already visited?

      if (nei->occ > 0)
      {
        code |= VISIT_OUT;
      }
      else
      {
        // Transmit superluminal info

        nei->occ = stb->occ;
        nei->n   = stb->n;    // used to calculate skew
        nei->obj = stb->obj;  // used for collapse

        code |= TRAV_OUT;
        if (code & PONZ)
        {
          // The seed is transported superluminaly.

          nei->ch  = stb->ch;   // charges
          nei->a1   = stb->a1;  // affinity
          nei->a2   = stb->a2;  // affinity
          nei->syn = stb->syn;  // wf synch
          nei->u   = stb->u;    // sine
          nei->k   = stb->k;    // kind
          CP(nei->p, stb->p);   // used to find pole
          CP(nei->s, stb->s);   // spin
          CP(nei->o, stb->o);   // bubble origin
          CP(nei->po, stb->po); // pole
          CP(nei->pP, stb->pP); // decay
          CP(nei->m, stb->m);   // messenger

          code |= POLE_OUT;
        }
      }
      continue;
    }
    // The second part of this block deals with
    // spherical synchronization (SEED and WAVE processing).

    if (code & SZ)
    {
      code |= NWAVE_OUT;
      continue;
    }

    // Calculate new origin vector.
    // Roles possible: SEED or WAVE.

    int org[3];
    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Propagate forward only.

    if (abs(org[i]) < abs(stb->o[i]))
    {
      code |= BACK_OUT;
      continue;
    }
    int mag3 = org[0] * org[0] + org[1] * org[1] + org[2] * org[2];
    if (mag3 > SIDE2/4)
    {
      code |= BACK_OUT;	// provisional
      continue;
    }

    // Is wavefront synchronized?
    // (see Ref. [14])

    if (code & RAW)
    {
      code |= RAW_OUT;
      continue;
    }
    if (mag3 == SIDE2/4)
    {
      if (ISMILD(org))
      {
        if (code & PZ)
        {
          code |= WRAP_OUT;
          continue;
        }
        else
        {
          code |= UNI_OUT;
        }
      }
      else
      {
        code |= CLASH_OUT;
        continue;
      }
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

    nei->syn = LIGHT2 * MOD2(org);

    // Let neighbor wrappable by default?????

    RSET(nei->po);

    // From now on consider momentum itself.

    if (role != SEED)
    {
      code |= WF_OUT;
      continue;
    }

    // Select momentum destination (Sect. 3.1).

    int dot = org[0] * stb->s[0] + org[1] * stb->s[1] + org[2] * stb->s[2];
    int mag1 = org[0] * org[0] + org[1] * org[1] + org[2] * org[2];
    int mag2 = stb->s[0] * stb->s[0] + stb->s[1] * stb->s[1] + stb->s[2] * stb->s[2];
    if ((code & TX_OUT) == 0 && dot * dot == mag1 * mag2)
    {
      CP(nei->p, stb->p);    // propagate momentum
      nei->occ = SIDE_2 - 1; // cell is crest now
      code |= TX_OUT;
    }
  }

  //////////////// End of for loop ///////////////

  if (code & POLE_OUT)
  {
    puts("POLE_OUT");  // transported p
    empty(drf);
  }
  else if (code & TX_OUT)
  {
    puts("TX_OUT");

    // Per definition of GRID:

    RSET(drf->p);     // no seed
    RSET(drf->s);     // no wavefront
    SAT(drf->m);      // no messenger
    SAT(drf->po);     // no traveller (1)
    drf->obj = SIDE3; // no traveller (2)
    drf->k = NONE;
    RSET(drf->pP);
  }
  else if (code & WF_OUT)
  {
    puts("WF_OUT");   // WAVE
    empty(drf);
  }
  else if (code & NWAVE_OUT)// neither SEED nor WAVE
  {
    puts("NWAVE_OUT");
    empty(drf);
  }
  else if (code & TRAV_OUT)
  {
    puts("TRAV_OUT");  // hunting
    empty(drf);
  }
  else if (code & CLASH_OUT)
  {
    puts("CLASH_OUT");
    empty(drf);
  }
  else if (code & RAW_OUT)
  {
    //puts("RAW_OUT (does nothing!!)");
  }
  else if (code & VISIT_OUT)
  {
    puts("VISIT_OUT (not expected)");
    assert(0);
  }
  else if (code & UNI_OUT)
  {
    puts("UNI_OUT");
	empty(drf);
  }
  else if (code & WRAP_OUT)
  {
    puts("WRAP_OUT (not expected)");
    assert(0);
  }
  else if (code & BUSY_OUT)
  {
    puts("BUSY_OUT (not expected)");
    assert(0);
  }
  else if (code & BACK_OUT)
  {
    puts("BACK_OUT (not expected)");
    assert(0);
  }
  else
  {
    puts("** ELSE ** (catastrophic)");
    assert(0);
  }
}
