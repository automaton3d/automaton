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
char rotateRight[] = { 0, 4, 1, 0, 2, 0, 0, 0 };
char qcd[] = { 0x3f, 0x01, 0x02, 0x04, 0x20, 0x10, 0x08, 0x3f };
boolean img_changed;
int limit;

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
	return t->p22 && isNull(t->p3) && isNull(t->p4) && t->p8==VIRTUAL && !t->p10 && t->p16==FREE && (t->p21 & PREON);
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

Brick *complement(Brick *b)
{
	return dual0 + (SIDE2*b->p0.x + SIDE*b->p0.y + b->p0.z)*NPREONS + b->p20;
}

boolean ready(Brick *b)
{
	return b->p13 & (PXP | UXU | UXP | UXG | REISSUE | VACUUM);
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
			if(nual->p25 && nual->p18 > pri->p18)
				continue;
			//
			// Axiom 13 - Cancellation
			//
			if(dual->p25 == CANCEL)
			{
				assert(nual->p1 == dual->p1);
				nual->p1 = dual->p1;	// ?????
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
		dual->p21 |= PREON;
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
		puts("cancellation");
		// Axiom 13 - Cancellation
		// P <- P0
		//
		resetTuple(&dual->p3);
		resetTuple(&dual->p4);
		dual->p8 = VIRTUAL;
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
	if((pri->p21 & (PREON | GRAV)) == 0 || pri->p1 <= pri->p24 || ready(pri))
		return;
	//
	if(isNull(pri->p2))
		dual->p1 %= SYNCH;
	//
	// Tree expansion
	//
	double bestDot = -1;
	Vector3d p4;
	p4.x = pri->p4.x;
	p4.y = pri->p4.y;
	p4.z = pri->p4.z;
	Brick *bestNual = NULL;
	boolean isGrav = pri->p21 & GRAV;
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p2, pri->p23))
		{
			Brick *nual = getNeighbor(dir, dual);
			if((nual->p21 & PREON)  == 0)
			{
				copyBrick(nual, dual);
				//
				// Axiom 5 - Virtual decay of P
				//
				nual->p17 >>= 1;
				//
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
				addTuples(&nual->p2, dirs[dir]);	// update origin vector
			}
			//
			nual->p21 &= ~GRAV;					// erase GRAV bit
			if(isGrav)
			{
				Vector3d p2;
				p2.x = nual->p2.x;
				p2.y = nual->p2.y;
				p2.z = nual->p2.z;
				norm3d(&p2);
				double dot = dot3d(p4, p2);
				if(dot > bestDot)
				{
					bestDot = dot;
					bestNual = nual;
				}
			}
		}
	}
	//
	// Propagate the graviton up to the wrapping limit
	//
	if(bestNual != NULL && (int)modTuple(&pri->p2) < limit)
		bestNual->p21 |= GRAV;
	//
	// Axiom 4 - Interference
	// (exponential decay)
	//
	if(pri->p16 == FREE && pri->p8 == VIRTUAL && pri->p17 == 0 &&
	   (pri->p7 == -1 || pri->p9 != LEPT || pri->p9 != ANTILEPT) && !isNull(pri->p4) && pri->p22)
	{
		if(pri->p2.x == pri->p2.y && pri->p2.y == pri->p2.z && pri->p2.x > 0)
		{
			puts("decay");
			dual->p13 = VACUUM;
			return;
		}
	}
	//
	// Check wrapping
	//
	if(pri->p2.x == -SIDE/2 && pri->p2.y == -SIDE/2 && pri->p2.z == -SIDE/2)
	{
		resetTuple(&dual->p2);
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
		t1->p6 == -t2->p6			&&
		t1->p7 == -t2->p7			&&
		t1->p8 == t2->p8			&&
		t1->p9 == (~t2->p9 & 0x3f)	&&
		t1->p16 == t2->p16;
}

/*
 * Result: U, P, UXU, UXG
 * Axiom 7 - Detection by mutual comparison
 */
void classify1()
{
	Brick *t1 = dual, *t2;
	Brick *peer = NULL;
	int nuxg, npair, nuxu;
	int w2;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if(t1->p21 == UNDEF || t1->p21 == GRAV || isNull(t1->p2) || t1->p25 || t1->p13 == UXG)
		{
			assert(t1->p13 != UXG);
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
			if(w1 != w2 && t2->p21 == GRAV && t1->p8 == REAL && !isEqual(t1->p2, t2->p2) && !ready(t2))
				nuxg++;
		if(nuxg)
		{
			// Select unambiguous peer
			//
			nuxg >>= 1;
			if(nuxg == 0)
				nuxg = 1;
			t2 = t1;
			w2 = w1;
			while(nuxg)
			{
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(t2->p21 == GRAV && t1->p8 == REAL && !isEqual(t1->p2, t2->p2) && !ready(t2))
				{
					nuxg--;
				}
			}
			//
			// Axiom 12 - Graviton detection
			//
			t1->p13 = UXG;
			t1->p29 = true;
			t1->p3  = t2->p3;	// U inherits G's LM direction
			t1->p25 = DESTROY;
			//
			// Delay graviton by one light step to avoid conflict
			//
			assert(t2->p1 >= SYNCH);
			t2->p1 -= SYNCH;
			t2->p24 += SYNCH;
			//
			// Reissue at contact point
			//
			t1->p15 = t1->p0;
			continue;
		}
		//
		// Detect P
		//
		npair = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && isP(t1, t2) && !ready(t1) && !ready(t2))
			{
				npair++;
				peer = t2;
			}
		if(npair)
		{
			// Select unambiguous peer
			//
			npair >>= 1;
			if(npair == 0)
				npair = 1;
			t2 = t1;
			w2 = w1;
			while(npair)
			{
				w2++; t2++;
				if(w2 == NPREONS)
				{
					w2 = 0;
					t2 = dual;
				}
				if(isP(t1, t2) && !ready(t1) && !ready(t2))
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
			t1->p13 = P;
			peer->p13 = P;
			t1->p22 = true;
			peer->p22 = true;
			t1->p20 = peer->p19;
			peer->p20 = t1->p19;
			continue;
		}
		//
		// Detect UXU
		//
		nuxu = 0;
		t2 = dual;
		for(w2 = 0; w2 < NPREONS; w2++, t2++)
			if(w1 != w2 && t1->p8 == t2->p8 && (t2->p21 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2) &&
			   !ready(t1) && !ready(t2) && !t2->p22)
				nuxu++;
		if(nuxu)
		{
			// Select unambiguous peer
			//
			nuxu >>= 1;
			if(nuxu == 0)
				nuxu = 1;
			t2 = t1;
			w2 = w1;
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
				if(t1->p8 == t2->p8 && (t2->p21 & PREON) && !isNull(t1->p2) && !isEqual(t1->p2, t2->p2) && !ready(t1) && !ready(t2) && !t2->p22)
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
			t1->p13 = UXU;
			t2->p13 = UXU;
			if(t1->p6 == -t2->p6 && t1->p7 == -t2->p7 && t1->p9 == (~t2->p9 & 0x3f))
			{
				// Annihilation: Reissue at contact point
				//
				t1->p15 = t1->p0;
				t2->p15 = t2->p0;
				//
				// They are peers now
				//
				t1->p20 = t2->p19;
				t2->p20 = t1->p19;
				//
				t1->p25 = DESTROY;
				t2->p25 = DESTROY;
				//
				if(t1->p19 > t2->p19)
				{
					t1->p3 = t2->p3;
					t1->p4 = t2->p4;
					invertTuple(&t1->p3);
					invertTuple(&t1->p4);
				}
				t1->p10 = 0;
				t1->p16 = FREE;
				//
				t2->p10 = 0;		// cuidado para não atropelar nonlocal
				t2->p16 = FREE;
				puts("annihil");
			}
			else if(t1->p9 == t2->p9)
			{
				// Reissue both at contact point
				//
				t1->p15 = t1->p0;
				t2->p15 = t1->p0;
				//
				// They are peers now
				//
				t1->p20 = t2->p19;
				t2->p20 = t1->p19;
				//
				t1->p25 = DESTROY;
				t2->p25 = DESTROY;
				assert(!isEqual(t1->p2, t2->p2));
			}
		}
	}
}

/*
 * UXP interactions.
 */
boolean UXPaction(Brick *u, Brick *p)
{
	if(isEqual(u->p2, p->p2))
		return false;
	//
	// Used to calculate polarization
	//
	int light = (int) modTuple(&p->p2);
	int cycle = SIDE / p->p12;
	boolean capture = false;
	//
	// Axiom 12 - Gravitational acceleration
	// (mandatory operation)
	//
	if(u->p29 && vacuum(p))
	{
		capture = true;
		//
		// P0 is on-shell now
		//
		p->p16 = BOUND;
		complement(p)->p16 = BOUND;
		p->p8 = REAL;
		complement(p)->p8 = REAL;
		//
		// P is aligned with spin of origin G
		//
		assert(!isNull(u->p4));
		p->p3 = u->p4;
		complement(p)->p3 = u->p4;
		//
		// The U cannot capture vacuum anymore
		//
		u->p29 = false;
	}
	//
	// Axiom 12 - EM interaction
	//
	else if(vacuum(p) || (pwm(p->p14) && vacuum(p)))
	{
		// The vacuum P acquires static boson properties
		//
		p->p6s = u->p6;
		complement(p)->p6s = u->p6;
		p->p4 = u->p4;
		complement(p)->p4 = u->p4;
		invertTuple(&complement(p)->p4);
		//
		// The P is turned on-shell
		//
		p->p8 = REAL;
		complement(p)->p8 = REAL;
	}
	//
	// Axiom 12 - Coulomb interaction
	// Axiom 6 - Polarization
	//
	else if(pwm(p->p14) && (light % cycle) < cycle / 8)
	{
		printf("Coulomb %ld\n", timer);
		p->p3 = p->p2;
		complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		if(p->p6 == u->p6)
		{
			// Repulsion
			//
			invertTuple(&p->p3);
			invertTuple(&complement(p)->p3);
		}
		//
		// Turn EM boson into KNP
		//
		p->p16 = BOUND;
		complement(p)->p16 = BOUND;
		//
		// turn off EM P
		//
		p->p6s = 0;
	}
	//
	// Axiom 12 - Magnetic interaction
	// Axiom 6 - Polarization
	//
	else if(!isNull(p->p4) && !isNull(u->p4) && pwm(p->p14) && (light % cycle) < cycle / 4 && pwm(tupleDot(&p->p4, &u->p4) / SIDE - SIDE) / 2)
	{
		printf("Magn. %ld\n", timer);
		//
		Tuple radial = p->p2;
		subTuples(&radial, u->p2);
		tupleCross(p->p4, radial, &p->p3);
		complement(p)->p4 = p->p4;
		normalizeTuple(&p->p3);
		normalizeTuple(&complement(p)->p3);
		//
		// Turn EM boson into KNP
		//
		p->p16 = BOUND;
		complement(p)->p16 = BOUND;
	}
	//
	// Axiom 12 - Absorption interaction
	//
	else if(p->p6 == -u->p6 && p->p16 == FREE && pwm(p->p14) && (light % cycle) < cycle / 8)
	{
		puts("Absorp.");
		//
		// Reissue all superposed preons
		//
		Brick *t2_ = dual;
		int w1 = p->p19;
		for(int w2 = 0; w2 < NPREONS; w2++, t2_++)
		{
			if(w1 != w2 && isEqual(p->p2, t2_->p2))
			{
				t2_->p13 = REISSUE;
				t2_->p25 = DESTROY;
				t2_->p15 = u->p0;
				puts("superposed");
			}
		}
	}
	//
	// Axiom 12 - Strong interaction
	//
	else if(isColored(u) && (p->p9 & u->p9))
	{
		puts("Strong");
		p->p3 = p->p2;
		complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		//
		// Axiom 11 - Color exchange
		//
		exchangeColors(p, u);
	}
	//
	// Axiom 12 - Weak force
	//
	else if(p->p7 != 0 && u->p7 != 0 &&
			p->p7 == -conjug(p) && u->p7 == -conjug(u) &&
			pwm(p->p14) && pwm(u->p14))
	{
		puts("Weak");
		p->p3 = p->p2;
		complement(p)->p3 = p->p2;
		subTuples(&u->p3, u->p2);
		//
		// Axiom 14 - Weak harvesting
		//
		if(vacuum(p))
		{
			u->p17 = SIDE;
			p->p16 = BOUND;
			complement(p)->p16 = BOUND;
		}
	}
	//
	// Axiom 12 - Inertia
	//
	else if(p->p16 == BOUND && p->p8 == u->p8)
	{
		puts("inertia");

		// Parallel transport
		//
		Tuple p3 = p->p3;
		scaleTuple(&p3, modTuple(&p->p2));
		//
		subRectify(&p->p15, p->p2);
		addRectify(&p->p15, p3);
		complement(p)->p15 = p->p15;
		//
		subRectify(&u->p15, u->p2);
		addRectify(&u->p15, p3);
	}
	else
	{
		// Consume unused preons
		//
		//puts("\tNOT USED");
		u->p13 = UNDEF;
		p->p13 = UNDEF;
		complement(p)->p13 = UNDEF;
		return false;
	}
	//
	// Interaction type
	//
	u->p13 = UXP;
	p->p13 = UXP;
	complement(p)->p13 = UXP;
	//
	// Burst type
	//
	u->p25 = DESTROY;
	p->p25 = DESTROY;
	complement(p)->p25 = DESTROY;
	//
	// Define reissue address
	//
	if(capture)
		u->p15 = getOrg(u);
	else
		u->p15 = u->p0;
	p->p15 = p->p0;
	complement(p)->p15 = p->p0;
	//
	return true;
}

boolean PXPaction(Brick *p1, Brick *p2)
{
	if(p1->p20 == p2->p19 || p2->p20 == p1->p19)
		return false;
	//
	p1->p13 = REISSUE;
	p2->p13 = REISSUE;
	Brick *p1c = complement(p1);
	Brick *p2c = complement(p2);
	//
	// Axiom 13 - LM alignment
	//
	Tuple m1 = p1->p3;
	Tuple m2 = p2->p3;
	normalizeTuple(&m1);
	normalizeTuple(&m2);
	int dot = tupleDot(&m1, &m2);
	if(dot > 0 && pwm(dot) && !isEqual(p1->p2, p2->p2))								// p3.p3 ~ +1
	{
		puts("alignment");
		p1->p16 = p2->p16 = FREE;
	}
	else if(isEqual(p1->p2, p2->p2) && vacuum(p1) && vacuum(p2) && broken(p1, p2))
	{
		// Vacuum symmetry broken: synthesis
		//
		puts("Synthesis");
	}
	else if(vacuum(p1) && vacuum(p2))
	{
		puts("quantum fluctuation");
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
	else if(dot < 0 && pwm(-dot))						// p3.p3 ~ -1
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
		p1->p8 = VIRTUAL;
		p2->p8 = VIRTUAL;
		p1->p10 = 0;
		p2->p10 = 0;
		p1->p16 = FREE;
		p2->p16 = FREE;
		//
		// P1' <- P0
		// P2' <- P0
		//
		resetTuple(&p1c->p3);
		resetTuple(&p2c->p3);
		resetTuple(&p1c->p4);
		resetTuple(&p2c->p4);
		p1c->p8 = VIRTUAL;
		p2c->p8 = VIRTUAL;
		p1c->p10 = 0;
		p2c->p10 = 0;
		p1c->p16 = FREE;
		p2c->p16 = FREE;
		puts("Px <- P0");
	}
	else if(p1->p9 != LEPT && p1->p9 != ANTILEPT && p2->p9 != LEPT && p2->p9 != ANTILEPT)
	{
		// Gluon-gluon interaction
		//
		exchangeColors(p1, p2);
		printf("gluon-gluon %d %d %d %d\n", p1->p9, p1c->p9, p2->p9, p2c->p9);
	}
	else
	{
		p1->p13 = UNDEF;
		p2->p13 = UNDEF;
		return false;
	}
	//
	// Reissue at the contact point
	//
	p1->p15 = p1->p0;
	p1c->p15 = p1->p0;
	p2->p15 = p1->p0;
	p2c->p15 = p1->p0;
	//
	// Launch bursts
	//
	p1->p25 = DESTROY;
	p1c->p25 = DESTROY;
	p2->p25 = DESTROY;
	p2c->p25 = DESTROY;
	return true;
}

/*
 * UxP, PXP
 * Axiom 7 - Interaction detection
 */
void classify2()
{
	Brick *t1 = dual;
	for(int w1 = 0; w1 < NPREONS; w1++, t1++)
	{
		if((t1->p13 != U && t1->p13 != P) || isNull(t1->p2) || t1->p25)
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
				else if(t1->p13 == U && t2->p13 == P && UXPaction(t1, t2))
					break;
				else if(t1->p13 == P && t2->p13 == U && UXPaction(t2, t1))
					break;
				//
				// PxP interaction
				//
				else if(t1->p22 && t2->p22 && t1->p20 != t2->p20 && t1->p13 != PXP && t2->p13 != PXP && t1->p13 != REISSUE && t2->p13 != REISSUE && PXPaction(t1, t2))
					break;
			}
		}
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
		//
		// Burst?
		//
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

void reissue()
{
	assert(dual->p25==DESTROY || dual->p25==CANCEL);
	//
	// Calculate new wake time
	//
	dual->p24 = dual->p1 - dual->p1 % SYNCH + SYNCH;
	//
	// Reset timeout of virtual particle
	//
	dual->p17 = SIDE;
	//
	// Deactivate EM boson
	//
	dual->p6s = 0;
	//
	// Axiom 7 - Spin rotation
	//
	rotateSpin(dual);
	//
	// Axiom 7 - Entanglement
	//
	Brick *peer = dual0 + (SIDE2 * dual->p0.x + SIDE * dual->p0.y + dual->p0.z) * NPREONS + dual->p20;
	dual->p10 = dual->p19 * peer->p19;
	//
	// Q-isso?
	//
	tupleCross(dual->p4, peer->p4, &dual->p4);
	if(dual->p19 > dual->p20)
		invertTuple(&dual->p4);
	//
	// Axiom 10 - Vacuum expansion
	//
	if(dual->p22 && vacuum(dual))
	{
		if(dual->p19 > dual->p20)
			dual->p9 = qcd[dual->p1 & 0x03];
		else
			dual->p9 = qcd[8 - (dual->p1 & 0x03)];
	}
	//
	// Real preon?
	//
	if(dual->p8 == REAL)
	{
		dual->p21 |= GRAV;		// Axiom 10 - Launch graviton
		dual->p4 = dual->p2;
	}
	//
	// Test specific cases
	//
	if(dual->p13 == VACUUM)
	{
		puts("vacuum");
		dual->p15 = dual->p0;
		//
		// P <- P0
		// (P returns to vacuum)
		//
		resetTuple(&dual->p2);
		resetTuple(&dual->p3);
		resetTuple(&dual->p4);
		dual->p10 = 0;
		dual->p16 = FREE;
		dual->p17 = SIDE;
	}
	dual->p13 = UNDEF;
	//
	// Axiom 8 - Launch burst
	//
	dual->p18 = signature(dual);
	resetTuple(&dual->p26);
	resetTuple(&dual->p2);
}

void expand()
{
	Brick *p = pri, *d = dual;
	if(pri->p1 % SYNCH < BURST)
		for(int w = 0; w < NPREONS; w++, pri++, dual++)
			expandBurst();
	else
		for(int w = 0; w < NPREONS; w++, pri++, dual++)
			if(!ready(dual))
				expandPreon();
			else
				reissue();
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
	if(pri->p1 % SYNCH == SYNCH-1)
	{
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				classify1();
		//
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				classify2();
	}
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
	usleep(30000);
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
