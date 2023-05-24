/*
 * model.c
 */

#include "simulation.h"

extern Cell *stb, *drf;

/*
 * Wavefront expansion.
 */
void phase4()
{
  // If cell is empty, does nothing.

  if (GET_ROLE(stb) == EMPTY)
    return;

  // Spherical expansion.

  boolean tx = false;
  for(int dir = 0; dir < NDIR; dir++)
  {
    // The first part of this block propagates info
    // asynchronously.

    Cell* nei = neighbor(drf, dir);
//    if (GET_ROLE(stb) == TRAVELLER)
//    	puts("Traveller");
/*
    // Do not touch wavefront.

    if (nei->occ == 0 && !BUSY(nei) && getRole(stb) == TRAVELLER)
    {
      // Transmit superluminal info

      nei->occ = stb->occ;
      nei->n   = stb->n;    // used to calculate skew
      nei->obj = stb->obj;  // used for collapse
      CP(nei->p, stb->p);	// used to find pole
    }
*/
    // The second part of this block deals with
    // spherical synchronization.

    if (GET_ROLE(stb) != SEED && GET_ROLE(stb) != WAVE)
    	continue;

    int org[3];
    CP(org, stb->o);
    int i = dir >> 1;
    org[i] += (dir % 2 == 0) ? +1 : -1;

    // Propagate forward only.

    if (abs(org[i]) < abs(stb->o[i]))
      continue;

    // Wrapped?

    if(ISMILD(org))
      continue;

    // Is wavefront synchronized?
    // (see Ref. [14])

    if (stb->n * stb->n < stb->syn)
      continue;

	printf("\tWave %d\n", stb->n);

    // Check if destiny cell has not been occupied.

    if (!ZERO(nei->s))
      continue;

    // Transmit superluminal info

    tx = true;
    nei->n   = stb->n;    // ticks
    nei->obj = stb->obj;  // target
    nei->ch  = stb->ch;   // charges
    nei->a1  = stb->a1;   // affinity LO
    nei->a2  = stb->a2;   // affinity HO
    nei->k   = stb->k;    // content
    nei->occ = stb->occ;  // occupation status
    CP(nei->m, stb->m);   // messenger status

    // Propagate exclusive spherical info.

    nei->u = stb->u;      // sine
    CP(nei->s, stb->s);   // spin
    CP(nei->o, org);      // bubble origin
    CP(nei->pP, stb->pP); // decay

    // Spherical synchronism.

    nei->syn = LIGHT2 * MOD2(org);

    // Let neighbor wrappable by default.

    MILD(nei->po);

    // Select momentum destination (Sect. 3.1).
    // Uses calculated alignment with spin.

    if (DOT(org, stb->s) == 1)
    {
      CP(nei->p, stb->p);   // propagate momentum
      CP(nei->po, stb->po); // propagate pole
      nei->occ = SIDE_2 - 1;
    }
    else
    {
      RSET(nei->p);
    }
  }

  // Test momentum transfer.

  if (tx && !ZERO(stb->p))
  {
    // There was a SEED (Table 2) here:
    // change abandoned cell role to GRID, but
    // trail info remains (p, o etc.).

    SAT(drf->po);
  }
  else
  {
    // Change role to EMPTY.

    drf->a1 = 0;
  }

  // No WAVE, nor SEED anymore.

  RSET(drf->s);
}
