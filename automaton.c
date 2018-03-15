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

#include "brick.h"
#include "common.h"
#include "params.h"
#include "init.h"
#include "tuple.h"
#include "vector3d.h"
#include "utils.h"
#include "plot3d.h"
#include "rotation.h"
#include "text.h"
#include "main3d.h"
#include "tree.h"

// The lattices

Brick *pri0, *dual0;
Brick *pri, *dual;
int limit;
char rotateRight[] = { 0, 4, 1, 0, 2, 0, 0, 0 };
char qcd[] = { 0x3f, 0x01, 0x02, 0x04, 0x20, 0x10, 0x08, 0x3f };

/*
 * Gets the neighbor's dual.
 */
Brick *getNual(int dir)
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
 * Definition 5 - Checks if dir is valid for graviton expansion.
 * returns:
 *      0 if forbidden
 *      1 if overflow
 *      2 if allowed
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

int conjug(Brick *t)
{
	if((t->p6 & 0x07) == (t->p6 >> 3))
		return 0;
	if(t->p6 & 0x07)
		return -1;
	return +1;
}

/*
 * Tests if a pair is trivial.
 */
boolean trivial(Brick *t)
{
	char *p = (char *) t;
	for(int i = 0; i < sizeof(Brick); i++, p++)
		if(*p)
			return false;
	return true;
}

/*
 * Axiom 11 - Color exchange
 */
void exchangeColors(Brick *t1, Brick *t2)
{
	// Rule out leptonic cases
	//
	if((t1->p6 & t2->p6) || t1->p6 == LEPT || t1->p6 == ANTILEPT ||t2->p6 == LEPT || t2->p6 == ANTILEPT)
		return;
	//
	// Rule out trivial case
	//
	if(t1->p6 == 0 && t2->p6 == 0)
		return;
	//
	// Change 000000 to 100100
	//
	int c1 = t1->p6;
	int c2 = t2->p6;
	if(c1 == 0)
		c1 = 0x24;
	if(c2 == 0)
		c2 = 0x24;
	//
	int d[4];
	d[0] = rotateRight[c1 & 0x07];
	d[1] = rotateRight[c1 >> 3];
	d[2] = rotateRight[c2 & 0x07];
	d[3] = rotateRight[c2 >> 3];
	//
	int opt = t1->p1 & 15;
	if(opt & 0x01)
		d[0] = rotateRight[d[0]];
	if(opt & 0x02)
		d[1] = rotateRight[d[1]];
	if(opt & 0x04)
		d[2] = rotateRight[d[2]];
	if(opt & 0x08)
		d[3] = rotateRight[d[3]];
	//
	c1 = d[1] << 3 | d[0];
	c2 = d[3] << 3 | d[2];
	//
	if(c1 & c2)
		return;
	//
	int n1 = 0, n2 = 0;
	if(c1 & 0x07)
		n1--;
	if(c1 >> 3)
		n1++;
	if(c2 & 0x07)
		n2--;
	if(c2 >> 3)
		n2++;
	//
	int m1 = 0, m2 = 0;
	if(d[0])
		m1--;
	if(d[1])
		m1++;
	if(d[2])
		m2--;
	if(d[3])
		m2++;
	if(n1 != m1 || n2 != m2)
		return;
	//
	t1->p6 = c1;
	t2->p6 = c2;
}

/*
 * Executes final procedures after collision detection.
 */
boolean interaction(Brick *t)
{
	// Interaction occurs at the last tick of the time frame
	// to avoid the burst waiting
	//
	if(t->p13 < UXU || t->p13 == PXP || t->p1 % SYNCH < SYNCH-1)
		return false;
	//
	t->p23 = t->p1 - t->p1 % SYNCH + SYNCH;
	addMarker(t->p0);
	//
	// Calculate disambiguation value
	//
	t->px = SIDE2 * t->p0.x + SIDE * t->p0.y + t->p0.z;
	//
	// Axiom 8 - Launch burst
	//
	t->p24 = DESTROY;
	resetTuple(&t->p25);
	//
	// Axiom 8 - Spin rotation
	//
	rotateSpin(t);
	//
	// Axiom 8 - Entanglement
	//
	t->p9 = t->p18 * t->p18;
	//
	// Axiom 10 - Vacuum expansion
	//
	if(t->p21 && trivial(t))
	{
		if(t->p18 > t->p19)
			t->p6 = qcd[t->p1 & 0x03];
		else
			t->p6 = qcd[8 - (t->p1 & 0x03)];
	}
	//
	// Real preon?
	//
	if(t->p8)
	{
		// Axiom 10 - Launch graviton
		//
		t->p20 |= GRAV;
		//
		// Definition 5 - G direction
		//
		tupleCross(t->p7, t->p2, &t->p7);
		normalizeTuple(&t->p7);
	}
	resetTuple(&t->p2);
	t->p13 = UNDEF;
	//
	return true;
}

/*
 * Cancellation.
 */
void cancellation(Brick *t)
{
	// Launch reconfiguration burst
	//
	t->p24 = CANCEL;
	resetTuple(&t->p25);
}

/*
 * G expansion
 */
void expandGraviton()
{
	if(pri->p20 != GRAV || dual->p1 <= dual->p23)
		return;
	//
	if(isNull(pri->p2))
		dual->p1 %= SYNCH;
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
		Brick *nual = getNual(dir);
		copyBrick(nual, dual);
		addTuples(&nual->p2, dirs[dir]);
		nual->p23 = SYNCH * (modTuple(&nual->p2) + 0.5);
		nual->p20 |= GRAV;
	}
	if(dual->p20 & SEED)
		dual->p20 = SEED;
	else
		cleanTile(dual);
}

void expandGraviton2()
{
	if(pri->p20 != GRAV)
		return;
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p2, pri->p22))
		{
			Brick *nual = getNual(dir);
			Vector3d probe;
			probe.x = pri->p7.x;
			probe.y = pri->p7.y;
			probe.z = pri->p7.z;
			norm3d(&probe);
			scale3d(&probe, pri->p1-BURST+1);
			Tuple p2 = dual->p2;
			addTuples(&p2, dirs[dir]);
			printf("%d: %s == %s\n", dual->p1, vector2str(&probe), tuple2str(&p2));
			if(voronoi(probe, p2))
			{
				copyBrick(nual, dual);
				nual->p22 = dir;
				addTuples(&nual->p2, dirs[dir]);
				//
				if(dual->p20 & SEED)
					dual->p20 = SEED;
				else
					cleanTile(dual);
				return;
			}
		}
	}
}


/*
 * Burst expansion
 */
void expandBurst()
{
	if(!pri->p24)
		return;
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p25, pri->p26))
		{
			Brick *nual = getNual(dir);
			//
			// Axiom 2 - Burst conflict
			//
			if(nual->p24 && dual->px < nual->px)
				continue;
			//
			copyBrick(nual, dual);
			nual->p20 &= PREON;					// erase SEED | GRAV
			nual->p26 = dir;					// save direction
			addTuples(&nual->p25, dirs[dir]);	// update burst origin vector
			if(pri->p9 && pri->p9 == nual->p9)	// entangled?
			{
				// Axiom 8 - Nonlocal operation
				//
				nual->p7 = pri->p7;
				invertTuple(&nual->p7);
				//
				// Axiom 4 - Interference
				//
				nual->p14 += nual->p27a1;
			}
		}
	}
	if((pri->p20 & PREON) && isEqual(pri->p15, pri->p0))
	{
		printf("Reissue: p0=%s w=%d timer=%lu w=%d elapsed=%lu\n", tuple2str(&pri->p0), pri->p18, timer, pri->p18, GetTickCount() - begin);
		//
		// Reissue at the specified address
		//
		dual->p23 = SYNCH + BURST - dual->p1;
		dual->p20 |= SEED;						// turn on SEED
		resetTuple(&dual->p2);
		dual->p15.x = -1;						// invalidate return-path
		dual->p13 = UNDEF;
		//
		// Axiom 3 - Sinusoidal phase
		//
		resetDFO(dual);
	}
	else if(pri->p24 == DESTROY)
	{
		if(dual->p20 && (dual->p20 & SEED))
			printf("  %s destroyed\n", tuple2str(&dual->p0));
		cleanTile(dual);
	}
	else
	{
		resetTuple(&dual->p3);
	}
	dual->p24 = UNDEF;							// leave no track
}

/*
 * Axiom 1
 */
void expandPreon()
{
	if((pri->p20 & (PREON | SEED)) == 0 || pri->p1 <= pri->p23)
		return;
	//
	assert(timer%SYNCH==pri->p1%SYNCH);
	if(isNull(pri->p2))
		dual->p1 %= SYNCH;
	//
	// Tree expansion
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		// Axiom 2 - Visit-once-tree
		//
		if(isAllowed(dir, pri->p2, pri->p22))
		{
			Brick *nual = getNual(dir);
			copyBrick(nual, dual);
			nual->p23 = SYNCH * (modTuple(&nual->p2) + 0.5);
			nual->p22 = dir;
			addTuples(&nual->p2, dirs[dir]);	// update origin vector
			nual->p20 = PREON;					// turn off SEED bit
			int distance = (int) modTuple(&nual->p2) / (2 * DIAMETER);
			if(distance > 0)
			{
				// Axiom 4 - Interference
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
			//
			// Axiom 6 - Virtual decay of P
			//
			if(!nual->p16)
				nual->p17 >>= 1;
			if(nual->p17 == 0 && !nual->p8 && !nual->p16 && (nual->p5 || nual->p6) && tupleDot(&V0, &nual->p2))
			{
				// P <- P0
				//
				nual->p4 = 0;
				nual->p5 = 0;
				nual->p6 = 0;
				nual->p20 = PREON;
				nual->p13 = REISSUE;
			}
		}
	}
	//
	// Check wrapping
	//
	if((int)modTuple(&pri->p2) == limit && pri->p22 == 4)
	{
		// Reissue
		//
		resetTuple(&dual->p2);
		dual->p4 = 0;		// undo EMP
	}
	else
	{
		if(dual->p20 & GRAV)
			dual->p20 = GRAV;
		else
			cleanTile(dual);
	}
}

/*
 * Tests if t1 and t2 form a P.
 */
boolean isP(Brick *t1, Brick *t2)
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
		t2->p20 == PREON;
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 7 - Detection by mutual comparison
 */
void classify1(Brick *dual)
{
	if(pri->p1 % SYNCH < BURST || pri->p24)
		return;
	//
	Brick *t1 = dual, *t2;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p20 == UNDEF || isNull(t1->p2))
		{
			t1->p13 = UNDEF;
			continue;
		}
		t1->p13 = U;
		incrDFO(t1);
		//
		// Detect UXG
		// t1 -> U
		// t2 -> G
		//
		nuxg = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t2->p20 == GRAV && t1->p8)
				nuxg++;
		if(nuxg)
		{
			// Select unambiguous peer
			//
			nuxg >>= 1;
			t2 = t1;
			w2 = t1->p18;
			while(nuxg)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p20 == GRAV && t1->p8)
					nuxg--;
			}
			//
			// Axiom 12 - Graviton detection
			//
			t1->p13 = UXG;
			t1->p18 = w2;
			t1->p3  = t2->p3;	// U inherits G's LM direction
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
			w2 = t1->p18;
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
					//  Axiom 3 - Sinusoidal phase
					// (bumped multiple times if superposition)
					//
					incrDFO(t1);
				}
			}
			t1->p13 = P;
			t1->p18 = w2;
			continue;
		}
		//
		// Detect UXU
		//
		nuxu = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && (t2->p20 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2))
				nuxu++;
		if(nuxu)
		{
			// Select unambiguous peer
			//
			nuxu >>= 1;
			t2 = t1;
			w2 = t1->p18;
			while(nuxu)
			{
				w2++;
				t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if((t2->p20 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2))
				{
					nuxu--;
					//
					//  Axiom 3 - Sinusoidal phase
					//
					incrDFO(t1);
				}
			}
			//
			// Axiom 13 - UxU interaction
			//
			if(t1->p4 == -t2->p4 && t1->p5 == -t1->p5 && t1->p6 == ~t2->p6)
			{
				// Annihilation
				//
				t1->p13 = UNDEF;
				t1->p4 = 0;
				t1->p6 = 0;

			}
			else if(t1->p6 == t2->p6)
			{
				// Both Us are reissued
				//
				t1->p13 = UXU;
				t1->p18 = w2;
				t1->p15 = t1->p0;
			}
		}
	}
}

/*
 * UxP, PXP
 * Axiom 7 - Interaction detection
 */
void classify2(Brick *dual)
{
	if(pri->p1 % SYNCH < BURST || pri->p24)
		return;
	//
	Brick *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p13 == UNDEF || isNull(t1->p2) || t1->p13 == UXU || t1->p13 == UXG)
			continue;
		//
		Brick *t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
		{
			if(w1 != w2)
			{
				if(isNull(t2->p2))
					continue;
				if(t1->p13 == P && t2->p13 == U)
				{
					t1->p13 = UXP;
					t1->p18 = w2;
					t1->p15 = t1->p0;
					//
					int light = (int) modTuple(&t1->p2);
					int cycle = SIDE / t1->p11;
					//
					// Axiom 14 - Gravitational acceleration
					//
					if(t1->p8 && t2->p13 == UXG)
					{
						t1->p16 = true;
						t2->p13 = UNDEF;
						//
						// P is aligned with p7 of origin G
						//
						t1->p3 = t2->p7;
					}
					//
					// Axiom 14 - Inertia
					//
					else if(t1->p16)
					{
						// Parallel transport
						//
						subRectify(&t1->p15, t1->p2);
						addRectify(&t1->p15, t2->p3);
					}
					//
					// Axiom 14 - LM alignment
					//
					else if(pwm(tupleDot(&t1->p3, &t2->p3) / SIDE - SIDE) / 2)
					{
						t1->p16 = false;
					}
					//
					// Axiom 14 - EM interaction
					//
					else if(t1->p4 != 0 && t2->p4 != 0 && pwm(t1->p14) && trivial(t1))
					{
						t1->p4 = t2->p4;
						t1->p7 = t2->p7;
					}
					//
					// Axiom 14 - Coulomb interaction
					// Axiom 6 - Polarization
					//
					else if(t1->p4 != 0 && t2->p4 != 0 && pwm(t1->p14) && (light % cycle) < cycle / 8)
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						if(t1->p4 == t2->p4)
						{
							// Repulsion
							//
							invertTuple(&t1->p3);
						}
					}
					//
					// Axiom 14 - Magnetic interaction
					// Axiom 6 - Polarization
					//
					else if(!isNull(t1->p7) && !isNull(t2->p7) && pwm(t1->p14)  && (light % cycle) < cycle / 4 && pwm(tupleDot(&t1->p7, &t2->p7) / SIDE - SIDE) / 2)
					{
						Tuple radial = t1->p2;
						subTuples(&radial, t2->p2);
						tupleCross(t1->p7, radial, &t1->p3);
						normalizeTuple(&t1->p3);
						t1->p4 = 0;
					}
					//
					// Axiom 14 - Absorption interaction
					//
					else if(t1->p4 != 0 && t1->p4 == -t2->p4 && !t1->p16 )
					{
						// Reissue all superposed preons
						//
						Brick *t2_ = dual;
						int w1 = t1->p18;
						for(int w2 = 0; w2 < NPREONS; w2++, t2_++)
							if(w1 != w2 && isEqual(t1->p2, t2_->p2))
								t2_->p13 = REISSUE;
					}
					//
					// Axiom 14 - Strong interaction
					//
					else if(t1->p6 & t2->p6)
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						//
						// Axiom 11 - Color exchange
						//
						exchangeColors(t1, t2);
					}
					//
					// Axiom 14 - Weak force
					//
					else if(t1->p5 != 0 && t2->p5 != 0 &&
							t1->p5 == -conjug(t1) && t2->p5 == -conjug(t2) &&
							pwm(t1->p14) && pwm(t2->p14))
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						//
						// Axiom 14 - Weak harvesting
						//
						if(trivial(t1))
						{
							t2->p17 = SIDE;
							t1->p16 = true;
						}
					}
					break;
				}
				if(t1->p13 == U && !t1->p21 && t2->p13 == P)
				{
					t1->p13 = UXP;
					t1->p18 = w2;
					t1->p15 = t1->p0;
					break;
				}
				if(t1->p13 == P && t2->p13 == P)
				{
					t1->p13 = PXP;
					t1->p18 = w2;
					//
					// Axiom 15 - Leptonic synthesis
					//
					if(t1->p6 == 0x37 && trivial(t2))
					{
						if(t1->p1 % 2)
						{
							t1->p6 = 0x38;
							t2->p6 = 0x07;
						}
						else
						{
							t1->p6 = 0x07;
							t2->p6 = 0x38;
						}
					}
					//
					// Axiom 15 - Cancellation
					//
					else if(pwm(tupleDot(&t1->p3, &t2->p3) / SIDE - SIDE) / 2)
					{
						cleanTile(t1);
					}
					//
					// Gluon-gluon interaction
					//
					else if(t1->p6 && t2->p6)
					{
						exchangeColors(t1, t2);
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

char getVoxel(Brick *pri, Brick *dual)
{
	char color = gridcolor;
	Brick *pri4d = pri;
	Brick *dual4d = dual;
	for(int w = 0; w < NPREONS; w++, pri4d++, dual4d++)
	{
		copyBrick(dual4d, pri4d);
		//
		// Bump cell clock
		//
		dual4d->p1++;
		//
		// Calculate voxel color
		//
		if(pri4d->p24)
			color = BB;
		else if(pri4d->p20 & GRAV)
			color = GG;
		else if(pri4d->p20 & PREON)
			color = 7 + pri4d->p18;
	}
	return color;
}

void expand()
{
	Brick *p = pri, *d = dual;
	for(int w = 0; w < NPREONS; w++, pri++, dual++)
	{
		if(pri->p1 % SYNCH < BURST)
		{
			expandBurst();
		}
		else if(dual->p13 == PXP)
		{
			cancellation(dual);
		}
		else if(!interaction(dual))
		{
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
	Brick *xchg = dual0;
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
		if(stop)
		{
			usleep(200000);
		}
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
