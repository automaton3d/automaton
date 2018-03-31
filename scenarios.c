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
		"Single unpaired preon",
		"Vacuum demo",
		"No UxU interaction",
		"UxU interaction",
		"Gravitons",
		"Lone pairs",
		"Big bang",
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
	showAxes = false;
	showGrid = true;
	showBox = false;
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	Brick *b;
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, +1, LEPT, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, +1, LEPT, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, +1, LEPT, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	b = addPreon(x,y,z,0, p4, +1, +1, +1, +1, +1, LEPT, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
}

/*
 * Pure vacuum scenario. Only PxP interactions may occur (vacuum fluctuations).
 */
void VacuumScenario()
{
	int p8 = +1;
	resetTuple(&p4);
	for(int w = 0; w < NPREONS; w += 2)
	{
		int x = rndCoord();
		int y = rndCoord();
		int z = rndCoord();
		addPreon(x,y,z,w, p4, -1, +1, -1, p8, LEPT, PREON, BURST, UNDEF);
		addPreon(x,y,z,w+1, p4, +1, -1, -1, p8, ANTILEPT, PREON, BURST, UNDEF);
		p8 *= -1;
	}
}

/*
 * Shows a single unpaired preon.
 */
void UScenario()
{
	showGrid = true;
	showBox = false;
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, p4, -1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * Shows a couple of preons that do not interact because
 * one is real while the other is virtual.
 */
void NoUXUScenario()
{
	showGrid = true;
	showBox = false;
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	addPreon(x,y,z,0, p4, -1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
	x = rndCoord();
	y = rndCoord();
	z = rndCoord();
	addPreon(x,y,z,3, p4, -1, -1, -1, -1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * Clash of two real Us.
 * Both are reissued at the contact point.
 */
void UXUScenario()
{
	showGrid = true;
	showBox = false;
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(rndCoord(),rndCoord(),rndCoord(),0, p4, -1, -1, -1, -1, LEPT, PREON, 5*SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(rndCoord(),rndCoord(),rndCoord(),5, p4, -1, -1, -1, -1, LEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * Shows a couple of opposite gravitons.
 */
void GRAVScenario()
{
	showGrid = true;
	p4.x = 5;
	p4.y = 5;
	p4.z = 5;
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
	p4.x = -5;
	p4.y = -5;
	p4.z = -5;
	addPreon(SIDE/2,SIDE/2,SIDE/3,1, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
}

/*
 * Charged preons interacting.
 */
void ElectronsScenario()
{
	VacuumScenario();
	for(int i = 0; i < NPREONS/2; i += 2)
	{
		eraseLayer(i);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i, p4, +1, -1, -1, +1, LEPT, PREON, SYNCH, UNDEF);
		//
		eraseLayer(i + 1);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i+1, p4, -1, +1, +1, +1, ANTILEPT, PREON, SYNCH, UNDEF);
	}
}

/*
 * PxP interaction demo.
 */
void LonePScenario()
{
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, p4, -1, +1, -1, +1, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,2, p4, +1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,4, p4, -1, +1, -1, +1, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,6, p4, +1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * The universe.
 */
void BigBangScenario()
{
	resetTuple(&p4);
	for(int i = 0; i < NPREONS; i ++)
		addPreon(SIDE/2,SIDE/2,SIDE/2,i, p4, (i & HEL)?+1:-1, (i & ELT)?+1:-1, (i & CHI)?+1:-1, (i & GRV)?+1:-1, (i & CLR)?LEPT:ANTILEPT, PREON, SYNCH, UNDEF);
}
