/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include "automaton.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include "common.h"
#include "params.h"
#include "init.h"
#include "tile.h"
#include "tuple.h"
#include "vector3d.h"
#include "utils.h"
#include "plot3d.h"
#include "rotation.h"
#include "text.h"
#include "main3d.h"
#include "tree.h"

// The lattices

Tile *pri0, *dual0;
Tile *pri, *dual;
int limit;

/*
 * Gets the neighbor's dual.
 */
Tile *getNual(int dir)
{
        switch(dir)
        {
                case 0:
                        if(dual->p0.x == SIDE-1)
                                return dual - WRAP0;
                        else
                                return dual + WRAP1;
                case 1:
                        if(dual->p0.x == 0)
                                return dual + WRAP0;
                        else
                                return dual - WRAP1;
                case 2:
                        if(dual->p0.y == SIDE-1)
                                return dual - WRAP2;
                        else
                                return dual + WRAP3;
                case 3:
                        if(dual->p0.y == 0)
                                return dual + WRAP2;
                        else
                                return dual - WRAP3;
                case 4:
                        if(dual->p0.z == SIDE-1)
                                return dual - WRAP4;
                        else
                                return dual + WRAP5;
                case 5:
                        if(dual->p0.z == 0)
                                return dual + WRAP4;
                        else
                                return dual - WRAP5;
        }
        return NULL;
}

/*
 * Definition 5 - Graviton.
 */
int checkPath(int dir)
{
	double d1 = mod2Tuple(&pri->p2);
	int x = pri->p2.x + dirs[dir].x;
	int y = pri->p2.y + dirs[dir].y;
	int z = pri->p2.z + dirs[dir].z;
	double d2 = x*x + y*y + z*z;
	if(d2 <= d1)
		return 0;
	//
	// Wrapping test
	//
	if(x == SIDE/2+1 || x == -SIDE/2 || y == SIDE/2+1 || y == -SIDE/2 || z == SIDE/2+1 || z == -SIDE/2)
		return 1;
	return 2;
}

/*
 * Tests if a pair is trivial.
 */
boolean trivial(Tile *t)
{
	char *p = (char *) t;
	for(int i = 0; i < sizeof(Tile); i++, p++)
		if(*p)
			return false;
	return true;
}

/*
 * Executes final procedures after collision detection.
 */
boolean interaction(Tile *t)
{
	if(t->p1 < BURST || t->p13 == UNDEF || t->p13 == PXP)
		return false;
	//
	// Normalize clock
	//
	t->p1 %= SYNCH;
	//
	// Launch burst
	//
	t->p22 = DESTROY;
	resetTuple(&t->p23);
	//
	// Real preon?
	//
	if(t->p8)
	{
		// Axiom 14 - Launch graviton
		//
		dual->p18 |= GRAV;
		tupleCross(t->p7, t->p2, &t->p7);
		normalizeTuple(&t->p7);
	}
	resetTuple(&t->p2);
	t->p21 = t->p1 + SYNCH;
	t->p13 = UNDEF;
	//

	puts("interaction");
	return true;
}

/*
 * Axiom 22 - Cancellation.
 */
void cancellation(Tile *t)
{
	if(t->p1 < BURST)
		return;
	//
	// Launch reconfiguration burst
	//
	t->p22 = CANCEL;
	resetTuple(&t->p23);
}

/*
 * Axiom 13 - G expansion
 */
void expandGraviton()
{
	if(pri->p18 != GRAV || dual->p1 <= dual->p21 || pri->p1 < BURST)
		return;
	if(isNull(pri->p2))
		dual->p1 = dual->p1 % SYNCH + 1;
	//
	unsigned char dir = 6;
	Vector3d v1, v2;
	v1.x = pri->p7.x;
	v1.y = pri->p7.y;
	v1.z = pri->p7.z;
	norm3d(&v1);
	double best = 0;
	for(int i = 0; i < NDIR; i++)
	{
		int path = checkPath(i);
		if(path == 2)
		{
			v2.x = pri->p2.x + dirs[i].x;
			v2.y = pri->p2.y + dirs[i].y;
			v2.z = pri->p2.z + dirs[i].z;
			norm3d(&v2);
			double c = dot3d(v1, v2);
			if(c >= best)
			{
				best = c;
				dir = i;
			}
		}
		else if(path == 1)
		{
			dir = 6;
			break;
		}
	}
	if(dir < 6)
	{
		Tile *nual = getNual(dir);
		copyTile(nual, dual);
		addTuples(&nual->p2, dirs[dir]);
		nual->p21 = SYNCH * modTuple(&nual->p2) + 0.5;
		nual->p18 |= GRAV;
	}
	if(dual->p18 & SEED)
		dual->p18 = SEED;
	else
		cleanTile(dual);
}

/*
 * Axiom 8 - Burst expansion
 */
void expandBurst()
{
	if(!pri->p22)
		return;
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p23, pri->p24))
		{
			Tile *nual = getNual(dir);
			//
			// Axiom 4 - Burst conflict
			//
			if(nual->p22 && tupleDot(&V0, &dual->p2) < tupleDot(&V0, &nual->p2))
				continue;
			//
			copyTile(nual, dual);
			nual->p18 &= PREON;					// erase SEED | GRAV
			nual->p24 = dir;					// save direction
			addTuples(&nual->p23, dirs[dir]);	// update burst origin vector
			if(pri->p9 && pri->p9 == nual->p9)	// entangled?
			{
				// Axiom 11 - nonlocal operation
				//
				nual->p7 = pri->p7;
				invertTuple(&nual->p7);
				//
				// Axiom 5 - Interference
				//
				nual->p14 += nual->p25a1;
			}
		}
	}
	if((pri->p18 & PREON) && isEqual(pri->p15, pri->p0))
	{
		printf("Reemit: p0=%s w=%d timer=%lu w=%d elapsed=%lu\n", tuple2str(&pri->p0), pri->p17, timer, pri->p17, GetTickCount() - begin);
		//
		// Reissue at the specified address
		//
		dual->p1 %= SYNCH;						// normalize clock
		dual->p18 |= SEED;						// turn on SEED
		dual->p21 = dual->p1 + SYNCH;
		resetTuple(&dual->p2);
		dual->p15.x = -1;						// invalidate return-path
		dual->p13 = UNDEF;
		//
		// Axiom 3 - Sinusoidal phase
		//
		resetDFO(dual);
	}
	else if(pri->p22 == DESTROY)
	{
		cleanTile(dual);
	}
	else
	{
		resetTuple(&dual->p3);
	}
	dual->p22 = UNDEF;							// leave no track
}

/*
 * Axiom 1
 */
void expandPreon()
{
	if((pri->p18 & (PREON | SEED)) == 0 || pri->p1 <= pri->p21 || pri->p1 < BURST)
		return;
	//
	// Tree expansion
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		// Axiom 2 - Visit-once-tree
		//
		if(isAllowed(dir, pri->p2, pri->p20))
		{
			Tile *nual = getNual(dir);
			copyTile(nual, dual);
			nual->p21 = nual->p1 + SYNCH * modTuple(&nual->p2) + 0.5;
			nual->p20 = dir;
			addTuples(&nual->p2, dirs[dir]);	// update origin vector
			nual->p18 = PREON;					// turn off SEED bit
			int distance = (int) modTuple(&nual->p2) / (2 * DIAMETER);
			if(distance > 0)
			{
				// Axiom 5 - Interference
				// (exponential decay)
				//
				if(pri->p14 > 0)
				{
					dual->p14 *= (SIDE - SIDE / (2 * distance));
					if(dual->p14 < 0)
						dual->p14 = 0;
				}
				else if(dual->p14 < 0)
				{
					dual->p14 *= (SIDE + SIDE / (2 * distance));
					if(dual->p14 > 0)
						dual->p14 = 0;
				}
			}
		}
	}
	//
	// Check wrapping
	//
	if((int)modTuple(&pri->p2) == limit && pri->p20 == 4)
	{
		// Reissue
		//
		dual->p1 = dual->p1 % SYNCH + 1;
		resetTuple(&dual->p2);
		dual->p4 = 0;		// undo EMP
		dual->p21 = SYNCH;
	}
	else
	{
		if(dual->p18 & GRAV)
			dual->p18 = GRAV;
		else
			cleanTile(dual);
	}
}

/*
 * Tests if t1 and t2 form a P.
 */
boolean isP(Tile *t1, Tile *t2)
{
	return
		!isNull(t2->p2)			&&
		t1->p4 == t2->p4		&&
		t1->p5 == t2->p5		&&
		t1->p6 == t2->p6		&&
		isEqual(t1->p7, t2->p7)	&&
		t1->p8 == t2->p8		&&
		t1->p9 == t2->p9		&&
		t1->p12 == t2->p12		&&
		t1->p16 == t2->p16		&&
		t2->p18 == PREON;
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 7 - Detection by mutual comparison
 */
void classify1(Tile *dual)
{
	if(pri->p1 < BURST)
		return;
	//
	Tile *t1 = dual, *t2;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p18 == UNDEF || isNull(t1->p2))
		{
			t1->p13 = UNDEF;
			continue;
		}
		t1->p13 = U;
		//
		// Detect UXG
		//
		nuxg = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t2->p18 == GRAV)
				nuxg++;
		if(nuxg)
		{
			// Select unambiguous peer
			//
			nuxg >>= 1;
			t2 = t1;
			w2 = t1->p17;
			while(nuxg)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p18 == GRAV)
					nuxg--;
			}
			t1->p13 = UXG;
			t1->p26 = w2;
			//
			// U inherits G's LM direction (temporary?)
			//
			t1->p3 =  t2->p3;
			continue;
		}
		//
		// Detect P
		//
		npair = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && isP(t1, t2))
				npair++;
		if(npair)
		{
			// Select unambiguous peer
			//
			npair >>= 1;
			t2 = t1;
			w2 = t1->p17;
			while(npair)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(isP(t1, t2))
				{
					npair--;
					//
					// Axiom 3 - Sinusoidal phase
					// (bumped multiple times if superposition)
					//
					incrDFO(t1);
				}
			}
			t1->p13 = P;
			t1->p26 = w2;
			continue;
		}
		//
		// Detect UXU
		//
		nuxu = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && (t2->p18 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2))
				nuxu++;
		if(nuxu)
		{
			// Select unambiguous peer
			//
			nuxu >>= 1;
			t2 = t1;
			w2 = t1->p17;
			while(nuxu)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p13 == U && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2))
				{
					nuxu--;
					//
					// Axiom 3 - Sinusoidal phase
					//
					incrDFO(t1);
				}
			}
			//
			// Both Us are reissued
			//
			t1->p13 = UXU;
			t1->p26 = w2;
			t1->p15 = t1->p0;
		}
	}
}

/*
 * UxP, PXP
 * Axiom 7 - Interaction detection
 */
void classify2(Tile *dual)
{
	if(pri->p1 < BURST)
		return;
	//
	Tile *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p13 == UNDEF || isNull(t1->p2) || t1->p13 == UXU)
			continue;
		//
		Tile *t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
		{
			if(w1 != w2)
			{
				if(isNull(t2->p2))
					continue;
				if(t1->p13 == P && t2->p13 == U)
				{
					t1->p13 = UXP;
					t1->p26 = w2;
					t1->p15 = t1->p0;
					//
					// Axiom 9 - Inertia
					//
					if(t1->p16)
					{
						// Parallel transport
						//
						subRectify(&t1->p15, t1->p2);
						addRectify(&t1->p15, t2->p3);
						break;
					}
					//
					// Axiom 10 - LM alignment
					//
					if(pwm(tupleDot(&t1->p3, &t2->p3) / SIDE - SIDE) / 2)
					{
						t1->p16 = false;
					}
					//
					// Axiom 14 - UXG interaction
					// t1 -> U
					// t2 -> G
					//
					if(t1->p8)
					{
						t1->p13 = UXG;
						t1->p3 = t2->p3;
						break;
					}
					//
					// TODO ???
					//
					if(trivial(t1))
					{
						t1->p16 = true;
						break;
					}
					//
					// Axiom 15 - Gravitational acceleration
					// t1 -> P
					// t2 -> U
					//
					if(t1->p8 && t2->p13 == UXG)
					{
						t1->p16 = true;
						t2->p13 = UNDEF;
						//
						// P is aligned with p7 of origin G
						//
						t1->p3 = t2->p7;
						break;
					}
					//
					// Axiom 16 - EM interaction
					//
					if(pwm(t1->p14) && trivial(t1))
					{
						t1->p4 = t2->p4;
						t1->p7 = t2->p7;
						break;
					}
					//
					// Axiom 17 - Coulomb interaction
					// Axiom 6 - polarization
					//
					int light = (int) modTuple(&t1->p2);
					int cycle = SIDE / t1->p11;
					if(t1->p4 != 0 && t2->p4 != 0 && pwm(t1->p14) && (light % cycle < cycle / 8))
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						if(t1->p4 == t2->p4)
						{
							// Repulsion
							//
							invertTuple(&t1->p3);
						}
						break;
					}
					//
					// Axiom 18 - Magnetic interaction
					// Axiom 6 - polarization
					//
					if(!isNull(t1->p7) && !isNull(t2->p7) && pwm(t1->p14)  && (light % cycle < cycle / 4))
					{
						Tuple radial = t1->p2;
						subTuples(&radial, t2->p2);
						tupleCross(t1->p7, radial, &t1->p3);
						normalizeTuple(&t1->p3);
						break;
					}
					//
					// Axiom 19 - Absorption interaction
					//
					if(t1->p4 != 0 && t2->p4 != 0 && !t1->p16 )
					{
						// Axiom 12 - spin rotation
						//
						if(!t1->p16)
							rotateSpin(t1);
					}
					//
					// Axiom 20 - Strong interaction
					//
					if(t1->p6 & t2->p6)
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						break;
					}
					//
					// Axiom 21 - weak interaction
					//
					if((t1->p5 & t2->p5) && pwm(t1->p14) && pwm(t2->p14))
					{
						// Neutral P?
						//
						if(isNull(t1->p7))
						{
							// P has LM direction redefined
							//
							t1->p22 = t2->p22;
						}
						break;
					}
					//
					// Axiom 11 - nonlocal ops
					//
					t1->p9 = t2->p9 = t1->p17 * t2->p17;
					//
					break;
				}
				if(t1->p13 == U && !t1->p19 && t2->p13 == P)
				{
					t1->p13 = UXP;
					t1->p26 = w2;
					t1->p15 = t1->p0;
					break;
				}
				if(t1->p13 == P && t2->p13 == P)
				{
					// Axiom 22 - Opposite Ps
					//
					if(pwm(tupleDot(&t1->p3, &t2->p3) / SIDE - SIDE) / 2)
					{
						t1->p13 = PXP;
						t1->p26 = w2;
					}
					break;
				}
			}
		}
		//
		// Disregard intermediate values
		//
		if(t1->p13 != UXP && t1->p13 != PXP)
			t1->p13 = UNDEF;
	}
}

char getVoxel(Tile *pri, Tile *dual)
{
	char color = gridcolor;
	Tile *pri4d = pri;
	Tile *dual4d = dual;
	for(int w = 0; w < NPREONS; w++, pri4d++, dual4d++)
	{
		copyTile(dual4d, pri4d);
		//
		// Bump cell clock
		//
		dual4d->p1++;
		//
		// Calculate voxel color
		//
		if(pri4d->p22)
			color = BB;
		else if(pri4d->p18 & GRAV)
			color = GG;
		else if(pri4d->p18 & PREON)
			color = 7 + pri4d->p17;
	}
	return color;
}

void expand()
{
	Tile *p = pri, *d = dual;
	for(int w = 0; w < NPREONS; w++, pri++, dual++)
	{
		if(dual->p13 == PXP)
		{
			cancellation(dual);
		}
		else if(!interaction(dual))
		{
			expandBurst();
			expandGraviton();
			expandPreon();
		}
	}
	pri = p; dual = d;
}

/*
 * Executes one cycle of the automaton program.
 * Optimization (old - new) / new x 100% = 34%
 */
void cycle()
{
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		draft[p3d] = getVoxel(pri, dual);
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			classify1(dual);
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			classify2(dual);
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			expand();
	//
	// Flip lattices
	//
	Tile *xchg = dual0;
	dual0 = pri0;
	pri0 = xchg;
	//
	timer++;
}

/*
 * The automaton thread.
 */
void *AutomatonLoop()
{
    pthread_detach(pthread_self());
	initAutomaton();
	while(true)
	{
		if(stop && timer % 2 == 0)
			usleep(200000);
		else
		{
			cycle();
			//
			// Swap image buffer
			//
        	pthread_mutex_lock(&mutex);
			char *flip = draft;
			draft = clean;
			clean = flip;
    	    pthread_mutex_unlock(&mutex);
		}
	}
	return NULL;
}


