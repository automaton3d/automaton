/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include <stdio.h>
#include <math.h>
#include "automaton.h"
#include "tuple.h"
#include "params.h"
#include "tile.h"
#include "vector3d.h"
#include "utils.h"
#include "rotation.h"

// The lattice

Tile *principal, *dual;
Tile grid0 [SIDE4];
Tile grid1 [SIDE4];
Tuple dirs[6];

// Sine amplitude

double wT = 2 * PI / SIDE;
double K, U1, U2;

// The clock
// Conceptually, each tile should have its own p1

unsigned long p1;

// Auxiliary

int w;
double mindist;
Tile *winner;
Vector3d realpos;
long t1;
double d1, d2;
int pos;

Tuple cp;				// contact point

void updateCP()
{
	cp.x = pos / SIDE3;
	cp.y = pos / SIDE2 % SIDE;
	cp.z = pos / SIDE % SIDE;
	// w = pos % SIDE;
}

/*
 * Initializes sine wave parameters.
 * Axiom: sine
 */
void initSineWave()
{
	K = 2 * cos(wT);
	U1 = SIDE * sin(-2 * wT);
	U2 = SIDE * sin(-wT);
}

boolean pwm(int n)
{
    return (n % STEP) < (n / NSTEPS);
}

void resetDFO(Tile *t)
{
	t->p141 = U1;
	t->p142 = U2;
}

void incrDFO(Tile *t)
{
	int u3 = K * t->p142 - t->p141;
	t->p141 = t->p142;
	t->p142 = u3;
}

boolean isMatter(unsigned color)
{
	return false;
}

boolean filterEM()
{
	return false;
}

boolean emTest()
{
	return false;
}

void entangle(Tile *t1, Tile *t2)
{
	t1->p13 = t2->p13 = t1->p5 * t1->p5 + SIDE;
}

boolean tangled(Tile *t1, Tile *t2)
{
	if(abs(t2->p13 - t1->p13) > SIDE)
		return false;
	else
		return pwm(((t1->p13 - t1->p5 * t1->p5) * (t1->p13 - t1->p5 * t1->p5)) / SIDE);
}

int sgn(int n)
{
	if(n > 0)
		return 1;
	else if(n < 0)
		return -1;
	return 0;
}

boolean leptonFilter(Tile *neighbor)
{
	// 10% probability
	//
	return false;
}

boolean quarkFilter(Tile *neighbor)
{
	// 70% probability
	//
	return false;
}

boolean weakFilter1()
{
	return false;
}

boolean weakFilter2()
{
	return false;
}

boolean hadronFilter1()
{
	return false;
}

boolean hadronFilter2()
{
	return false;
}

/*
 * Builds an empty lattice.
 * Called twice. One for principal and one to dual grid
 */
void buildLattice(Tile *grid)
{
	Tile *ptr = grid;
	int x, y, z, w;
	for(x = 0; x < SIDE; x++)
		for(y = 0; y < SIDE; y++)
			for(z = 0; z < SIDE; z++)
				for(w = 0; w < SIDE; w++)
				{
					ptr->p5 = w;
					ptr->p2 = NIL;
					ptr->p202 = 0;
					ptr->p6.x = 0;
					ptr->p6.y = 0;
					ptr->p6.z = 0;
					//
					// Create channels to all neighbors
					//
					int p;
					p = x + 1;
					if(p == SIDE)
						p = 0;
					ptr->n[0] = grid + (SIDE3 * p +  + SIDE2 * y + SIDE * z + w);
					//
					p = x - 1;
					if(p < 0)
						p += SIDE;
					ptr->n[1] = grid + (SIDE3 * p +  + SIDE2 * y + SIDE * z + w);
					//
					p = y + 1;
					if(p == SIDE)
						p = 0;
					ptr->n[2] = grid + (SIDE3 * x +  + SIDE2 * p + SIDE * z + w);
					//
					p = y - 1;
					if(p < 0)
						p += SIDE;
					ptr->n[3] = grid + (SIDE3 * x +  + SIDE2 * p + SIDE * z + w);
					//
					p = z + 1;
					if(p == SIDE)
						p = 0;
					ptr->n[4] = grid + (SIDE3 * x +  + SIDE2 * y + SIDE * p + w);
					//
					p = z - 1;
					if(p < 0)
						p += SIDE;
					ptr->n[5] = grid + (SIDE3 * x +  + SIDE2 * y + SIDE * p + w);
					//
					p = w + 1;
					if(p == SIDE)
						p = 0;
					ptr->n[6] = grid + (SIDE3 * x +  + SIDE2 * y + SIDE * z + p);
					//
					ptr++;
				}
}

/*
 * Emits a bubble from the specified address of the grid0 lattice.
 */
Tile *addBubble(int x, int y, int z, int w,	// bubble address
		unsigned char p1, int p2, unsigned char p3, unsigned char p4, unsigned char p5,
		int sx, int sy, int sz)
{
	Tile *p = grid0 + (SIDE3 * x + SIDE2 * y + SIDE * z + w);
	p->p2 = p1;
	p->p8 = p2;
	p->p9 = p3;
	p->p10 = p4;
	p->p11 = p5;
	p->p12.x = sx;
	p->p12.y = sy;
	p->p12.z = sz;
	return p;
}

/*
 * Initializes the automaton program
 */
void init()
{
	initDirs();													// von Neumann directions
	initSineWave();												// sine wave generator
	buildLattice(grid0);										// principal/dual lattice
	buildLattice(grid1);										// principal/dual lattice
	//
	// Initial state of the universe
	// Axioms: weak-sharing, params
	//
	addBubble(SIDE/2,SIDE/2,SIDE/2,0, REAL, +1, 0, LEPTONIC, OFF, 0, 0, 0);
	//
	setvbuf(stdout, null, _IOLBF, 0);							// patch for Eclipse bug
}

void reemit(Tile *t, int p3, Tuple p19)
{
	t->p3 = p3;
	t->p202 = 0;
	resetTuple(&t->p6);
	tupleCopy(&t->p19, p19);
	resetDFO(t);
}

boolean burstInteraction()
{
	if(!(principal->p3 == PLAIN || principal->p3 == FORCING || principal->p3 == COLL))
		return false;
	//
	// Axiom: burst
	// Burst detected
	//
	if(isEqual(principal->p19, cp))
	{
		// The reemission point (seed) has been found
		//
		dual->p3 = OFF;
		dual->p11 = OFF;
		resetTuple(&dual->p6);
	}
	else
	{
		Tile *neighbor = principal - w;
		Tile *nual = dual - w;
		int j;
		for(j = 0; j < SIDE; j++, neighbor++)
		{
			if(isEqual(principal->p6, neighbor->p6) && tangled(principal, neighbor) && neighbor->p144)
			{
				nual->p144 = 0;
				incrDFO(neighbor);
			}
			// Axiom: burst
			//
			if(principal->p3 == FORCING && dual->p16 == U && neighbor->p16 == U && tangled(principal, neighbor))
			{
				updateCP();
				tupleCopy(&nual->p19, cp);
				subTuples(&nual->p19, neighbor->p6);
				reemit(neighbor, PLAIN, nual->p19);
			}
			else if(principal->p3 == COLL && dual->p16 == P && tangled(principal, neighbor))
			{
				updateCP();
				tupleCopy(&nual->p19, cp);
				subTuples(&nual->p19, principal->p6);
				reemit(neighbor, PLAIN, nual->p19);
			}
			else if(principal->p3 == FORCING && dual->p16 == Z && principal->p13 == neighbor->p13)
			{
				// Bread reemission
				//
				updateCP();
				tupleCopy(&nual->p19, cp);
				subTuples(&nual->p19, principal->p6);
				reemit(neighbor, PLAIN, nual->p19);
			}
			// Axiom: graviton
			//
			if(principal->p5 == neighbor->p5 && isEqual(principal->p6, neighbor->p6))
			{
				if(neighbor->p2 == REAL)
				{
					nual->p2 = GRAV;
					nual->p11 = OFF;
				}
				else
				{
					nual->p2 = NIL;
					resetDFO(neighbor);
				}
			}
			else if(principal->p13 != 0 && principal->p13 == neighbor->p13 && isEqual(principal->p12, neighbor->p12))
			{
				// Realign twins
				//
				tupleCopy(&nual->p12, dual->p12);
			}
			if(neighbor->p2 == REAL && principal == neighbor)
				nual->p11 = ON;
		}
		resetDFO(principal);
	}
	return true;
}

boolean interference()
{
	if(principal->p2 != NIL)
	{
		dual->p17 = 0;
		return false;
	}
	//
	// Interference I
	// The cell is not visited, so,
	// at each light step
	//
	if(LIGHT)
	{
		dual->p17++;
		//
		// Track: p18 decays absolutely and exponentially
		// (Sciarretta)
		//
		if(principal->p18 > 0)
		{
			dual->p18 *= (SIDE - SIDE / (2 * dual->p17));
			if(dual->p18 < 0)
				dual->p18 = 0;
		}
		else if(dual->p18 < 0)
		{
			dual->p18 *= (SIDE + SIDE / (2 * dual->p17));
			if(dual->p18 > 0)
				dual->p18 = 0;
		}
		//
		// Axiom: ex-conjure
		// Decrement TTL
		//
		dual->p18 = principal->p18 - 1;
		if(dual->p18 < 0)
			dual->p18 = 0;
	}
	return true;
}

void PXPdetection(Tile *neighbor, Tile *nual)
{
	// Axiom: boson-formation
	//
	if(principal->p13 != 0 && principal->p13 == neighbor->p13 && isNull(principal->p7) && isNull(neighbor->p7))
	{
		resetTuple(&dual->p6);
		resetTuple(&nual->p6);
		updateCP();
		reemit(principal, PLAIN, cp);
		reemit(neighbor, PLAIN, cp);
		incrDFO(principal);
		incrDFO(neighbor);
	}
	//
	// Axiom: cancel
	//
	else if(principal->p13 != 0 && principal->p13 == neighbor->p13 && dual->p16 == KNP && nual->p16 == KNP)
	{
		int proj = tupleDot(&principal->p7, &neighbor->p7);
		if(proj > 0 && filterEM())
		{
			// They revert to VCP
			//
			resetTuple(&dual->p7);
			resetTuple(&dual->p7);
			dual->p2 = nual->p2 = VIRT;
			dual->p13 = nual->p13 = 0;
			updateCP();
			reemit(principal, PLAIN, cp);
			reemit(neighbor, PLAIN, cp);
		}
	}
	//
	// Axiom: photon-formation
	//
	else if(principal->p13 != 0 && principal->p13 == neighbor->p13 && principal->p4 == ON && neighbor->p4 == OFF && emTest())
	{
		nual->p4 = ON;
	}
	//
	// Axiom:
	//
	else if(principal->p10 != LEPTONIC && principal->p10 != ANTILEPTONIC && neighbor->p10 != LEPTONIC && neighbor->p10 != ANTILEPTONIC)
	{
		updateCP();
		reemit(principal, PLAIN, cp);
		reemit(neighbor, PLAIN, cp);
	}
	else if(principal->p16 == PHP)
	{
		//
		// Axiom inertia
		// Non-leptonic Ps act like U in this regard
		//
		tupleCopy(&nual->p7, neighbor->p7);
		scaleTuple(&nual->p7, modTuple(&neighbor->p6));
		normalizeTuple(&nual->p7);
		tupleCopy(&nual->p19, nual->p7);
		addTuples(&nual->p19, neighbor->p6);
		//
		tupleCopy(&dual->p19, nual->p19);
		subTuples(&dual->p19, neighbor->p6);
		addTuples(&dual->p19, principal->p6);
		//
		reemit(dual, PLAIN, dual->p19);
		reemit(nual, PLAIN, dual->p19);
	}
}

void UXPdetection(Tile *neighbor, Tile *nual)
{
	nual->p16 = UXP;
	if(isEqual(principal->p6, neighbor->p6))
	{
		// Triad detected
		//
		nual->p16 = B;
		dual->p16 = Z;
		//
		// Axiom: return-c2u
		//
		if(modTuple(&principal->p6) >= LOST)
		{
			dual->p16 = U;
			nual->p16 = P;
			//
			// reemit(C, PLAIN, OP_C)
			//
			updateCP();
			tupleCopy(&dual->p19, cp);
			subTuples(&dual->p19, principal->p6);
			reemit(principal, PLAIN, dual->p19);
			//
			// reemit(B, PLAIN, OP_B)
			//
			tupleCopy(&nual->p19, cp);
			subTuples(&nual->p19, neighbor->p6);
			reemit(neighbor, PLAIN, nual->p19);
		}
	}
	else
	{
		Tuple v;
		boolean em;
		//
		// Calculate UXP
		//
		switch(dual->p22)
		{
			case KNP:
				//
				// Axiom: inertia
				//
				tupleCopy(&nual->p7, neighbor->p7);
				scaleTuple(&nual->p7, modTuple(&neighbor->p6));
				normalizeTuple(&nual->p7);
				tupleCopy(&nual->p19, nual->p7);
				addTuples(&nual->p19, neighbor->p6);
				//
				tupleCopy(&dual->p19, nual->p19);
				subTuples(&dual->p19, neighbor->p6);
				addTuples(&dual->p19, principal->p6);
				//
				reemit(dual, PLAIN, dual->p19);
				reemit(nual, PLAIN, dual->p19);
				break;
			case MGP:
				//
				// Axiom: uxmgp-interaction
				//
				dual->p3 = PLAIN;
				nual->p3 = PLAIN;
				if(neighbor->p10 != LEPTONIC)
					nual->p18 = SIDE;
				else if(neighbor->p9 != 0)
					nual->p18 = SIDE;
				break;
			case EMP:
				//
				// Axiom: electric-force (no polarization considered ??!!)
				//
				em = (principal->p11 & principal->p144 & pwm(neighbor->p18)) != 0;
				int sign = sgn(principal->p8 * neighbor->p8);
				if(em)
				{
					nual->p4 = ON;
					Tuple radial;
					subTuples3(&radial, principal->p6, neighbor->p6);
					tupleCopy(&nual->p7, radial);
					scaleTuple(&nual->p7, sign);
					entangle(dual, nual);
					//
					// reemit(U, PLAIN, OP_U)
					//
					updateCP();
					tupleCopy(&dual->p19, cp);
					subTuples(&dual->p19, principal->p6);
					reemit(principal, PLAIN, dual->p19);
					//
					// reemit(P, PLAIN, CP)
					//
					reemit(principal, PLAIN, cp);
				}
				//
				// Axiom: magnetic-force
				//
				int si = abs(tupleDot(&principal->p12, &neighbor->p12));
				if(em && pwm(si))
				{
					nual->p4 = ON;
					tupleCross(principal->p12, neighbor->p6, &nual->p7);
					scaleTuple(&nual->p7, sign);
					tupleCopy(&dual->p12, neighbor->p7);
					entangle(dual, nual);
					//
					// reemit(U, PLAIN, OP_U)
					//
					updateCP();
					tupleCopy(&dual->p19, cp);
					subTuples(&dual->p19, principal->p6);
					reemit(principal, PLAIN, dual->p19);
					//
					// reemit(P, PLAIN, CP)
					//
					reemit(principal, PLAIN, cp);
				}
				break;
			case PHP:
				//
				// Axiom: polarization
				//
				// Vector perpendicular to radial direction
				//
				tupleCross(neighbor->p12, neighbor->p6, &v);
				//
				// Synchronize polarization vector with sine wave
				//
				rotateSpin(&v, nual->p15E);
				//
				int c = tupleDot(&v, &principal->p12);
				dual->p15E = pwm(c * c);
				dual->p15M = pwm((SIDE - c) * (SIDE - c));
				//
				updateCP();
				em = (principal->p11 & principal->p144 & pwm(neighbor->p18)) != 0;
				if(em && dual->p15E && principal->p4 != neighbor->p4)
				{
					// Axiom: lightxmatter (electric)
					//
					reemit(neighbor, COLL, cp);
					tupleCopy(&dual->p19, cp);
					subTuples(&dual->p19, principal->p6);
					reemit(principal, PLAIN, dual->p19);
				}
				else if(em && dual->p15M && principal->p4 != neighbor->p4)
				{
					// Axiom: lightxmatter (magnetic)
					//
					reemit(neighbor, COLL, cp);
					tupleCopy(&dual->p19, cp);
					subTuples(&dual->p19, principal->p6);
					reemit(principal, PLAIN, dual->p19);
				}
				break;
			case VCP:
				nual->p13 = principal->p13;
				//
				// Vacuum interactions
				//
				if(principal->p11 == ON)
				{
					// Axiom: gravity-force
					//
					// Turn off gravity status
					//
					dual->p11 = OFF;
					//
					// Configure spin
					//
					tupleCopy(&nual->p12, principal->p12);
					if(principal->p5 < neighbor->p5)
						invertTuple(&nual->p12);
					//
					// The vacuum pair gets entangled with the U
					//
					nual->p13 = principal->p13;
					//
					// Form a KNP
					//
					updateCP();
					tupleCopy(&dual->p19, cp);
					subTuples(&dual->p19, principal->p6);
					//
					// Reemit U and P
					//
					reemit(dual, PLAIN, dual->p19);
					reemit(nual, PLAIN, cp);
				}
				else if(principal->p9 == RHM_LHAM && (principal->p10 == LEPTONIC || principal->p10 == ANTILEPTONIC))
				{
					// Axiom: stp-formation
					//
					nual->p2 = REAL;
					nual->p13 = principal->p13;
					//
					// Configure spin
					//
					tupleCopy(&nual->p12, principal->p12);
					if(principal->p5 < neighbor->p5)
						invertTuple(&nual->p12);
				}
				else if(dual->p16 != Z)
				{
					// Axiom: tr-formation
					//
					updateCP();
					tupleCopy(&dual->p19, cp);
					tupleCopy(&nual->p19, cp);
					nual->p13 = principal->p13;	// get entangled
					if(!isNull(principal->p12))
					{
						tupleCopy(&nual->p12, principal->p12);
						if(principal->p5 > neighbor->p5)
							invertTuple(&nual->p12);
						//
						reemit(principal, PLAIN, cp);
						reemit(neighbor, PLAIN, cp);
					}
					//
					// Axiom: strong tr
					//
					if(principal->p10 != LEPTONIC && principal->p10 != ANTILEPTONIC)
					{
						if(principal->p5 > neighbor->p5)
							nual->p10 = principal->p10;
						else
							nual->p10 = ~principal->p10;
					}
					//
					// Axiom: em-tr
					//
					else if(!isNull(principal->p12))
					{
						if(principal->p5 > neighbor->p5)
							nual->p8 = principal->p8;
						else
							nual->p8 = -principal->p8;
					}
					//
					// Axiom: weak-tr
					//
					else if(principal->p9 != 0)
					{
						if(principal->p5 > neighbor->p5)
							nual->p9 = principal->p9;
						else
							nual->p9 = -principal->p9;
					}
				}
				//
				// Axiom: weak-virtual-mgp
				//
				if(principal->p9 != 0)
				{
					tupleCopy(&nual->p12, principal->p12);
					tupleCopy(&nual->p7, principal->p7);
					nual->p9 = principal->p9;
					nual->p18 = SIDE;		// WTTL
					nual->p13 = principal->p13;	// get entangled
					if(principal->p5 > neighbor->p5)
					{
						invertTuple(&nual->p12);
						invertTuple(&nual->p7);
					}
				}
				//
				// Axiom: strong-virtual-mgp
				//
				if(principal->p10 != LEPTONIC && principal->p10 != ANTILEPTONIC)
				{
					nual->p18 = SIDE;		// STTL
					nual->p13 = principal->p13;	// get entangled
					if(principal->p8 == neighbor->p8)
					{
						// P=V1
						//
						if(principal->p1 % 2 == 0)
							nual->p10 = principal->p10;
					}
					else
					{
						// P=V2
						//
						if(principal->p1 % 2 == 0)
							nual->p10 = (principal->p10 >> 4) | (principal->p10 << 2);
						else
							nual->p10 = (principal->p10 << 4) | (principal->p10 >> 2);
						dual->p10 = (nual->p10 >> 3) | (nual->p10 << 2);
					}
				}
				else if(principal->p8 < 0)
				{
					// Axiom: am
					//
					invertTuple(&nual->p12);
				}
				else if(principal->p9 != 0)
				{
					// Axiom: weak-virtual
					// (weak MGP is formed)
					//
					// Configure spins
					//
					tupleCopy(&nual->p12, principal->p12);
					if(principal->p8 < neighbor->p8)
					{
						invertTuple(&nual->p12);
						//
						// Configure momentum direction
						//
						tupleCopy(&nual->p19, principal->p6);
					}
					else
					{
						// Configure inverse momentum direction
						//
						tupleCopy(&nual->p19, principal->p6);
						invertTuple(&nual->p19);
					}
					//
					// Copy weakness
					//
					nual->p9 = principal->p9;
					//
					// It is virtual
					//
					nual->p18 = SIDE;		// WTTL
				}
				else if(principal->p10 == RED || principal->p10 == GREEN || principal->p10 == BLUE || principal->p10 == ANTIRED || principal->p10 == ANTIGREEN || principal->p10 == ANTIBLUE)
				{
					// Axiom: strong-virtual
					// (strong MGP is formed)
					//
					if(principal->p8 == neighbor->p8)
					{
						nual->p10 = principal->p10;
					}
					else
					{
						if(p1 % 2 == 0)
							nual->p10 = (principal->p10 << 5) | (principal->p10 >> 1);
						else
							nual->p10 = (principal->p10 >> 5) | (principal->p10 << 1);
						nual->p10 &= 0x3f;
					}
					// Configure spins
					//
					tupleCopy(&nual->p12, principal->p12);
					if(principal->p8 < neighbor->p8)
					{
						invertTuple(&nual->p12);
						//
						// Configure momentum direction
						//
						tupleCopy(&nual->p19, principal->p6);
					}
					else
					{
						// Configure inverse momentum direction
						//
						tupleCopy(&nual->p19, principal->p6);
						invertTuple(&nual->p19);
					}
					//
					// It is virtual
					//
					nual->p18 = SIDE;	// STTL
				}
				else
				{
					// Axiom: default
					// (STP is formed)
					//
					nual->p2 = REAL;
				}
				break;

		}
	}
}

void UXGdetection(Tile *neighbor, Tile *nual)
{
	// Axiom: UXG-interaction
	//
	nual->p16 = UXG;
	dual->p11 = ON;
	tupleCopy(&dual->p7, neighbor->p6);
	invertTuple(&dual->p7);
}

void cohesion(Tile *neighbor, Tile *nual)
{
	// Axiom: cohesion
	//
	if(principal->p8 == neighbor->p8 && principal->p10 == neighbor->p10)
	{
		updateCP();
		if(principal->p5 > neighbor->p5)
		{
			// p19[1] = OP
			//
			tupleCopy(&dual->p19, cp);
			subTuples(&dual->p19, dual->p6);
			//
			// p19[2] = CP
			//
			tupleCopy(&nual->p19, cp);
		}
		else
		{
			// p19[1] = CP
			//
			tupleCopy(&dual->p19, cp);
			//
			// p19[2] = OP
			//
			tupleCopy(&nual->p19, cp);
			subTuples(&nual->p19, dual->p6);
		}
		reemit(dual, FORCING, dual->p19);
		reemit(nual, FORCING, nual->p19);
	}
}

void annihilation(Tile *neighbor, Tile *nual)
{
	// Axiom: annihil
	//
	updateCP();
	tupleCross(principal->p6, neighbor->p6, &nual->p12);
	if(principal->p5 > neighbor->p5)
		invertTuple(&nual->p12);
	entangle(dual, nual);
	reemit(dual, PLAIN, cp);
	reemit(nual, PLAIN, cp);
	//
	// Entangle preons
	//
	dual->p13 = nual->p13 = SIDE / 2;
}


/*
 * Step 1 in axiom: combination
 * [Burst,P,G] detection
 *
 * Field p21 contains the type of P
 */
void patterns1()
{
	updateCP();
	//
	if(burstInteraction() || interference())
		return;
	//
	// The remaining possibilities
	//
	if(principal->p2 == GRAV)
	{
		if(principal->p11)
		{
			dual->p16 = G;
			//
			// Axiom: g-extinction
			//
			if(isNull(principal->p6))
			{
				dual->p16 = ND;
				dual->p2 = NIL;
				dual->p11 = OFF;
			}
		}
		else
		{
			dual->p16 = ND;
		}
		return;
	}
	else
	{
		dual->p16 = U;
	}
	//
	Tile *neighbor = principal - w;
	Tile *nual = dual - w;
	int j;
	for(j = 0; j < SIDE; j++, neighbor++)
	{
		if(principal == neighbor || (neighbor->p2 != REAL && neighbor->p2 != VIRT))
			continue;
		//
		// Neighbor tile is REAL or VIRTUAL
		//
		if(isEqual(principal->p6, neighbor->p6))
		{
			// Preons are overlapping, have the same origin p6
			//
			if(isNull(principal->p12) && isNull(neighbor->p12) && principal->p2 == VIRT && neighbor->p2 == VIRT)
			{
				// Preons are virtual and spins are undefined, so its a VCP
				//
				dual->p16 = P;
				dual->p22 = VCP;
				return;
			}
			else if(isNull(principal->p6))
			{
				// Reemission, reorganize U
				//
				if(tangled(principal, neighbor))
				{
					if(leptonFilter(neighbor))
					{
						// Pair formation: P -> l l_bar
						//
						dual->p16 = U;
						nual->p16 = U;
						//
						// Reemit the U only to avoid overlapping
						//
						reemit(principal, PLAIN, cp);
					}
					else if(quarkFilter(neighbor))
					{
						// Pair formation: P -> q q_bar
						//
						dual->p16 = U;
						nual->p16 = U;
						//
						// Reemit the U only to avoid overlapping
						//
						reemit(principal, PLAIN, cp);
					}
				}
			}
			//
			// Neutrino detection NTP
			//
			if((isNull(principal->p12) || isNull(neighbor->p12)) && (!isNull(principal->p12) || !isNull(neighbor->p12)) && principal->p2 == REAL)
			{
				dual->p16 = NTP;
			}
			if(!isNull(principal->p12) && isOpposite(principal->p12, neighbor->p12))
			{
				// Spins are defined
				//
				if(principal->p2 == neighbor->p2)
				{
					dual->p16 = P;
					//
					// Detect FLP
					//
					if((principal->p9 == LEFTMATTER && isMatter(principal->p10)) && (neighbor->p9 == LEFTMATTER && isMatter(neighbor->p10)))
						dual->p22 = MGP;	// FLP
					else if((principal->p9 == RIGHTMATTER && !isMatter(principal->p10)) && (neighbor->p9 == RIGHTMATTER && !isMatter(neighbor->p10)))
						dual->p22 = P;
					//
					// Detect GLP
					//
					else if(principal->p10 == RED && neighbor->p10 == ANTIGREEN)
						dual->p22 = GLP;
					else if(principal->p10 == GREEN && neighbor->p10 == ANTIRED)
						dual->p22 = GLP;
					else if(principal->p10 == BLUE && neighbor->p10 == ANTIRED)
						dual->p22 = GLP;
					else if(principal->p10 == RED && neighbor->p10 == ANTIBLUE)
						dual->p22 = GLP;
					else if(principal->p10 == GREEN && neighbor->p10 == ANTIBLUE)
						dual->p22 = GLP;
					else if(principal->p10 == BLUE && neighbor->p10 == ANTIGREEN)
						dual->p22 = GLP;
					//
					// Detect KNP
					//
					else if(principal->p2 == REAL && principal->p13 != neighbor->p13 && (principal->p13 || neighbor->p13 == 0))
						dual->p22 = KNP;
					//
					// Detect MGP
					//
					else if(principal->p13 != 0 && neighbor->p13 != 0)
						dual->p22 = MGP;
					//
					// Detect MSP
					//
					else if(principal->p10 == RED && neighbor->p10 == ANTIRED)
						dual->p22 = MSP;
					else if(principal->p10 == GREEN && neighbor->p10 == ANTIGREEN)
						dual->p22 = MSP;
					else if(principal->p10 == BLUE && neighbor->p10 == ANTIBLUE)
						dual->p22 = MSP;
					//
					// Detect PHP
					//
					else if(principal->p10 == LEPTONIC && neighbor->p10 == ANTILEPTONIC)
						dual->p22 = PHP;
					else if(principal->p10 == ANTILEPTONIC && neighbor->p10 == LEPTONIC)
						dual->p22 = PHP;
					//
					// Detect STP
					//
					else if(principal->p2 == REAL)
						dual->p22 = EMP;
				}
			}
		}
	}
}

/*
 * Step 2 in axiom: combination
 * [UXP,UXG,Tr] detection
 */
void patterns2()
{
	if(principal->p16 == P)
	{
		Tile *neighbor = principal - w;
		Tile *nual = dual - w;
		int j;
		for(j = 0; j < SIDE; j++, neighbor++, nual++)
		{
			if(principal == neighbor)
				continue;
			//
			if(neighbor->p16 == P)
			{
				if(isNull(principal->p6) && neighbor->p16 == VCP)
				{
					// Reemission
					//
					if(weakFilter1())
					{
						// W / Z pair formation
						//
						dual->p8 = principal->p8;
						nual->p8 = -principal->p8;
						//
						dual->p16 = nual->p16 = MGP;
					}
					else if(weakFilter2())
					{
						dual->p16 = nual->p16 = WZBOSON;
					}
					else if(hadronFilter1())
					{
						dual->p16 = nual->p16 = HADRON;
					}
				}
				else
				{
					PXPdetection(neighbor, nual);
				}
			}
		}
		return;
	}
	else if(principal->p16 != U)
		return;
	//
	Tile *neighbor = principal - w;
	Tile *nual = dual - w;
	int j;
	for(j = 0; j < SIDE; j++, neighbor++, nual++)
	{
		if(principal == neighbor)
			continue;
		//
		if(neighbor->p16 == P)
			UXPdetection(neighbor, nual);
		else if(neighbor->p16 == G)
			UXGdetection(neighbor, nual);
	}
}

/*
 * Step 3 in axiom: combination
 * [UXU,UXTr] detection
 */
void patterns3()
{
	Tile *neighbor = principal - w;
	Tile *nual = dual - w;
	int j;
	for(j = 0; j < SIDE; j++, neighbor++, nual++)
	{
		if(principal == neighbor || dual->p16 != U || dual->p16 != Z)
			continue;
		//
		if(principal->p16 == WZBOSON && neighbor->p16 == VCP)
		{
			// Reemission
			//
			nual->p16 = MGP;
			continue;
		}
		if(principal->p16 == HADRON && neighbor->p16 == VCP)
		{
			// Reemission
			//
			nual->p16 = MGP;
			if(hadronFilter2())
			{
				// VCP2 -> GLP_bar
				//
			}
			else
			{
				// VCP2 -> MSP-
				//

			}
			continue;
		}
		if(principal->p8 != 0)
		{
			if(principal->p8 == neighbor->p8)
				cohesion(neighbor, nual);
			else if((principal->p10 & neighbor->p10) == 0)
				annihilation(neighbor, nual);
		}
		entangle(dual, nual);
	}
}

void updateMessenger(Tile *target, int dir)
{
	// Update propagation variables
	//
	target->p16 = principal->p16;
	target->p202 = principal->p202 + 1;
	target->p6.x = principal->p6.x + dirs[dir].x;
	target->p6.y = principal->p6.y + dirs[dir].y;
	target->p6.z = principal->p6.z + dirs[dir].z;
}

void updateWavefront(Tile *t, int dir)
{
	t->p144 = 1;		// sine phase changed
	//
	// Wavefront properties
	//
	t->p203  = principal->p203;
	t->p204  = (long) (2 * DIAMETER * d2 + 0.5);
	//
	// Update expanded suastica status
	//
	t->p202 = principal->p202 + 1;
	t->p6.x = principal->p6.x + dirs[dir].x;
	t->p6.y = principal->p6.y + dirs[dir].y;
	t->p6.z = principal->p6.z + dirs[dir].z;
	//
	if(principal->p2 == GRAV)
	{
		if(isNull(t->p6))
		{
			// G-extinction
			//
			t->p2 = NIL;
			t->p11 = OFF;
		}
		else
		{
			// Calculate distance to new candidate position
			//
			Vector3d v;
			v.x = t->p6.x;
			v.y = t->p6.y;
			v.z = t->p6.z;
			subVectors(&v, realpos);
			double dist = module3d(&v);
			//
			// Is this the shortest distance?
			//
			if(dist < mindist)
			{
				mindist = dist;
				winner = t;
				t1 = (long) (2 * DIAMETER * d2 + 0.5);
			}
		}
		if(winner != null)
		{
			// The winner neighbor is the new active graviton
			//
			winner->p203 = principal->p203;
			winner->p201 = dir;
			winner->p6.x = principal->p6.x;
			winner->p6.y = principal->p6.y;
			winner->p6.z = principal->p6.z;
			winner->p204  = t1;
			winner->p2 = GRAV;
			winner->p11 = ON;
		}
	}
}


boolean isFree(int pos)
{
	// Axioms: wavefront and tree
	// Is it a messenger or an interaction is in course?
	//
	if(dual->p16 != ND)
		return true;
	else
		return (p1 - principal->p203) > principal->p204;
}

/*
 * Updates the dual target preon
 *
 * param dir: direction of motion
 */
void updateDual(int dir)
{
	// Open the channel to the indicated neighbor in the dual grid
	//
	Tile *target = dual->n[dir];
	//
	// Luminal or superluminal?
	//
	if(principal->p3 != IDLE)
		updateMessenger(target, dir);
	else
		updateWavefront(target, dir);
}

/*
 * Expands the preon bubble (synchronized or not).
 */
void expand()
{
	// Is this preon a graviton?
	//
	if((principal->p2 & GRAV) != 0)
	{
		// Next position in Real space
		//
		realpos.x = principal->p12.x / (2 * DIAMETER * SIDE);
		realpos.y = principal->p12.y / (2 * DIAMETER * SIDE);
		realpos.z = principal->p12.z / (2 * DIAMETER * SIDE);
	}
	//
	mindist = 100000;
	winner = null;
	//
	d1 = modTuple(&principal->p6);
	//
	int s0 = (principal->p6.x * principal->p6.y < 0) ? 1 : 0;
	int s1 = (principal->p6.y * principal->p6.z < 0) ? 1 : 0;
	int s2 = (principal->p6.z * principal->p6.x < 0) ? 1 : 0;
	//
	int sx = sgn(principal->p6.x);
	int sy = sgn(principal->p6.y);
	int sz = sgn(principal->p6.z);
	//
	// Axiom: tree
	//
	if(principal->p6.x != 0)
	{
		if(principal->p6.y != 0)
		{
			if(principal->p6.z != 0)
			{
				// 3d spiral
				//
				switch(principal->p202 % 3)
				{
					case 0:
						if(sx < 0)
							updateDual(1);
						else
							updateDual(0);
						break;
					case 1:
						if(sy < 0)
							updateDual(3);
						else
							updateDual(2);
						break;
					case 2:
						if(sz < 0)
							updateDual(5);
						else
							updateDual(4);
						break;
				}
			}
			else
			{
				// xy plane
				//
				if(principal->p202 % 2 == s0)
				{
					if(sx < 0)
						updateDual(1);
					else
						updateDual(0);
				}
				else
				{
					if(sy < 0)
						updateDual(3);
					else
						updateDual(2);
				}
				//
				if(principal->p202 % 3 == 0)
				{
					updateDual(4);
					updateDual(5);
				}
			}
		}
		else if(principal->p6.z != 0)
		{
			// zx plane
			//
			if(principal->p202 % 2 == s2)
			{
				if(sz < 0)
					updateDual(5);
				else
					updateDual(4);
			}
			else
			{
				if(sx < 0)
					updateDual(1);
				else
					updateDual(0);
			}
			//
			if(principal->p202 % 3 == 2)
			{
				updateDual(2);
				updateDual(3);
			}
		}
		else
		{
			// x axis
			//
			if(sx < 0)
				updateDual(1);
			else
				updateDual(0);
			//
			if(principal->p202 % 2 == 0)
			{
				if(sx < 0)
					updateDual(2);
				else
					updateDual(3);
				//
				if(sx < 0)
					updateDual(5);
				else
					updateDual(4);
			}
			else
			{
				if(sx < 0)
					updateDual(3);
				else
					updateDual(2);
				//
				if(sx < 0)
					updateDual(4);
				else
					updateDual(5);
			}
		}
	}
	else if(principal->p6.y != 0)
	{
		if(principal->p6.z != 0)
		{
			// yz plane
			//
			if(principal->p202 % 2 == s1)
			{
				if(sy < 0)
					updateDual(3);
				else
					updateDual(2);
			}
			else
			{
				if(sz < 0)
					updateDual(5);
				else
					updateDual(4);
			}
			//
			if(principal->p202 % 3 == 1)
			{
				updateDual(0);
				updateDual(1);
			}
		}
		else
		{
			// y axis
			//
			if(sy < 0)
				updateDual(3);
			else
				updateDual(2);
			//
			if(principal->p202 % 2 == 0)
			{
				if(sy < 0)
					updateDual(1);
				else
					updateDual(0);
				//
				if(sy < 0)
					updateDual(4);
				else
					updateDual(5);
			}
			else
			{
				if(sy < 0)
					updateDual(0);
				else
					updateDual(1);
				//
				if(sy < 0)
					updateDual(5);
				else
					updateDual(4);
			}
		}
	}
	else if(principal->p6.z != 0)
	{
		// z axis
		//
		if(sz < 0)
			updateDual(5);
		else
			updateDual(4);
		//
		if(principal->p202 % 2 == 0)
		{
			if(sz < 0)
				updateDual(0);
			else
				updateDual(1);
			//
			if(sz < 0)
				updateDual(3);
			else
				updateDual(2);
		}
		else
		{
			if(sz < 0)
				updateDual(1);
			else
				updateDual(0);
			//
			if(sz < 0)
				updateDual(2);
			else
				updateDual(3);
		}
	}
	else
	{
		// origem
		//
		int i;
		for(i = 0; i < 6; i++)
			updateDual(i);
	}
}

/*
 * Updates dual preon based on its neighbors (same w) and siblings (same x,y,z).
 */
void updateCell(int pos)
{
	// In the box?
	//
	if(abs(principal->p6.x) < SIDE / 2 &&
	   abs(principal->p6.y) < SIDE / 2 &&
	   abs(principal->p6.z) < SIDE / 2)
	{
		// Process preon if exists
		//
		if(principal->p2 != NIL)
		{
			if(isFree(pos))
				expand();
			else
				copy(dual, principal);
		}
	}
	//
	// Go process next cell
	//
	principal++;
	dual++;
}

/*
 * Executes one cycle of the automaton program.
 */
void cycle()
{
	// Clean all dual cells
	// Axiom: flip
	//
	if(p1 % 2 == 0)
		dual = grid1;
	else
		dual = grid0;
	//
	int i;
	for(i = 0; i < SIDE4; i++)
		cleanCell(dual++);
	//
	// Commute roles of grids
	//
	if(p1 % 2 == 0)
	{
		principal = grid0;
		dual      = grid1;
	}
	else
	{
		principal = grid1;
		dual      = grid0;
	}
	//
	// Axiom: combination
	// Determine combination in 3 phases
	//
	for(pos = 0; pos < SIDE4; pos++)
		patterns1();
	for(pos = 0; pos < SIDE4; pos++)
		patterns2();
	for(pos = 0; pos < SIDE4; pos++)
		patterns3();
	//
	// Update all dual cells based on the data at the principal cells
	// (necessary in the central processor solution)
	//
	for(i = 0; i < SIDE4; i++)
		updateCell(i);
	//
	// Advance time
	//
	p1++;
}

/*
 *  Entry point.
 */
int main()
{
	init();
	while(true)
		cycle();
	return 0;
}
