/*
 *  Created on: 21/09/2016
 *      Author: Alexandre
 */

#include "automaton.h"

// The lattice

Tile *pri0, *dual0;
Tile *pri, *dual;
Tuple dirs[6];

void reemit(){}	// dumb

/*
 * Gets the neighbor's dual.
 */
Tile *getNual(int dir)
{
	Tuple pos;
	tupleCopy(&pos, dual->p0);
	addRectify(&pos, dirs[dir]);
	return dual0 + (SIDE3 * pos.x +  + SIDE2 * pos.y + SIDE * pos.z + dual->p5);
}

int checkPath(int dir)
{
	double d1 = mod2Tuple(&pri->p6);
	int x = pri->p6.x + dirs[dir].x;
	int y = pri->p6.y + dirs[dir].y;
	int z = pri->p6.z + dirs[dir].z;
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

boolean interaction()
{
	Tuple p;
	unsigned p1;
	switch(ORDER)
	{
		case 3:
			p.x = 2; p.y=4; p.z=2;
			p1 = 52;
			break;
		case 4:
//			p.x = 5; p.y=8; p.z=5;
	//		p1 = 192;
			p.x = 3; p.y=8; p.z=5;
			p1 = 360;
			break;
		case 5:
			p.x = 6; p.y=16; p.z=10;
			p1 = 1596;
			break;
		case 6:
			p.x = 53; p.y=32; p.z=21;
			p1 = 0;
			break;
	}
	if(isEqual(pri->p0, p) && pri->p1 == p1)
	{
		// Launch burst
		//
		dual->p1 = pri->p1 % SYNCH;
		dual->p3 = PLAIN;
		tupleCopy(&dual->p19, pri->p0);
		subRectify(&dual->p19, pri->p6);
		resetTuple(&dual->p26);
		return true;
	}
	return false;
}

boolean xinteraction()
{
	if((int)modTuple(&pri->p6) == 3 && pri->p1 % SYNCH >= BURST)
	{
		printf("p1=%d p0=%s mod=%d B=%d\n", pri->p1, tuple2str(&pri->p0), pri->p1%BURST, BURST);
		stop = true;
	}
	return false;
}

int n;

void expandGraviton()
{
	if(pri->p2 == UNDEF || pri->p1 % SYNCH < BURST)
		return;
	if(pri->p1_ < 1000000)
	{
		dual->p1 = dual->p1_;
		dual->p1_ = 1000000;
	}
	if(dual->p1 <= dual->p203)
		return;
	//
	unsigned char dir = 6;
	int i;
	Vector3d v1, v2;
	v1.x = pri->p12.x;
	v1.y = pri->p12.y;
	v1.z = pri->p12.z;
	norm3d(&v1);
	double best = 0;
	for(i = 0; i < NDIR; i++)
	{
		int path = checkPath(i);
		if(path == 2)
		{
			v2.x = pri->p6.x + dirs[i].x;
			v2.y = pri->p6.y + dirs[i].y;
			v2.z = pri->p6.z + dirs[i].z;
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
		addTuples(&nual->p6, dirs[dir]);
		nual->p18 = true;
		nual->p203 = SYNCH * modTuple(&nual->p6) + 0.5;
	}
	cleanTile(dual);
}

void expandBurst()
{
	if(pri->p1 == SYNCH)
	{
		dual->p1 = 0;
		if(pri->p2 == REAL)
		{
			dual->p1_ = 1000000;
			dual->p2 = GRAV;
		}
	}
	else if(pri->p1 >= BURST)
	{
		return;					// p1 must complete one full SYNCH
	}
	//
	int dir;
	for(dir = 0; dir < NDIR; dir++)
	{
		if(isAllowed(dir, pri->p26, pri->p260))
		{
			Tile *nual = getNual(dir);
			//
			boolean gr = nual->p2 == GRAV;
			int p1 = nual->p1;
			Tuple p6;
			tupleCopy(&p6, nual->p6);
			//
			copyTile(nual, dual);
			if(dual->p2 == GRAV)		// patch
				nual->p2 = REAL;		// must transport to OP
			nual->p1_ = 1000000;
			if(gr)
			{
				nual->p2 = GRAV;
				nual->p1_ = p1;
				tupleCopy(&nual->p6, p6);
			}
			nual->p260 = dir;
			addTuples(&nual->p26, dirs[dir]);
			n++;
		}
	}
	dual->p3 = UNDEF;
	if(isEqual(pri->p19, pri->p0))
	{
		// Reemit
		//
		dual->p25 = BURST;
	}
	else if(dual->p1 == 0 && dual->p2 == GRAV)
	{
		tupleCross(pri->p12, pri->p6, &dual->p12);
		normalizeTuple(&dual->p12);
		resetTuple(&dual->p6);
		dual->p203 = BURST;
	}
	else if(pri->p2 != GRAV)
	{
		cleanTile(dual);
	}
}

void expandPreon()
{
	if(pri->p2 == UNDEF || pri->p1 % SYNCH < BURST)
		return;
	if(pri->p25 == BURST)
	{
		resetDFO(dual);
		dual->p1 = pri->p1 % SYNCH;
		dual->p3 = UNDEF;
		resetTuple(&dual->p6);
		dual->p203 = pri->p1 == BURST ? 0 : SYNCH + BURST - dual->p1;
		dual->p25 = 0;
		return;
	}
	//
	// Synchronize wavefront
	//
	if(pri->p1 <= pri->p203)
		return;
	//
	// Test if interacted
	//
	if(interaction())
		return;
	//
	// Tree expansion
	//
	int dir;
	for(dir = 0; dir < NDIR; dir++)
	{
		Tile *nual = getNual(dir);
		if(isAllowed(dir, pri->p6, pri->p200))
		{
			copyTile(nual, dual);
			nual->p203 = SYNCH * modTuple(&nual->p6) + 0.5;
			nual->p200 = dir;
			addTuples(&nual->p6, dirs[dir]);
			if(dir % 2 == 0)
				nual->p25++;
		}
	}
	cleanTile(dual);
}

/*
 * Executes one cycle of the automaton program.
 */
void cycle()
{
	int pos;
	pri = pri0;
	dual = dual0;
	for(pos = 0; pos < SIDE4; pos++, pri++, dual++)
	{
		copyTile(dual, pri);
		dual->p1++;		// increment the cell clock
	}
//		pairClassification();
	//
	/*
	pri = pri0;
	dual = dual0;
	for(pos = 0; pos < SIDE4; pos++, pri++, dual++)
		patterns2();
	*/
	/*
	//
	flipLattices();
	for(pos = 0; pos < SIDE4; pos++, pri++, dual++)
		patterns3();

	 */
	//
	pri = pri0;
	dual = dual0;
	for(pos = 0; pos < SIDE4; pos++, pri++, dual++)
		if(pri->p3 != UNDEF)
			expandBurst();
		else if(pri->p2 == GRAV)
			expandGraviton();
		else
			expandPreon();
	//
	// Flip lattices
	//
	Tile *xchg= dual0;
	dual0 = pri0;
	pri0 = xchg;
	//
	timer++;	//delay(80);
}

/*
 * The automaton thread.
 */
void *AutomatonLoop()
{
	init();
	while(true)
	{
		if(stop && timer % 2 == 0)
			delay(200);
		else
			cycle();
	}
	return NULL;
}
