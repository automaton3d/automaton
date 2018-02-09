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

boolean trivial(Tile *t)
{
	char *p = (char *) t;
	for(int i = 0; i < sizeof(Tile); i++, p++)
		if(*p)
			return false;
	return true;
}

boolean interaction(Tile *t)
{
	if(t->p1 < BURST || (t->p12 != UXU && t->p12 != UXP && t->p12 != PXP))
		return false;
	//
	// Normalize clock
	//
	t->p1 %= SYNCH;
	//
	// Launch burst
	//
	t->p19 = true;
	resetTuple(&t->p25);
	//
	// Real preon?
	//
	if(t->p7)
	{
		// Axiom 13 - Launch graviton
		//
		dual->p17 |= GRAV;
		tupleCross(t->p6, t->p2, &t->p6);
		normalizeTuple(&t->p6);
		resetTuple(&t->p2);
	}
	t->p23 = t->p1 + SYNCH;
	t->p12 = UNDEF;
	//
	return true;
}

/*
 * Axiom 13
 */
void expandGraviton()
{
	if((pri->p17 & GRAV) == 0 || dual->p1 <= dual->p23)
		return;
	if(isNull(pri->p2))
		dual->p1 = dual->p1 % SYNCH + 1;
	//
	unsigned char dir = 6;
	Vector3d v1, v2;
	v1.x = pri->p6.x;
	v1.y = pri->p6.y;
	v1.z = pri->p6.z;
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
		nual->p23 = SYNCH * modTuple(&nual->p2) + 0.5;
		nual->p17 |= GRAV;
	}
	if(dual->p17 & SEED)
		dual->p17 = SEED;
	else
		cleanTile(dual);
}

/*
 * Axiom 7
 */
void expandBurst()
{
	if(!pri->p19)
		return;
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p25, pri->p26))
		{
			Tile *nual = getNual(dir);
			copyTile(nual, dual);
			nual->p17 &= PREON;
			nual->p26 = dir;					// save direction
			addTuples(&nual->p25, dirs[dir]);	// update burst origin vector
			//
			// Axiom 3 - Sinusoidal phase
			//
			incrDFO(nual);
			//
			// Axiom 4 - Interference
			//
			nual->p13 += nual->p21a1;
			//
			// Axiom 11 - nonlocal operation
			//
			if(pri->p8 && pri->p8 == nual->p8)
			{
				nual->p6 = pri->p6;
				invertTuple(&nual->p6);
			}
		}
	}
	if((pri->p17 & PREON) && isEqual(pri->p14, pri->p0))
	{
		printf("Reemit: p0=%s w=%d timer=%lu w=%d elapsed=%lu\n", tuple2str(&pri->p0), pri->p16, timer, pri->p16, GetTickCount() - begin);
		//
		dual->p1 %= SYNCH;
		dual->p17 |= SEED;
		dual->p23 = dual->p1 + SYNCH;
		resetTuple(&dual->p2);
		dual->p14.x = -1;
		dual->p12 = UNDEF;
		//
		// Axiom 3 - Sinusoidal phase
		//
		resetDFO(dual);
	}
	else
	{
		cleanTile(dual);
	}
	dual->p19 = false;
}

/*
 * Axiom 1
 */
void expandPreon()
{
	if((pri->p17 & (PREON | SEED)) == 0 || pri->p1 <= pri->p23 || pri->p1 < BURST)
		return;
	//
	// Tree expansion
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		// Axiom 2 - Visit-once-tree
		//
		if(isAllowed(dir, pri->p2, pri->p22))
		{
			Tile *nual = getNual(dir);
			copyTile(nual, dual);
			nual->p23 = nual->p1 + SYNCH * modTuple(&nual->p2) + 0.5;
			nual->p22 = dir;
			addTuples(&nual->p2, dirs[dir]);
			nual->p17 = PREON;
			int distance = (int) modTuple(&nual->p2) / (2 * DIAMETER);
			//
			// Axiom 4 - Interference
			// (exponential decay)
			//
			if(pri->p13 > 0)
			{
				dual->p13 *= (SIDE - SIDE / (2 * distance));
				if(dual->p13 < 0)
					dual->p13 = 0;
			}
			else if(dual->p13 < 0)
			{
				dual->p13 *= (SIDE + SIDE / (2 * distance));
				if(dual->p13 > 0)
					dual->p13 = 0;
			}
		}
	}
	//
	// Check wrapping
	//
	if((int)modTuple(&pri->p2) == limit && pri->p22 == 4)
	{
		dual->p1 = dual->p1 % SYNCH + 1;
		resetTuple(&dual->p2);
		dual->p3 = 0;		// undo EMP
		dual->p23 = SYNCH;
	}
	else
	{
		cleanTile(dual);
	}
}

boolean isP(Tile *t1, Tile *t2)
{
	return
		t2->p17 == PREON		&&
		t1->p3 == t2->p3		&&
		t1->p4 == t2->p4		&&
		t1->p5 == t2->p5		&&
		isEqual(t1->p6, t2->p6)	&&
		t1->p7 == t2->p7		&&
		t1->p8 == t2->p8		&&
		t1->p11 == t2->p11		&&
		t1->p15 == t2->p15;
}

boolean isU(Tile *t1, Tile *t2)
{
	return t2->p17 == PREON && !isP(t1, t2) && !isEqual(t1->p2, t2->p2);
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 6
 */
void classify1(Tile *dual)
{
	Tile *t1 = dual, *t2;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p17 == UNDEF || t1->p19 || isNull(t1->p2))
		{
			t1->p12 = UNDEF;
			continue;
		}
		t1->p12 = U;




		nuxg = 0;
		t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t2->p17 == GRAV)
				nuxg++;
		if(nuxg)
		{
			nuxg >>= 1;
			t2 = t1;
			w2 = dual->p16;
			while(nuxg)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(isP(t1, t2))
					nuxg--;
			}
			dual->p12 = UXG;
			dual->p27 = w2;
			continue;
		}



		npair = 0;
		t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && isP(t1, t2))
				npair++;
		if(npair)
		{
			npair >>= 1;
			t2 = t1;
			w2 = dual->p16;
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
					npair--;
			}
			dual->p12 = P;
			dual->p27 = w2;
			continue;
		}



		//
		nuxu = 0;
		t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && isU(t1, t2))
				nuxu++;
		if(nuxu)
		{
			nuxu >>= 1;
			t2 = t1;
			w2 = dual->p16;
			while(nuxu)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(isU(t1, t2))
					nuxu--;
			}
			//
			// Axiom 8
			//
			t1->p12 = UXU;
			t1->p27 = w2;
			t1->p14 = dual->p0;
		}
	}
}

/*
 * UxP, PXP
 * Axiom 6
 */
void classify2(Tile *dual)
{
	Tile *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p17 == UNDEF || t1->p12 == UNDEF || t1->p12 == UXU)
			continue;
		//
		Tile *t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
		{
			if(w1 != w2)
			{
				if(t1->p12 == P && t2->p12 == U)
				{
					t1->p12 = UXP;
					t1->p27 = w2;
					t1->p14 = t1->p0;
					//
					// Axiom 9 - Inertia
					//
					if(t1->p15)
					{
						// Parallel transport
						//
						subRectify(&t1->p14, t1->p2);
						addRectify(&t1->p14, t2->p29);
					}
					//
					// Axiom 10 - photon formation
					//
					if(pwm(tupleDot(&t1->p29, &t2->p29) / (3 * SIDE) - SIDE))
					{
						t1->p15 = false;
					}
					//
					// Axiom 14 - UXG interaction
					//
					if(trivial(t1))
					{
						t1->p15 = true;
					}
					//
					// Axiom 15 - gravitational acceleration
					//
					if(virtual(t1) && t2->p12 == UXG)
					{
						t1->p15 = true;
						t2->p12 = UNDEF;	// cuidado!
						//
						// P is aligned with p6 of origin G
						//
						// TODO
					}
					//
					// Axiom 16 - EM interaction
					//
					if(pwm(t1->p13) && trivial(t1))
					{
						t1->p3 = t2->p3;
						t1->p6 = t2->p6;
					}
					//
					// Axiom 17 - Coulomb interaction
					//
					int light = (int) modTuple(&t1->p2);
					int cycle = SIDE / t1->p10;
					//
					// Axiom 5 - polarization
					//
					if(t1->p3 != 0 && t2->p3 != 0 && pwm(t1->p13) && (light % cycle < cycle / 8) && pwm(t1->p13))
					{
						t1->p29 = t1->p2;
						subTuples(&t2->p29, t2->p2);
						if(t1->p3 == t2->p3)
						{
							// Repulsion
							//
							invertTuple(&t1->p29);
						}
					}
					//
					// Axiom 18 - Magnetic interaction
					// Axiom 5 - polarization
					//
					if(!isNull(t1->p6) && !isNull(t1->p6) && pwm(t1->p13)  && (light % cycle < cycle / 4) && pwm(t1->p13))
					{
						Tuple radial = t1->p2;
						subTuples(&radial, t2->p2);
						tupleCross(t1->p6, radial, &t1->p29);
						normalizeTuple(&t1->p29);
					}
					//
					// Axiom 19 - Absorption interaction
					//
					if(t1->p3 != 0 && t2->p3 != 0 && !t1->p15 )
					{
						// Axiom 12 - spin rotation
						//
						if(!t1->p15)
							rotateSpin(t1);
					}
					//
					// Axiom 20 - weak interaction
					//
					if((t1->p4 & t2->p4) && pwm(t1->p13) && pwm(t2->p13))
					{
						puts("Que fazer????");
					}
					//
					// Axiom 21 - Strong interaction
					//
					if(t1->p5 & t2->p5)
					{
						t1->p29 = t1->p2;
						subTuples(&t2->p29, t2->p2);
					}
					//
					// Axiom 11 - nonlocal ops
					//
					t1->p8 = t2->p8 = t1->p16 * t2->p16;
					//
					break;
				}
				if(t1->p12 == U && t2->p12 == P)
				{
					t1->p12 = UXP;
					t1->p27 = w2;
					t1->p14 = t1->p0;
					break;
				}
				if(t1->p12 == P && t2->p12 == P)
				{
					t1->p12 = PXP;
					t1->p27 = w2;
					//
					// Axiom 22
					//
					if(pwm(tupleDot(&t1->p29, &t2->p29) / (3 * SIDE) + SIDE))
					{
						// Trivial preon
						//
						cleanTile(t1);
					}
					break;
				}
			}
		}
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
		if(pri4d->p19)
			color = BB;
		else if(pri4d->p17 & GRAV)
			color = GG;
		else if(pri4d->p17 & PREON)
			color = 7 + pri4d->p16;
	}
	return color;
}

void expand()
{
	Tile *p = pri, *d = dual;
	for(int w = 0; w < NPREONS; w++, pri++, dual++)
		if(!interaction(dual))
		{
			expandBurst();
			expandGraviton();
			expandPreon();
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
	init();
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
