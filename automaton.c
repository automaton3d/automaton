/*
 * automaton.c
 */

#include "automaton.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "brick.h"
#include "common.h"
#include "params.h"
#include "init.h"
#include "main.h"
#include "tuple.h"
#include "vector3d.h"
#include "utils.h"
#include "plot3d.h"
#include "rotation.h"
#include "text.h"
#include "tree.h"

// The lattices

Brick *pri0, *dual0;
Brick *pri, *dual;
int limit;
char rotateRight[] = { 0, 4, 1, 0, 2, 0, 0, 0 };
char qcd[] = { 0x3f, 0x01, 0x02, 0x04, 0x20, 0x10, 0x08, 0x3f };
boolean img_changed;

/*
 * Gets the neighbor to the cell.
 */
Brick *getNeighbor(int dir, Brick *b)
{
	switch(dir)
	{
		case 0:
			if(b->p0.x == SIDE-1)
				return b - WRAP0;
			else
				return b + WRAP1;
		case 1:
			if(b->p0.x == 0)
				return b + WRAP0;
			else
				return b - WRAP1;
		case 2:
			if(b->p0.y == SIDE-1)
				return b - WRAP2;
			else
				return b + WRAP3;
		case 3:
			if(b->p0.y == 0)
				return b + WRAP2;
			else
				return b - WRAP3;
		case 4:
			if(b->p0.z == SIDE-1)
				return b - WRAP4;
			else
				return b + WRAP5;
		case 5:
			if(b->p0.z == 0)
				return b + WRAP4;
			else
				return b - WRAP5;
	}
	return NULL;
}

/*
 * Is the vacuum symmetry broken?
 */
boolean broken(Brick *t1, Brick *t2)
{
	static unsigned MASK = SIDE-1;
	unsigned index1, index2;
	index1 = SIDE2 * t1->p0.x + SIDE * t1->p0.y + t1->p0.z;
	index2 = SIDE2 * t2->p0.x + SIDE * t2->p0.y + t2->p0.z;
	unsigned h1 = hash(t1->p19) ^ hash(t1->p1) ^ index1;
	unsigned h2 = hash(t2->p19) ^ hash(t2->p1) ^ index2;
	return (h1 & MASK) == (h2 & MASK);
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

/*
 * Charge conjugation.
 *
 * Returns:
 * +1 matter
 * -1 antimatter
 * 0 neutral
 */
int conjug(Brick *t)
{
	if((t->p9 & 0x07) == (t->p9 >> 3))
		return 0;
	if(t->p9 & 0x07)
		return -1;
	return +1;
}

/*
 * Tests if a preon belongs to the vacuum
 */
boolean vacuum(Brick *t)
{
	return t->p22 && isNull(t->p3) && isNull(t->p4) && t->p8 == -1 && !t->p10 && !t->p16 && (t->p21 & PREON);
}

/*
 * Axiom 11 - Color exchange
 */
void exchangeColors(Brick *t1, Brick *t2)
{
	// Rule out leptonic cases
	//
	if((t1->p9 & t2->p9) || t1->p9 == LEPT || t1->p9 == ANTILEPT ||t2->p9 == LEPT || t2->p9 == ANTILEPT)
		return;
	//
	// Rule out trivial case
	//
	if(t1->p9 == 0 && t2->p9 == 0)
		return;
	//
	// Change 000000 to 100100
	//
	int c1 = t1->p9;
	int c2 = t2->p9;
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
	t1->p9 = c1;
	t2->p9 = c2;
}

/*
 * Executes final procedures after collision detection.
 */
boolean interaction(Brick *t)
{
	// Interaction occurs at the last tick of the time frame
	// to avoid the burst waiting
	//
	if(t->p13 < UXU || t->p1 % SYNCH > 0)
		return false;
	//
	// Axiom 8 - Launch burst
	//
	t->p15 = t->p0;
	t->p18 = signature(t);
	t->p25 = DESTROY;
	resetTuple(&t->p26);
	//
	// Reissue preon
	//
	t->p24 = t->p1 - t->p1 % SYNCH + SYNCH;	// Calculate new wake time
	//
	// Axiom 7 - Vacuum symmetry breaking
	//
	if(t->p13 == REISSUE)
	{
		resetTuple(&t->p2);
		t->p13 = UNDEF;
		return true;		// Inhibit expansion
	}
	//
	// Axiom 7 - Spin rotation
	//
	rotateSpin(t);
	//
	// Axiom 7 - Entanglement
	//
	Brick *t2 = dual0 + (SIDE2 * t->p0.x + SIDE * t->p0.y + t->p0.z) * NPREONS + t->p20;

	t->p10 = t->p19 * t2->p19;
	tupleCross(t->p4, t2->p4, &t->p4);
	if(t->p19 > t->p20)
		invertTuple(&t->p4);
	//
	// Axiom 10 - Vacuum expansion
	//
	if(t->p22 && vacuum(t))
	{
		if(t->p19 > t->p20)
			t->p9 = qcd[t->p1 & 0x03];
		else
			t->p9 = qcd[8 - (t->p1 & 0x03)];
	}
	//
	// Real preon?
	//
	if(t->p8)
	{
		t->p21 |= GRAV;		// Axiom 10 - Launch graviton
		t->p4 = t->p2;
	}
	t->p13 = UNDEF;
	resetTuple(&t->p2);
	//
	return true;
}

/*
 * G expansion.
 */
void expandGraviton()
{
	if(!(pri->p21 & GRAV) || dual->p1 <= dual->p24)
		return;
	//
	unsigned char dir = 6;
	Vector3d v1, v2;
	v1.x = pri->p4.x;
	v1.y = pri->p4.y;
	v1.z = pri->p4.z;
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
		Brick *nual = getNeighbor(dir, dual);
		copyBrick(nual, dual);
		addTuples(&nual->p2, dirs[dir]);
		nual->p24 = SYNCH * (modTuple(&nual->p2) + 0.5);
		nual->p21 |= GRAV;
	}
	//
	// Preserve SEED
	//
	if(dual->p21 & PREON)
		dual->p21 = PREON;
	else
		cleanBrick(dual);
}

/*
 * Look-ahead mechanism for burst conflict resolution.
 */
boolean conflict(int dir, unsigned p18)
{
	// Disambiguate p18
	//
	Brick *nb = getNeighbor(dir, pri);
	int best = nb->p18;
	for(int i = 0; i < NDIR; i++)
	{
		Brick *nnb = getNeighbor(i, nb);
		if(nnb->p25 && nnb->p27 == opposite(i) && nnb->p18 > best)
			best = nnb->p18;
	}
	if(p18 > best)
		return false;
	if(p18 < best)
		return true;
	//
	// Disambiguate signature
	//
	best = 0;
	for(int i = 0; i < NDIR; i++)
	{
		Brick *nnb = getNeighbor(i, nb);
		if(nnb->p25 && nnb->p27 == opposite(i) && signature(nnb) > best)
			best = signature(nnb);
	}
	return signature(pri) < best;
}

/*
 * Axiom 7 - Burst expansion
 */
void expandBurst()
{
	if(!pri->p25)
		return;
	double d = modTuple(&pri->p26);
	for(int dir = 0; dir < NDIR; dir++)
	{
		Tuple next = pri->p26;
		addTuples(&next, dirs[dir]);
		if(next.x == SIDE/2 || next.x == -SIDE/2-1 || next.y == SIDE/2 || next.y == -SIDE/2-1 || next.z == SIDE/2 || next.z == -SIDE/2-1)
			continue;
		if(modTuple(&next) > d)
		{
			Brick *nual = getNeighbor(dir, dual);
			//
			// Axiom 2 - Burst conflict resolution
			//
			if(conflict(dir, pri->p18))
				continue;
			//
			// Axiom 13 - Cancellation
			//
			if(dual->p25 == CANCEL)
			{
				nual->p1 = dual->p1;
				nual->p18 = dual->p18;
				nual->p25 = CANCEL;
			}
			else
			{
				copyBrick(nual, dual);
				nual->p21 &= PREON;						// erase GRAV bit
				if(pri->p10 && pri->p10 == nual->p10)	// entangled?
				{
					// Axiom 7 - Nonlocal operation
					//
					nual->p4 = pri->p4;
					invertTuple(&nual->p4);
					//
					// Axiom 4 - Interference
					//
					nual->p14 += nual->p28a1;
				}
			}
			nual->p26 = next;
			nual->p27 = dir;					// save new direction
		}
	}
	if((pri->p21 & PREON) && isEqual(pri->p15, pri->p0))
	{
		// Reissue at the specified address
		//
		dual->p24 = SYNCH + BURST - dual->p1 % SYNCH;
		dual->p21 |= PREON;						// turn on SEED bit
		resetTuple(&dual->p2);
		dual->p13 = UNDEF;
		//
		// Axiom 3 - Sinusoidal phase
		//
		resetDFO(dual);
	}
	else if(pri->p25 == DESTROY)
	{
		cleanBrick(dual);
	}
	else
	{
		// Axiom 13 - Cancellation
		// P <- P0
		//
		resetTuple(&dual->p3);
		resetTuple(&dual->p4);
		dual->p8 = -1;
		dual->p10 = 0;
		dual->p16 = 0;
		dual->p18 = 0;
	}
	dual->p25 = UNDEF;							// leave no burst track
}

/*
 * Axiom 1
 */
void expandPreon()
{
	if((pri->p21 & PREON) == 0 || pri->p1 <= pri->p24)
		return;
	//
	assert(timer%SYNCH==pri->p1%SYNCH);
	if(isNull(pri->p2))
		dual->p1 %= SYNCH;
	dual->p13 = UNDEF;
	//
	// Tree expansion
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		// Axiom 2 - Visit-once-tree
		//
		if(isAllowed(dir, pri->p2, pri->p23))
		{
			Brick *nual = getNeighbor(dir, dual);
			copyBrick(nual, dual);
			nual->p24 = SYNCH * (modTuple(&nual->p2) + 0.5);
			nual->p23 = dir;
			addTuples(&nual->p2, dirs[dir]);	// update origin vector
			nual->p21 = PREON;					// turn off SEED bit
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
			// Axiom 5 - Virtual decay of P
			//
			if(!nual->p16)
			{
				nual->p17 >>= 1;
				if(!nual->p8 && nual->p17 == 0 && !nual->p16 && (nual->p7 || nual->p9) && tupleDot(&V0, &nual->p2))
				{
					// P <- P0
					//
					resetTuple(&nual->p3);
					resetTuple(&nual->p4);
					nual->p8 = -1;
					nual->p10 = 0;
					nual->p16 = 0;
					nual->p21 = PREON;
					nual->p13 = REISSUE;
					puts("virtual decay");
				}
			}
		}
	}
	//
	// Check wrapping
	//
	if((int)modTuple(&pri->p2) == limit && pri->p23 == 3)
	{
		// Reissue
		//
		resetTuple(&dual->p2);
		//
		// Undo EMP
		//
		if(dual->p19 < dual->p20)
			dual->p6 = +1;
		else
			dual->p6 = -1;
		//
		resetTuple(&dual->p4);
	}
	else
	{
		if(dual->p21 & GRAV)
			dual->p21 = GRAV;
		else
			cleanBrick(dual);
	}
}

/*
 * Tests if t1 and t2 form a P.
 */
boolean isP(Brick *t1, Brick *t2)
{
	return
		!isNull(t2->p2)				&&
		isEqual(t1->p2, t2->p2) 	&&
		t1->p6 == -t2->p6			&&
		t1->p7 == -t2->p7			&&
		t1->p9 == (~t2->p9 & 0x3f)	&&
		t1->p5 == t2->p5			&&
		t1->p16 == t2->p16			&&
		(t2->p21 & PREON);
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 7 - Detection by mutual comparison
 */
void classify1(Brick *dual)
{
	if(pri->p1 % SYNCH < SYNCH-1 || pri->p25)
		return;
	//
	Brick *t1 = dual, *t2;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p21 == UNDEF || isNull(t1->p2))
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
			if(w1 != w2 && t2->p21 == GRAV && t1->p8 && !isEqual(t1->p2, t2->p2))
				nuxg++;
		if(nuxg)
		{
			// Select unambiguous peer
			//
			nuxg >>= 1;
			t2 = t1;
			w2 = t1->p19;
			while(nuxg)
			{
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p21 == GRAV && t1->p8 && !isEqual(t1->p2, t2->p2))
					nuxg--;
			}
			//
			// Axiom 12 - Graviton detection
			//
			t1->p13 = UXG;
			t1->p20 = w2;
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
			w2 = t1->p19;
			while(npair)
			{
				w2++; t2++;
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
			//
			// Vacuum-vacuum interaction
			//
			if(vacuum(t1) && vacuum(t2) && isEqual(t1->p2, t2->p2) && broken(t1, t2))
			{
				t1->p13 = REISSUE;
				puts("vac-vac");
			}
			else
			{
				puts("formou um P");
				t1->p13 = P;
			}
			assert(t1->p20!=w2);
			t1->p20 = w2;
			t1->p22 = true;
			continue;
		}
		//
		// Detect UXU
		//
		nuxu = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t1->p8 == t2->p8 && (t2->p21 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2) && t1->p13 != REISSUE && t2->p13 != REISSUE)
				nuxu++;
		Brick *winner;
		if(nuxu)
		{
			// Select unambiguous peer
			//
			nuxu >>= 1;
			if(nuxu == 0)
				nuxu = 1;
			t2 = t1;
			w2 = t1->p19;
			int w;
			while(nuxu)
			{
				// Wrapping
				//
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				//
				if(t1->p8 == t2->p8 && (t2->p21 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2) && t1->p13 != REISSUE && t2->p13 != REISSUE)
				{
					nuxu--;
					//
					//  Axiom 3 - Sinusoidal phase
					//
					winner = t2;
					w = w2;
				}
			}
			t2 = winner;
			//
			// Axiom 13 - UxU interaction
			//
			if(t1->p6 == -t2->p6 && t1->p7 == -t1->p7 && t1->p9 == ~t2->p9)
			{
				// Annihilation
				//
				t1->p13 = REISSUE;
				resetTuple(&t1->p3);
				resetTuple(&t1->p4);
				t1->p10 = 0;
				t1->p16 = 0;
			}
			else if(t1->p9 == t2->p9)
			{
				//printf("%d: p0=%s,%d\n", t1->p1, tuple2str(&t1->p0), t1->p19);
				// Both Us are reissued
				//
				t1->p13 = UXU;
				t1->p15 = t1->p0;
				incrDFO(t1);
				t1->p20 = w;
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
	if(pri->p1 % SYNCH < SYNCH-1 || pri->p25 || dual->p13 == REISSUE)
		return;
	//
	Brick *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p13 == UNDEF || t1->p13 == UXU || t1->p13 == UXG || isNull(t1->p2))
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
					t1->p15 = t1->p0;
					//
					int light = (int) modTuple(&t1->p2);
					int cycle = SIDE / t1->p12;
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
						t1->p3 = t2->p4;
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
					else if(pwm(t1->p14) && vacuum(t1))
					{
						t1->p6 = t2->p6;
						t1->p4 = t2->p4;
					}
					//
					// Axiom 14 - Coulomb interaction
					// Axiom 6 - Polarization
					//
					else if(pwm(t1->p14) && (light % cycle) < cycle / 8)
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						if(t1->p6 == t2->p6)
							invertTuple(&t1->p3);	// Repulsion
						//
						// turn off EM P
						//
						if(t1->p19 < t1->p20)
							t1->p6 -= 1;
					}
					//
					// Axiom 14 - Magnetic interaction
					// Axiom 6 - Polarization
					//
					else if(!isNull(t1->p4) && !isNull(t2->p4) && pwm(t1->p14)  && (light % cycle) < cycle / 4 && pwm(tupleDot(&t1->p4, &t2->p4) / SIDE - SIDE) / 2)
					{
						Tuple radial = t1->p2;
						subTuples(&radial, t2->p2);
						tupleCross(t1->p4, radial, &t1->p3);
						normalizeTuple(&t1->p3);
						//
						// turn off EM P
						//
						if(t1->p19 < t1->p20)
							t1->p6 -= 1;
					}
					//
					// Axiom 14 - Absorption interaction
					//
					else if(t1->p6 == -t2->p6 && !t1->p16 )
					{
						// Reissue all superposed preons
						//
						Brick *t2_ = dual;
						int w1 = t1->p19;
						for(int w2 = 0; w2 < NPREONS; w2++, t2_++)
							if(w1 != w2 && isEqual(t1->p2, t2_->p2))
							{
								t2_->p13 = REISSUE;
								puts("superposed");
							}
					}
					//
					// Axiom 14 - Strong interaction
					//
					else if(t1->p9 & t2->p9)
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
					else if(t1->p7 != 0 && t2->p7 != 0 &&
							t1->p7 == -conjug(t1) && t2->p7 == -conjug(t2) &&
							pwm(t1->p14) && pwm(t2->p14))
					{
						t1->p3 = t1->p2;
						subTuples(&t2->p3, t2->p2);
						//
						// Axiom 14 - Weak harvesting
						//
						if(vacuum(t1))
						{
							t2->p17 = SIDE;
							t1->p16 = true;
						}
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
					break;
				}
				if(t1->p13 == U && !t1->p22 && t2->p13 == P)
				{
					t1->p13 = UXP;
					t1->p15 = t1->p0;
					break;
				}
				if(t1->p13 == P && t2->p13 == P && !isEqual(t1->p2, t2->p2))
				{
					t1->p13 = PXP;
					t1->p20 = w2;
					assert(t1->p20!=w2);puts("PxP");
					//
					// Vacuum-vacuum interaction
					//
					if(vacuum(t1) && vacuum(t2))
					{
						// puts("v-v");
					}
					//
					// Axiom 15 - Leptonic synthesis
					//
					else if(t1->p9 == 0x37 && vacuum(t2))
					{
						if(t1->p1 % 2)
						{
							t1->p9 = 0x38;
							t2->p9 = 0x07;
						}
						else
						{
							t1->p9 = 0x07;
							t2->p9 = 0x38;
						}
					}
					//
					// Axiom 13 - Cancellation
					//
					else if(pwm(tupleDot(&t1->p3, &t2->p3) / SIDE - SIDE) / 2)
					{
						cleanBrick(t1);
					}
					//
					// Axiom 13 - Gluon-gluon interaction
					//
					else if(t1->p9 && t2->p9)
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

char getVoxel(Brick *p, Brick *d)
{
	char color = gridcolor;
	for(int w = 0; w < NPREONS; w++, p++, d++)
	{
		copyBrick(d, p);
		//
		// Bump cell clock
		//
		d->p1++;
		if(d->p1 == BURST)
			d->p18 = 0;
		//
		// Calculate voxel color
		//
		if(p->p25)
			color = BB;	// (p->p18 & 0x0f)+6;
		else if(p->p21 & GRAV)
			color = GG;
		else if(p->p21 & PREON)
			color = 7 + p->p19;
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
			// Axiom 13 - Launch reconfiguration burst
			//
			dual->p25 = CANCEL;
			resetTuple(&dual->p26);
		}
		else if(!interaction(dual))
		{
			expandPreon();
			expandGraviton();
		}
	}
	pri = p; dual = d;
}

/*
 * Executes one cycle of the automaton program.
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
		if(stop && timer % 2 == 0)
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
			img_changed = true;
    	    pthread_mutex_unlock(&mutex);
		}
	}
	return NULL;
}
