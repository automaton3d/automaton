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

// The lattices

Brick *pri0, *dual0;
Brick *pri, *dual;
int limit;
char rotateRight[] = { 0, 4, 1, 0, 2, 0, 0, 0 };
char qcd[] = { 0x3f, 0x01, 0x02, 0x04, 0x20, 0x10, 0x08, 0x3f };
boolean img_changed;

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
boolean isAllowed(int dir, Tuple p, unsigned char d0)
{
	double d1 = mod2Tuple(&p);
	int x = p.x + dirs[dir].x;
	int y = p.y + dirs[dir].y;
	int z = p.z + dirs[dir].z;
	double d2 = x*x + y*y + z*z;
	if(d2 <= d1)
		return false;
	if(x == SIDE/2 || x == -SIDE/2-1 || y == SIDE/2 || y == -SIDE/2-1 || z == SIDE/2 || z == -SIDE/2-1)
		return false;
	return true;
}

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
	return t->p22 && isNull(t->p3) && isNull(t->p4) && t->p8==NOGRAV && !t->p10 && t->p16==FREE && (t->p21 & PREON);
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
	t->p21 |= GRAV;
	//
	// Axiom 7 - Vacuum symmetry breaking
	//
	if(t->p13 == REISSUE)
	{
		printf("%ld: REISSUE %s\n", timer, tuple2str(&t->p0));
		resetTuple(&t->p2);
		t->p13 = UNDEF;
		t->p17 = SIDE;
		return true;		// Inhibit expansion
	}
	if(t->p13 == VACUUM)
	{
		// P <- P0
		// (P returns to vacuum)
		//
		resetTuple(&t->p3);
		resetTuple(&t->p4);
		t->p10 = 0;
		t->p16 = FREE;
		resetTuple(&t->p2);
		t->p13 = UNDEF;
		t->p17 = SIDE;
		return true;		// Inhibit expansion
	}
	if(t->p13 == ANNIHIL)
	{
		resetTuple(&t->p2);
		t->p13 = UNDEF;
		t->p17 = SIDE;

		Brick *peer = dual0 + (SIDE2 * t->p0.x + SIDE * t->p0.y + t->p0.z) * NPREONS + t->p20;
		if(t->p19 > peer->p19)
		{
			t->p3 = peer->p3;
			t->p4 = peer->p4;
		}
		else
		{
			peer->p3 = t->p3;
			peer->p4 = t->p4;
		}
		t->p10 = 0;
		t->p16 = FREE;





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
	if(t->p8 == GENGRAV)
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
			if(nual->p18 > pri->p18)
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
		dual->p8 = NOGRAV;
		dual->p10 = 0;
		dual->p16 = FREE;
		//
		dual->p18 = 0;
	}
	dual->p25 = UNDEF;							// leave no burst track
}

/*
 * Axiom 1
 */
void expandPreon()
{
	if((pri->p21 & (PREON | GRAV)) == 0 || pri->p1 <= pri->p24)
		return;
	//
	assert(timer%SYNCH==pri->p1%SYNCH);
	if(isNull(pri->p2))
		dual->p1 %= SYNCH;
	//
	// Tree expansion
	//
	int bestDot = -3*SIDE2/4;
	Brick *bestNual = NULL;
	boolean isGrav = pri->p21 & GRAV;
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p2, pri->p23))
		{
			Brick *nual = getNeighbor(dir, dual);
			if(nual->p21 & GRAV)
			{
				nual->p21 &= ~GRAV;
				Tuple p2 = nual->p2;
				normalizeTuple(&p2);
				int dot = tupleDot(&pri->p4, &p2);
				if(dot > bestDot)
				{
					bestDot = dot;
					bestNual = nual;
				}
				continue;
			}
			if(nual->p21 & PREON)
				continue;
			copyBrick(nual, dual);
			addTuples(&nual->p2, dirs[dir]);	// update origin vector
			if(isGrav)
			{
				Tuple p2 = nual->p2;
				normalizeTuple(&p2);
				int dot = tupleDot(&pri->p4, &p2);
				if(dot > bestDot)
				{
					bestDot = dot;
					bestNual = nual;
				}
			}
			nual->p23 = dir;
			nual->p24 = SYNCH * (modTuple(&nual->p2) + 0.5);
			int distance = (int) modTuple(&nual->p2) / (2 * DIAMETER);
			if(distance > 0)
			{
				// Axiom 4 - Interference
				// (exponential decay)
				//
				if(pri->p14 > 0)
				{
					dual->p14 *= (SIDE - SIDE / (2*distance));
					if(dual->p14 < 0)
						dual->p14 = 0;
				}
				else if(dual->p14 < 0)
				{
					dual->p14 *= (SIDE + SIDE / (2*distance));
					if(dual->p14 > 0)
						dual->p14 = 0;
				}
			}
			//
			// Axiom 5 - Virtual decay of P
			//
			nual->p17 >>= 1;
			nual->p21 &= ~GRAV;					// erase GRAV bit
		}
	}
	if(isGrav && bestNual)
	{
		bestNual->p21 |= GRAV;
		addMarker(pri->p0);
		if(bestNual->p16 == FREE &&
		   bestNual->p8 == NOGRAV &&
		   bestNual->p17 == 0 &&
		   (bestNual->p7 == -1 || bestNual->p9 != LEPT || bestNual->p9 != ANTILEPT) &&
		   !isNull(bestNual->p4))
		{
			bestNual->p13 = VACUUM;
		}
	}
	//
	// Check wrapping
	//
	if((int)modTuple(&pri->p2) == limit && pri->p23 == 3)
	{
		// Reissue
		//
		dual->p13 = UNDEF;
		resetTuple(&dual->p2);
		//
		// Undo EMP
		//
		dual->p6s = 0;
		dual->p21 |= GRAV;
	}
	else
	{
		cleanBrick(dual);
	}
}

/*
 * Tests if t1 and t2 form a new P.
 */
boolean isP(Brick *t1, Brick *t2)
{
	return
		(t2->p21 & PREON)			&&
		!isNull(t2->p2)				&&
		isEqual(t1->p2, t2->p2) 	&&
//		t1->p5 == t2->p5			&&
		t1->p6 == -t2->p6			&&
		t1->p7 == -t2->p7			&&
		t1->p8 == t2->p8			&&
		t1->p9 == (~t2->p9 & 0x3f)	&&
		t1->p16 == t2->p16;
}

Brick *complement(Brick *b)
{
	return dual0 + (SIDE2*b->p0.x + SIDE*b->p0.y + b->p0.z)*NPREONS + b->p20;
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 7 - Detection by mutual comparison
 */
void classify1()
{
	if(pri->p1 % SYNCH < SYNCH-1 || pri->p13 == VACUUM)
		return;
	//
	Brick *t1 = dual, *t2;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p21 == UNDEF || isNull(t1->p2) || pri->p25)
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
			if(w1 != w2 && t2->p21 == GRAV && t1->p8 == GENGRAV && !isEqual(t1->p2, t2->p2))
				nuxg++;
		if(nuxg)
		{
			// Select unambiguous peer
			//
			nuxg >>= 1;
			t2 = t1;
			w2 = t1->p19;
			Brick *peer = NULL;
			while(nuxg)
			{
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p21 == GRAV && t1->p8 == GENGRAV && !isEqual(t1->p2, t2->p2))
				{
					nuxg--;
					peer = t2;
					t1->p20 = w2;
					t2->p20 = t1->p19;
				}
			}
			//
			// Axiom 12 - Graviton detection
			//
			t1->p13 = peer->p13 = UXG;
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
			Brick *peer = NULL;
			while(npair)
			{
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(w1 != w2 && isP(t1, t2))
				{
					npair--;
					if(!t1->p22 && !t2->p22)
					{
						t1->p20 = w2;
						peer = t2;
					}
					//
					//  Axiom 3 - Sinusoidal phase
					// (bumped multiple times if superposition)
					//
					incrDFO(t1);
				}
			}
			if(peer != NULL)
			{
				t1->p13 = peer->p13 = P;
				t1->p22 = peer->p22 = true;
			}
			continue;
		}
		//
		// Detect UXU
		//
		nuxu = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t1->p8 == t2->p8 && (t2->p21 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2) &&
			   t1->p13 != REISSUE && t2->p13 != REISSUE && t1->p13 != PXP && t2->p13 != PXP)
				nuxu++;
		Brick *peer;
		if(nuxu)
		{
			// Select unambiguous peer
			//
			nuxu >>= 1;
			if(nuxu == 0)
				nuxu = 1;
			t2 = t1;
			w2 = t1->p19;
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
					peer = t2;
					t2->p20 = w2;
				}
			}
			t2 = peer;
			//
			// Axiom 13 - UxU interaction
			//
			if(t1->p6 == -t2->p6 && t1->p7 == -t2->p7 && t1->p9 == (~t2->p9 & 0x3f))
			{
				// Annihilation
				//
				t1->p13 = t2->p13 = ANNIHIL;
			}
			else if(t1->p9 == t2->p9)
			{
				// Both Us are reissued
				//
				t1->p13 = t2->p13 = UXU;
				t1->p15 = t1->p0;
				t2->p15 = t1->p0;
				t1->p20 = t1->p19;
			}
		}
	}
}

/*
 * UXP interactions.
 */
boolean UXPaction(Brick *u, Brick *p)
{
	boolean grav = (u->p13 == UXG);
	//
	// Default interaction type
	//
	u->p13 = p->p13 = complement(p)->p13 = UXP;
	//
	// Reissue occurs at the contact point
	//
	u->p15 = u->p0;
	p->p15 = p->p0;
	complement(p)->p15 = p->p0;
	//
	// Used to calculate polarization
	//
	int light = (int) modTuple(&p->p2);
	int cycle = SIDE / p->p12;
	//
	// Axiom 12 - Gravitational acceleration
	// (mandatory operation)
	//
	if(grav && vacuum(p))
	{
		// P0 is on-shell
		//
		p->p16 = complement(p)->p16 = BOUND;
		//
		// Mark U as consumed
		//
		u->p13 = UNDEF;
		//
		// P is aligned with spin of origin G
		//
		p->p3 = complement(p)->p3 = u->p4;
		return true;
	}
	//
	// Axiom 12 - EM interaction
	//
	else if(pwm(p->p14) && vacuum(p))
	{
		// The P acquires static boson properties
		//
		p->p6s = complement(p)->p6s = u->p6;
		p->p4 = complement(p)->p4 = u->p4;
		return true;
	}
	//
	// Axiom 12 - Coulomb interaction
	// Axiom 6 - Polarization
	//
	else if(pwm(p->p14) && (light % cycle) < cycle / 8)
	{
		p->p3 = complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		if(p->p6 == u->p6)
		{
			// Repulsion
			//
			invertTuple(&p->p3);
			invertTuple(&complement(p)->p3);
		}
		//
		// turn off EM P
		//
		p->p6s = 0;
		return true;
	}
	//
	// Axiom 12 - Magnetic interaction
	// Axiom 6 - Polarization
	//
	else if(!isNull(p->p4) && !isNull(u->p4) && pwm(p->p14) && (light % cycle) < cycle / 4 && pwm(tupleDot(&p->p4, &u->p4) / SIDE - SIDE) / 2)
	{
		Tuple radial = p->p2;
		subTuples(&radial, u->p2);
		tupleCross(p->p4, radial, &p->p3);
		complement(p)->p4 = p->p4;
		normalizeTuple(&p->p3);
		normalizeTuple(&complement(p)->p3);
		//
		// turn off EM P
		//
		if(p->p19 < p->p20)
			p->p6 = -p->p6;
		return true;
	}
	//
	// Axiom 12 - Absorption interaction
	//
	else if(p->p6 == -u->p6 && p->p16 == FREE)
	{
		// Reissue all superposed preons
		//
		Brick *t2_ = dual;
		int w1 = p->p19;
		for(int w2 = 0; w2 < NPREONS; w2++, t2_++)
			if(w1 != w2 && isEqual(p->p2, t2_->p2))
			{
				t2_->p13 = REISSUE;
				puts("superposed");
			}
		return true;
	}
	//
	// Axiom 12 - Strong interaction
	//
	else if(p->p9 & u->p9)
	{
		p->p3 = complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		//
		// Axiom 11 - Color exchange
		//
		exchangeColors(p, u);
		return true;
	}
	//
	// Axiom 12 - Weak force
	//
	else if(p->p7 != 0 && u->p7 != 0 &&
			p->p7 == -conjug(p) && u->p7 == -conjug(u) &&
			pwm(p->p14) && pwm(u->p14))
	{
		p->p3 = complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		//
		// Axiom 14 - Weak harvesting
		//
		if(vacuum(p))
		{
			u->p17 = SIDE;
			p->p16 = complement(p)->p16 = BOUND;
		}
		return true;
	}
	//
	// Axiom 12 - Inertia
	//
	else if(p->p16 == BOUND)
	{
		// Parallel transport
		//
		subRectify(&p->p15, p->p2);
		addRectify(&p->p15, u->p3);
		complement(p)->p15 = p->p15;
		return true;
	}
	//
	// Consume unused preons
	//
	u->p13 = p->p13 = complement(p)->p13 = UNDEF;
	return false;
}

boolean PXPaction(Brick *p1, Brick *p2)
{
	p1->p13 = p2->p13 = REISSUE;
	Brick *p1c = complement(p1);
	Brick *p2c = complement(p2);
	//
	// Axiom 13 - LM alignment
	//
	if(pwm(tupleDot(&p1->p3, &p2->p3) / SIDE - SIDE) / 2)	// p3.p3 ~ +1
	{
		p1->p16 = p2->p16 = FREE;
	}
	else if(isEqual(p1->p2, p2->p2) && vacuum(p1) && vacuum(p2) && broken(p1, p2))
	{
		// Synthesis
		//
		puts("Synthesis");
	}
	else if(vacuum(p1) && vacuum(p2))
	{
		puts("vac-vac");
	}
	else if(p1->p9 == LEPT && p1c->p9 == ANTILEPT && vacuum(p2))
	{
		// Axiom 15 - Leptonic synthesis
		//
		if(p1->p1 % 2)
		{
			p1->p9 = 0x38;
			p2->p9 = 0x07;
		}
		else
		{
			p1->p9 = 0x07;
			p2->p9 = 0x38;
		}
		puts("leptonic synthesis");
	}
	else if(pwm(tupleDot(&p1->p3, &p2->p3) / SIDE - SIDE) / 2)
	{
		// Axiom 13 - Cancellation
		//
		// P1 <- P0
		// P2 <- P0
		//
		resetTuple(&p1->p3);
		resetTuple(&p2->p3);
		resetTuple(&p1->p4);
		resetTuple(&p2->p4);
		p1->p8 = NOGRAV;
		p2->p8 = NOGRAV;
		p1->p10 = 0;
		p2->p10 = 0;
		p1->p16 = p2->p16 = FREE;
		//
		resetTuple(&p1c->p3);
		resetTuple(&p2c->p3);
		resetTuple(&p1c->p4);
		resetTuple(&p2c->p4);
		p1c->p8 = NOGRAV;
		p2c->p8 = NOGRAV;
		p1c->p10 = 0;
		p2c->p10 = 0;
		p1c->p16 = p2c->p16 = FREE;
		puts("Px <- P0");
	}
	else if(p1->p9 != LEPT && p1c->p9 != ANTILEPT && p2->p9 != LEPT && p2c->p9 != ANTILEPT)
	{
		// Gluon-gluon interaction
		//
		exchangeColors(p1, p2);
		puts("gluon-gluon");
	}
	else
	{
		p1->p13 = p2->p13 = PXP;
		return false;
	}
	return true;
}

/*
 * UxP, PXP
 * Axiom 7 - Interaction detection
 */
void classify2()
{
	if(pri->p1 % SYNCH < SYNCH-1 || pri->p13 == VACUUM)
		return;
	//
	Brick *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p13 != U || t1->p13 != P || isNull(t1->p2) || pri->p25)
			continue;
		//
		Brick *t2 = dual;
		for(int w2 = 0; w2 < NPREONS; w2++, t2++)
		{
			if(w1 != w2)
			{
				if(isNull(t2->p2))
					continue;
				//
				// UXP interaction
				//
				else if(t1->p13 == P && t2->p13 == U && UXPaction(t2, t1))
					break;
				else if(t1->p13 == U && t2->p13 == P && UXPaction(t1, t2))
					break;
				//
				// PxP interaction
				//
				else if(t1->p22 && t2->p22 && t1->p20 != t2->p20 && t1->p13 != PXP && t2->p13 != PXP && t1->p13 != REISSUE && t2->p13 != REISSUE && PXPaction(t1, t2))
					break;
			}
		}
		//
		// Disregard intermediate values
		//
		if(t1->p13 != UXP && t1->p13 != PXP && t1->p13 != REISSUE)
			t1->p13 = UNDEF;
	}
}

char getVoxel()
{
	Brick *p = pri;
	Brick *d = dual;
	char color = gridcolor;
	for(int w = 0; w < NPREONS; w++, p++, d++)
	{
		copyBrick(d, p);
		//
		// Bump cell clock
		//
		d->p1++;
		if(p->p25)
		{
			// p18 look-ahead
			//
			int best_p18 = p->p18;
			int best_signature = signature(p);
			for(int i = 0; i < NDIR; i++)
			{
				Brick *nnb = getNeighbor(i, p);
				if(nnb->p25 && nnb->p27 == opposite(i))
				{
					if(nnb->p18 > best_p18)
						best_p18 = nnb->p18;
					if(signature(nnb) > best_signature)
						best_signature = signature(nnb);
				}
			}
			if(best_p18 > p->p18)
				d->p18 = best_p18;
			else if(best_p18 == p->p18 && signature(p) > best_signature)
				d->p18 = 10000000;
		}
		else
		{
			d->p18 = 0;
		}
		//
		// Calculate voxel color
		//
		if(p->p25)
			color = BB;
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
		draft[p3d] = getVoxel();
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			classify1();
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			classify2();
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
