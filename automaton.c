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

boolean interaction(Tile *dual4d)
{
	if(dual4d->p12 == UNDEF || dual4d->p1 < BURST)
		return false;
	//
	switch(dual4d->p12)
	{
		case UXU:
			printf("UXU %d\n", dual4d->p1);
			dual4d->p14 = dual4d->p0;
			break;
		case UXP:
			puts("UXP");
			break;
		case PXP:
			puts("PXP");
			break;
		default:
			return false;
	}
	dual4d->p1 %= SYNCH;
	//
	// Launch burst
	//
	dual4d->p19 = true;
	resetTuple(&dual4d->p25);
	//
	// Real preon?
	//
	if(dual4d->p7)
	{
		// Launch graviton
		//
		dual->p17 |= GRAV;
		tupleCross(dual4d->p6, dual4d->p2, &dual4d->p6);
		normalizeTuple(&dual4d->p6);
		resetTuple(&dual4d->p2);
	}
	dual4d->p23 = dual4d->p1 + SYNCH;
	dual4d->p12 = UNDEF;
	//
	return true;
}

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
		resetDFO(dual);
	}
	else
	{
		cleanTile(dual);
	}
	dual->p19 = false;
}

void expandPreon()
{
	if((pri->p17 & (PREON | SEED)) == 0 || pri->p1 <= pri->p23 || pri->p1 < BURST)
		return;
	//
	// Tree expansion
	//
	for(int dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p2, pri->p22))
		{
			Tile *nual = getNual(dir);
			copyTile(nual, dual);
			nual->p23 = nual->p1 + SYNCH * modTuple(&nual->p2) + 0.5;
			nual->p22 = dir;
			addTuples(&nual->p2, dirs[dir]);
			nual->p17 = PREON;
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

void pairClassification(Tile *dual, Tile *nual)
{
	unsigned char color = dual->p5 ^ nual->p5;
	//
	// VCP detection
	//
	if(isNull(dual->p6) && isNull(nual->p6) && dual->p8 == UNDEF && nual->p8 == UNDEF)
	{
		dual->p18 = VCP;
		return;
	}
	//
	// NTP detection
	//
	else if(dual->p4 == nual->p4 && dual->p4 != UNDEF && isNull(dual->p6) && isNull(nual->p6))
	{
		dual->p18 = NTP;
		return;
	}
	// GLP detection
	//
	else if(color == 0x22 || color == 0x14 || color == 0x0c || color == 0x21 || color == 0x11 || color == 0x0a)
	{
		dual->p18 = GLP;
		return;
	}
	//
	// MSP detection
	//
	else if(color == 0x24 || color == 0x12 || color == 0x09)
	{
		dual->p18 = MSP;
		return;
	}
	//
	// PHP detection
	//
	else if(dual->p3 == -nual->p3 && color == 0xff && isOpposite(dual->p6, nual->p6))
	{
		dual->p18 = PHP;
		return;
	}
	//
	// EMP detection
	//
	else if(!dual->p7)
	{
		dual->p18 = EMP;
		return;
	}
}

void classify10(Tile *pri, Tile *dual)
{
	int np = 0, nuxu = 0;
	Tile *dual4d = dual;
	for(int w = 0; w < NPREONS; w++, dual4d++)
	{
		if(dual4d->p17 == UNDEF || dual4d->p19 || isNull(dual4d->p2))
		{
			dual4d->p12 = UNDEF;
			continue;
		}
	//	if(isEqual(dual4d->p2, n->p2) && dual4d->p3 == -n->p3)
	}
}

/*
 * Result: U, P, UXU
 */
void classify1(Tile *pri, Tile *dual)
{
	Tile *dual4d = dual;
	for(int w = 0; w < NPREONS; w++, dual4d++)
	{
		if(dual4d->p17 == UNDEF || dual4d->p19 || isNull(dual4d->p2))
		{
			dual4d->p12 = UNDEF;
			continue;
		}
		//
		dual4d->p12 = U;
		Tile *n;
		n = dual4d + 1;
		for(int i = dual4d->p16+1; i < NPREONS; i++, n++)
		{
			if(isEqual(dual4d->p2, n->p2) && dual4d->p3 == -n->p3)
			{
				dual4d->p12 = P;
				pairClassification(dual4d, n);
				break;
			}
			else if(n->p12 != UXU && n->p17 == PREON && !isEqual(dual4d->p2, n->p2))
			{
				dual4d->p12 = UXU;
				dual4d->p14 = dual4d->p0;
				subRectify(&dual4d->p14, dual4d->p2);
				addRectify(&dual4d->p14, getDirection(dual4d->p14, n->p0));
				break;
			}
		}
		n = dual4d - 1;
		for(int i = dual4d->p16-1; i >= 0; i--, n--)
		{
			if(isEqual(dual4d->p2, n->p2) && dual4d->p3 == -n->p3)
			{
				dual4d->p12 = P;
				pairClassification(dual4d, n);
				break;
			}
			else if(n->p12 != UXU && n->p17 == PREON && !isEqual(dual4d->p2, n->p2))
			{
				dual4d->p12 = UXU;
				dual4d->p14 = dual4d->p0;
				subRectify(&dual4d->p14, dual4d->p2);
				addRectify(&dual4d->p14, getDirection(dual4d->p14, n->p0));
				break;
			}
		}
	}
}

/*
 * UxP
 */
void classify2(Tile *pri, Tile *dual)
{
	Tile *dual4d = dual;
	for(int w = 0; w < NPREONS; w++, dual4d++)
	{
		if(dual4d->p17 == UNDEF || dual4d->p12 == UNDEF || dual4d->p12 == UXU)
			continue;
		//
		Tile *n;
		n = dual4d + 1;
		for(int i = dual4d->p16+1; i < NPREONS; i++, n++)
		{
			if((dual4d->p12 == P && (n->p12 == U || n->p12 == U)) ||
			   (dual4d->p12 == U && (n->p12 == P || n->p12 == P)))
			{
				dual4d->p12 = UXP;
				break;
			}
		}
		n = dual4d - 1;
		for(int i = dual4d->p16-1; i >= 0; i--, n--)
		{
			if((dual4d->p12 == P && (n->p12 == U || n->p12 == U)) ||
			   (dual4d->p12 == U && (n->p12 == P || n->p12 == P)))
			{
				dual4d->p12 = UXP;
				break;
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
			classify1(pri, dual);
	//
	pri = pri0;	dual = dual0;
	for(int p3d = 0; p3d < SIDE3; p3d++, pri+=NPREONS, dual+=NPREONS)
		if(draft[p3d] != gridcolor)
			classify2(pri, dual);
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

//////////////////////////

/*
 * Called from patterns1.
 *
 * return true, if the cell has not been visited
 * return false, if activation time (AT)
 */
boolean interference()
{
	if(pri->p1 <= pri->p23)
	{
		// Interference I
		// The cell is not visited, so, at each light step
		//
//		dual->p15++;
		//
		// p14 decays absolutely and exponentially
		// (Sciarretta)
		//
		if(pri->p12 > 0)
		{
			dual->p12 *= (SIDE - SIDE / (2 * dual->p13));
			if(dual->p12 < 0)
				dual->p12 = 0;
		}
		else if(dual->p12 < 0)
		{
			dual->p12 *= (SIDE + SIDE / (2 * dual->p13));
			if(dual->p12 > 0)
				dual->p12 = 0;
		}
		return true;
	}
	else if(pri->p19 == UNDEF)
	{
		dual->p13 = 0;
		dual->p12 += pri->p21a2;
	}
	return false;
}

