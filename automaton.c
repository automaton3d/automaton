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
#include "text.h"
#include "main3d.h"
#include "interaction.h"
#include "tree.h"

// The lattices

Brick *pri0, *dual0;
Brick *pri, *dual;
int limit;			// The maximum origin vector length possible
int period;			// Is the longest period of a preon

/*
 * Gets the neighbor's dual.
 */
Brick *getNual(int dir)
{
	switch(dir)
	{
		case 0:
			if(dual->a.x == SIDE-1)
				return dual - WRAP0;
			else
				return dual + WRAP1;
		case 1:
			if(dual->a.x == 0)
				return dual + WRAP0;
			else
				return dual - WRAP1;
		case 2:
			if(dual->a.y == SIDE-1)
				return dual - WRAP2;
			else
				return dual + WRAP3;
		case 3:
			if(dual->a.y == 0)
				return dual + WRAP2;
			else
				return dual - WRAP3;
		case 4:
			if(dual->a.z == SIDE-1)
				return dual - WRAP4;
			else
				return dual + WRAP5;
		case 5:
			if(dual->a.z == 0)
				return dual + WRAP4;
			else
				return dual - WRAP5;
	}
	return NULL;
}

/*
 * Burst expansion
 */
void forkBurst()
{
	if(!pri->bcode)
		return;

	printf("BBBBBBBBB==============\n");

	boolean match = !isNull(dual->p) && isEqual(pri->a, pri->seed);
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->burst, pri->bdir))
		{
			Brick *nual = getNual(dir);
			//
			// Burst conflict (look-ahead algorithm)
			//
			if(nual->bcode && tupleDot(&V0, &dual->o) < tupleDot(&V0, &nual->o))
				continue;
			//
			copyBrick(nual, dual);
			//
			// If seed brick at its destination, do not propagate anymore
			//
			if(match)
				resetTuple(&nual->p);
			//
			nual->bdir = dir;						// save direction
			addTuples(&nual->burst, dirs[dir]);		// update burst origin vector
			//
			if(pri->e && pri->e == nual->e)			// entangled?
			{
				// Nonlocal operations
				//
				nual->s = pri->s;
				invertTuple(&nual->s);
				//
				nual->p = pri->p;
				invertTuple(&nual->p);
			}
		}
	}
	if(!match)
		cleanBrick(dual);
	//
	dual->bcode = UNDEF;							// leave no track of burst
}

/*
 * Synchronized wavefront expansion.
 */
void forkWavefront()
{
	// If not ripe, return
	//
	if(isNull(pri->p) || pri->t <= pri->synch)
		return;
	//
	// Tree expansion
	//
	boolean disable = false;
	for(int i = 0; i < NDIR; i++)
	{
		// Map index to pseudorandom sequence
		//
		int dir = pri->dirs[i];
		//
		// Visit-once-tree
		//
		if(isAllowed(dir, pri->o, pri->dir))
		{
			Brick *nual = getNual(dir);
			copyBrick(nual, dual);
			if(disable)
				resetTuple(&nual->seed);
			nual->synch = SYNCH*(modTuple(&nual->o) + 0.5);
			nual->dir = dir;
			addTuples(&nual->o, dirs[dir]);			// update origin vector
			disable = true;
		}
	}
	//
	// Check wrapping
	//
	if(imod2(pri->o) == LIMIT2)
	{
		// Force reissue
		//
		resetTuple(&dual->o);
		dual->synch = SYNCH;
		dual->seed = dual->a;
		dual->t &= 1;
	}
	else
	{
		cleanBrick(dual);
	}
}

/*
 * Detect the P type.
 */
boolean detectP(Brick *b1, Brick *b2)
{
	if(isNull(b2->o) || !isEqual(b1->o, b2->o))
		return false;
	//
	unsigned char c1 = b1->R + b1->G + b1->B;
	unsigned char c2 = b2->R + b2->G + b2->B;
	unsigned char j1 = c1 < 2;
	unsigned char j2 = c2 < 2;
	//
	unsigned char q = b1->q ^ b2->q;
	unsigned char w = b1->c ^ b2->c;
	unsigned char g = b1->g ^ b2->g;
	unsigned char d = b1->d ^ b2->d;
	unsigned char j = j1 ^ j2;
	//
	unsigned char eden = j & q & w & g & d;
	unsigned char u = j & q & w & g & !eden & !d;
	unsigned char zone = j & q & w & !u & !eden & !d;
	//
	unsigned char Z = 0, W = 0, gluon = 0;
	if((c1 == 1 || c1 == 2) && (c2 == 1 || c2 == 2))
	{
		gluon = zone;
		eden = 0;
		u = 0;
		zone = 0;
	}
	else
	{
		Z = j & q & !zone & !u & !eden & !d;
		W = j & !Z & !zone & !u & !eden & !w & !d;
	}
	return (gluon << 5) | (eden<<4) | (u<<3) | (zone<<2) | (Z<<1) | W;
}

/*
 * Tries to match the two bricks to form a P.
 */
void matchPair(Brick *b1, Brick *b2, int offset)
{
	if(isNull(b2->p))
		return;
	//
	int type = detectP(b1, b2);
	if(type > 0)
	{
		if(b1->offset == 0)
		{
			if(b2->offset == 0)
			{
				b1->type = type;
				b2->type = type;
				b1->offset = offset;
				b2->offset = -offset;
			}
		}
		else if(b1->offset > offset || b1->type > b2->type)
		{
			Brick *b3 = dual + b1->offset;
			b3->type = type;
			b3->offset = 0;
			b1->type = type;
			b2->type = type;
			b1->offset = offset;
			b2->offset = -offset;
		}
	}
}

/*
 * Detect and execute collision between two preons.
 */
boolean collision(Brick *b1, Brick *b2, int offset)
{
	// Elements must both be seed and have distinct centers
	//
	if(isNull(b1->seed) || isNull(b2->seed) || isNull(b2->o) || isEqual(b1->o, b2->o) || b2->used)
		return false;
	//
	// Calculate quantum phase
	//
	int phi1, phi2;
	if(b1->e && b1->e==b2->e)
	{
		phi1 = b1->phi_sin + b1->xi;
		phi2 = b2->phi_sin + b2->xi;
	}
	else
	{
		phi1 = b1->phi_sin;
		phi2 = b2->phi_sin;
	}
	//
	// Enforce antimatter extinction
	//
	boolean j1 = (b1->R + b1->G + b1->B) < 2;
	boolean j2 = (b2->R + b2->G + b2->B) < 2;
	if((b1->d ^ j1) & (b2->d ^ j2))
	{
		phi1 = SIDE/2 - 1;
		phi2 = SIDE/2 - 1;
	}
	//
	// Mandatory gravity selection
	//
	if(b1->g == b2->g && b1->d == b2->d)
	{
		if(b1->m == 1 && b2->f == 1)
			axiomUxG(b1, b2);
		else if(b1->f == 1 && b2->m == 1)
			axiomUxG(b2, b1);
		else if(b1->m == 1 && b2->f > 1)
			axiomPxG(b1, b2);
		else if(b1->f > 1 && b2->m == 1)
			axiomPxG(b2, b1);
	}
	//
	// Try preon-preon interaction filter
	//
	else if(phi1*phi2 > 0 && pwm(abs(2*phi1))*pwm(abs(2*phi2)) && pwm(abs(2*dot(b1->s, b2->s))))
	{
		// Select and execute interaction
		//
		if(b1->f == 1 && b2->f == 1)
			axiomUxU(b1, b2);
		else if(b1->f == 1 && b2->f > 1)
			axiomUxU(b2, b1);
		else if(b1->f > 1 && b2->f == 1)
			axiomUxP(b1, b2);
		else if(b2->f > 1 && b1->f == 1)
			axiomUxP(b2, b1);
		else if(b1->f > 1 && b2->f > 1)
			axiomPxP(b1, b2);
	}
	else
	{
		return false;
	}
	return true;
}

void launchBurst()
{
	printf("--------BURST!!!!-------------\n");
	// Emit graviton if due
	//
	if(dual->d == dual->g)
		dual->m = true;
	//
	// Prepare preon for reissue
	//
	dual->t = 0;
	dual->seed = dual->a;
	resetTuple(&dual->o);
	if(dual->f == 2)
		resetTuple(&(dual+dual->offset)->o);
	dual->phi_sin = 0;
	dual->phi_cos = SIDE/2;
	dual->e = dual->w * dual->w;
	dual->synch = SYNCH;
	dual->type = UNDEF;
	//
	// Emit burst
	//
	dual->bcode = DESTROY;
	resetTuple(&dual->burst);
	dual->bdir = 0;
}

/*
 * Detect Ps.
 */
void convolve1(Brick *dual)
{
	// Circular comparison in the w dimension
	//
	int offset, w, ww;
	Brick *b1;
	for(b1 = dual, w = 0; w < NPREONS; w++, b1++)
	{
		if(isNull(b1->p))
			continue;
		//
		// All updates of the wavefronts after each light step must be done here!!!!
		//
		// Update quadrature phase
		//
		b1->phi_sin += b1->phi_cos>>SHIFT;
		b1->phi_cos += b1->phi_sin>>SHIFT;
		//
		// Interference footprint decay
		//
		if((b1->t % TAU) == TAU-1)
			b1->xi /= 2;
		//
		// Prepare for the convolution phase
		//
		b1->used = false;
		//
		// Seek upward and downward
		//
		for(offset = 1;; offset++)
		{
			ww = w + offset;
			if(ww >= NPREONS)
				ww -= NPREONS;
			matchPair(b1, dual + ww, offset);
			//
			if(offset == NPREONS/2)
				break;
			ww = w - offset;
			if(ww < 0)
				ww += NPREONS;
			//
			matchPair(b1, dual + ww, offset);
		}
	}
}

/*
 * Detects MPs in this spatial address.
 */
void convolve2(Brick *dual)
{
	Brick *b1, *b2;
	int w1, w2;
	//
	// Circular comparison in the w dimension
	//
	for(b1 = dual, w1 = 0; w1 < NPREONS; w1++, b1++)
	{
		if(isNull(b1->p))
			continue;
		b1->f = 1;
		if(b1->type > 0)
		{
			b2 = dual;
			//
			// MPs have f > 2
			//
			for(w2 = 0; w2 < NPREONS; w2++, b2++)
				if(b1->type == b2->type)
					b1->f++;
		}
	}
}

/*
 * Detects the interaction type (UxU, UxP etc.)
 */
void convolve3(Brick *dual)
{
	// Circular comparison in the w dimension
	//
	Brick *b1;
	int w;
	for(b1 = dual, w = 0; w < NPREONS; w++, b1++)
	{
		// Seed does not interact
		//
		if(isNull(b1->o))
			continue;
		//
		// Seek upward and downward
		//
		int offset, ww;
		for(offset = 1;; offset++)
		{
			ww = w + offset;
			if(ww >= NPREONS)
				ww -= NPREONS;
			if(collision(b1, dual + ww, offset))
			{
				launchBurst();
				break;
			}
			//
			if(offset == NPREONS/2)
				break;
			ww = w - offset;
			if(ww < 0)
				ww += NPREONS;
			//
			if(collision(b1, dual + ww, offset))
			{
				launchBurst();
				break;
			}
		}
	}
}

/*
 * Calculates the voxel color of this xyz position.
 * Calculates the brick status.
 */
char getVoxel(Brick *pri, Brick *dual)
{
	char color = gridcolor;
	Brick *pri4d = pri;
	Brick *dual4d = dual;
	int w;
	for(w = 0; w < NPREONS; w++, pri4d++, dual4d++)
	{
		copyBrick(dual4d, pri4d);
		//
		// Bump cell clock
		//
		dual4d->t++;
		//
		// Calculate voxel color
		//
		if(!isNull(pri4d->seed))
			color = RR;
		else if(pri4d->bcode)
			color = BB;
		else if(pri4d->m)
			color = GG;
		else if(!isNull(pri4d->p))
			color = 7 + pri4d->w;
	}
	return color;
}

/*
 * Deploy burst.
 * Acts on the w dimension.
 */
void expandBurst()
{
//	printf("==========*******=============\n");
	Brick *p = pri, *d = dual;
	for(int w = 0; w < NPREONS; w++, pri++, dual++)
		forkBurst();
	pri = p; dual = d;
}

/*
 * Expands preon wavefront.
 * Acts on the w dimension.
 */
void expandWavefront()
{
	Brick *p = pri, *d = dual;
	for(int w = 0; w < NPREONS; w++, pri++, dual++)
		forkWavefront();
	pri = p; dual = d;
}

/*
 * Executes one cycle of the evolution algorithm.
 */
void cycle()
{
	// Calculate all voxels
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		draft[p3d] = getVoxel(pri, dual);
	//
	// Burst phase
	//
	if(pri0->t < BURST)
	{
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			expandBurst();
	}
	//
	// Convolution in the w dimension
	//
	else if(pri0->t == SYNCH - 1)		// last tick of a time frame?
	{
		// Check if interactions occurred
		//
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				convolve1(dual);
		//
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				convolve2(dual);
		//
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				convolve3(dual);
	}
	else
	{
		// Update preon dynamics
		//
		pri = pri0;	dual = dual0;
		for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
			if(draft[p3d] != gridcolor)
				expandWavefront();
	}
	//
	// Flip lattices
	//
	Brick *xchg = dual0;
	dual0 = pri0;
	pri0 = xchg;
	//
	// Bump time variables
	//
	timer++;
	pri0->t = timer % SYNCH;
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
