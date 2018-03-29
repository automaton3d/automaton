/*
 * scenarios.c
 */

#include "scenarios.h"

#include "brick.h"
#include "common.h"
#include "params.h"
#include "tuple.h"
#include "init.h"
#include "utils.h"

static Tuple p4;

const char *scenarios[] =
{
		"Burst test",
		"Vacuum demo",
		"Virtual preon",
		"Real preon",
		"UxU interaction",
		"The graviton",
		"Big bang",
		"Lone pairs"
};

int scenario;

/*
 * Erases all bricks in a w address.
 */
static void eraseLayer(int w)
{
	for(int x = 0; x < SIDE; x++)
		for(int y = 0; y < SIDE; y++)
			for(int z = 0; z < SIDE; z++)
			{
				Brick *t = pri0 + (SIDE2 * x + SIDE * y + z) * NPREONS + w;
				cleanBrick(t);
			}
}

/*
 * Tests multiple burst conflict resolution in a common layer.
 */
void BurstScenario()
{
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	Brick *b;
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, DESTROY, BURST);
	b->p15 = b->p0;
	b->p18 = signature(b);
}

/*
 * Pure vacuum scenario. No interactions occur.
 */
void VacuumScenario()
{
	createVacuum();
}

void VIRTScenario()
{
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	addPreon(x,y,z,0, -1, UNDEF, UNDEF, p4, false, PREON, false, SYNCH+BURST);
}

void REALScenario()
{
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH+BURST);
}

/*
 * Clash of two Us.
 */
void UXUScenario()
{
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(rndCoord(),rndCoord(),rndCoord(),0, -1, UNDEF, UNDEF, p4, true, PREON, false, 5*SYNCH+BURST);
//	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p4, true, PREON, false, 5*SYNCH+BURST);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(rndCoord(),rndCoord(),rndCoord(),5, -1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH+BURST);
//	addPreon(2*SIDE/3,2*SIDE/3,SIDE/2+2,5, -1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH+BURST);
}

void GRAVScenario()
{
	p4.x = 5;
	p4.y = 5;
	p4.z = 5;
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, false, UNDEF, 0, p4, false, GRAV, false, BURST);
	addPreon(SIDE/2,SIDE/2,SIDE/3,1, false, UNDEF, 0, p4, false, PREON, false, BURST);
}

void BigBangScenario()
{
	for(int i = 0; i < NPREONS; i += 2)
	{
		eraseLayer(i);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(0,0,0,i, +1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH);
		//
		eraseLayer(i + 1);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(0,0,0,i+1, -1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH);
	}
}

void ElectronsScenario()
{
	createVacuum();
	for(int i = 0; i < NPREONS/2; i += 2)
	{
		eraseLayer(i);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i, +1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH);
		//
		eraseLayer(i + 1);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i+1, -1, UNDEF, UNDEF, p4, true, PREON, false, SYNCH);
	}
}

void LonePScenario()
{
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, false, SYNCH+BURST);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,3, UNDEF, UNDEF, UNDEF, p4, false, PREON, false, SYNCH+BURST);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,0, UNDEF, UNDEF, UNDEF, p4, false, PREON, false, SYNCH+BURST);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,3, UNDEF, UNDEF, UNDEF, p4, false, PREON, false, SYNCH+BURST);
}
