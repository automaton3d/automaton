/*
 * model.c
 */

#include "simulation.h"

extern Cell *stb, *drf;
extern Cell *latt0, *latt1;//DEBUG

/*
 * Information spreading.
 */
void phase4()
{
  // If cell is empty, does nothing.

  int role = GET_ROLE(stb);
  if(role == EMPTY || role == GRID)
    return;

//  puts("SEED, WAVE, TRAVELLER");
  // Spread information.
  // Roles possible: SEED, WAVE, TRAVELLER.

  Cell *resort = NULL;
  boolean tx = false;
  for(int dir = 0; dir < NDIR; dir++)
  {
    // Calculate the address of the next neighbor.

    Cell* nei = neighbor(drf, dir);

//    printf("\n\tdir: %d\n", dir);

    // The first part of this block propagates info
    // asynchronously (TRAVELLER processing).

/*
    // Do not touch wavefront.

    if (nei->occ == 0 && !BUSY(nei) && getRole(stb) == TRAVELLER)
    {
      // Transmit superluminal info

      nei->occ = stb->occ;
      nei->n   = stb->n;    // used to calculate skew
      nei->obj = stb->obj;  // used for collapse
      if (!ZERO(stb->po))
        CP(nei->p, stb->p); // used to find pole
    }
*/
    // The second part of this block deals with
    // spherical synchronization (SEED and WAVE processing).

    if (ZERO(stb->s))
    	continue;

//    puts("\ts != 0");

    // Calculate new origin vector.
    // Roles possible: SEED or WAVE.

    int org[3];
    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Propagate forward only.

    if (abs(org[i]) < abs(stb->o[i]))
      continue;

//    puts("\tforward");

    // Wrapped?

    if (ISMILD(org))
    {
    	if (ZERO(stb->p))
    	      continue;
   		puts("\tFODALHAÇO");
    }

    // Is wavefront synchronized?
    // (see Ref. [14])

    if (stb->n * stb->n < stb->syn)
      continue;

//    puts("\tmaduro");

    // Check if destiny cell has not been occupied.

    if (ZERO(stb->p))
    {
        if (!ZERO(nei->s))
          continue;
    }
    else
    {
        if (GET_ROLE(nei) == SEED)
        {
          tx = true;
          continue;
        }
    }

//    puts("\tdestino vazio");

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

    // Let neighbor wrappable by default.

    MILD(nei->po);

    // From now on consider momentum itself.

    if (role != SEED)
    {
      continue;
    }

 //   puts("\tseed");

	resort = nei;

    // Select momentum destination (Sect. 3.1).

    int dot = org[0] * stb->s[0] + org[1] * stb->s[1] + org[2] * stb->s[2];
    int mag1 = org[0] * org[0] + org[1] * org[1] + org[2] * org[2];
    int mag2 = stb->s[0] * stb->s[0] + stb->s[1] * stb->s[1] + stb->s[2] * stb->s[2];
    if (!tx && dot * dot == mag1 * mag2)
    {
      CP(nei->p, stb->p);    // propagate momentum
      CP(nei->po, stb->po);  // propagate pole
      nei->occ = SIDE_2 - 1; // cell is crest now
      tx = true;
 //     puts("\ttx");
    }
  }

  // Separate roles.

  if (!ZERO(stb->s))
  {
//	    puts("wave");

    // Mature?

    if (stb->n * stb->n >= stb->syn)
    {
      // WAVE and SEED processing.
      // Test momentum transfer.
 //       puts("maduro");

      // Clean common unused variables.

      RSET(drf->p);     // no seed
      RSET(drf->s);     // no wavefront
      SAT(drf->m);      // no messenger
      SAT(drf->po);     // no traveller (1)
      drf->obj = SIDE3; // no traveller (2)

      if (tx)
      {
  //  	    puts("tx");

        // SEED processing:
        // change abandoned cell role to become a
        // GRID, but trail info remains (o,  a1 etc.).
        // Per definition of GRID:

        SAT(drf->pP);
        drf->obj = SIDE3;
        drf->k = NONE;
      }

      // WAVE processing?

      else
      {
  //  	    puts("not tx");

        // Momentum has not bee transmmited,
        // use last resort.

        if (!ZERO(stb->p))
        {
//            puts("p != 0");

          // Don't let momentum escape.

          printCell(stb);
//          printConfig(stb);
  //        printf("OFFSET %ld, %ld\n", (long)(stb-latt0), (long)(drf-latt1));
          //assert(resort != NULL);
          CP(resort->p, stb->p);
        }
      }
    }
  }

  // Role is TRAVELLER.

  else
  {
//	    puts("traveller");
    // Cell must be completely emptied.

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
}


/////////////// REBOT


/*
void phase4()
{
	if(ZERO(stb->s))
		return;
    Cell* nei = neighbor(drf, rand() % 6);
    if (ZERO(nei->s))
    {
        nei->s[0] = 1;
        nei->p[0] = 1;
    }
    RSET(drf->s);
}

void phase4z()
{
  int role = GET_ROLE(stb);				// OK
  if(role == EMPTY || role == GRID)		// OK
	  return;							// OK
  //printCell(stb);
  boolean test = false;
  for(int dir = 0; dir < NDIR; dir++)	// OK
  {
    Cell* nei = neighbor(drf, dir);		// OK
    int org[3];							// OK
    CP(org, stb->o);					// OK
    int i = dir >> 1;					// OK
    org[i] += (dir % 2 == 0) ? +1 : -1;	// OK
    if (abs(org[i]) < abs(stb->o[i]))	// OK
      continue;							// OK
    if(ISMILD(org))
      continue;
    if (stb->n * stb->n < stb->syn)		// OK
      continue;							// OK
    if (!ZERO(nei->s))					// OK
    	continue;						// OK
    nei->n    = stb->n;					// OK
    nei->p[0] = 1;						// OK
    nei->s[0] = 1;						// OK
    CP(nei->o, org);					// OK
    nei->syn = LIGHT2 * MOD2(org);		// OK
    test = true;
  }
  if(!test)
	  return;
  RSET(drf->p);
  RSET(drf->s);
  SAT(drf->m);
  SAT(drf->po);
//  empty(drf);	// DEBUG
  //assert(GET_ROLE(drf) == EMPTY);
}

*/
