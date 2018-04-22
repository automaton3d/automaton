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

static Tuple p0;
static Tuple p4;

const char *scenarios[] =
{
	"Burst test",
	"Single unpaired preon",
	"Vacuum demo",
	"No UxU interaction",
	"UxU interaction",
	"Gravitons",
	"Annihilation",
	"Big bang",
	"Virtual decay of P",
	"Lone pairs",
};

int scenario = -1;

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
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	Brick *b;
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
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
		p0.x = rndCoord();
		p0.y = rndCoord();
		p0.z = rndCoord();
		addPreon(p0,w, p4, -1, +1, -1, p8, LEPT, PREON, BURST, UNDEF);
		addPreon(p0,w+1, p4, +1, -1, -1, p8, ANTILEPT, PREON, BURST, UNDEF);
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
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = SIDE/2;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p4, -1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
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
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	addPreon(p0,0, p4, -1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	addPreon(p0,3, p4, -1, -1, -1, -1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * Clash of two real Us.
 * Both are reissued at the contact point.
 */
void UXUScenario()
{
	showGrid = true;
	showBox = false;
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p4, -1, -1, -1, -1, LEPT, PREON, 5*SYNCH+BURST, UNDEF);
	//
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,5, p4, -1, -1, -1, -1, LEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * Shows a couple of opposite gravitons.
 */
void GRAVScenario()
{
	showGrid = true;
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/3;
	//
	p4.x = 5;
	p4.y = 5;
	p4.z = 5;
	addPreon(p0,0, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
	p4.x = -5;
	p4.y = -5;
	p4.z = -5;
	addPreon(p0,1, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
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
		p0.x = rndCoord();
		p0.y = rndCoord();
		p0.z = rndCoord();
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(p0,i, p4, +1, -1, -1, +1, LEPT, PREON, SYNCH, UNDEF);
		//
		eraseLayer(i + 1);
		p0.x = rndCoord();
		p0.y = rndCoord();
		p0.z = rndCoord();
		//
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(p0,i+1, p4, -1, +1, +1, +1, ANTILEPT, PREON, SYNCH, UNDEF);
	}
}

/*
 * PxP interaction demo.
 */
void LonePScenario()
{
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = SIDE/2;
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p4, -1, +1, -1, +1, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p4, +1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p0.x = SIDE/2;
	p0.y = SIDE/3;
	p0.z = 2;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,4, p4, -1, +1, -1, +1, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,6, p4, +1, -1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}

/*
 * The universe.
 */
void BigBangScenario()
{
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/2;
	//
	resetTuple(&p4);
	for(int i = 0; i < NPREONS; i ++)
		addPreon(p0,i, p4, (i & HEL)?+1:-1, (i & ELT)?+1:-1, (i & CHI)?+1:-1, (i & GRV)?+1:-1, (i & CLR)?LEPT:ANTILEPT, PREON, SYNCH, UNDEF);
}

/*
 * Test of axiom 5.
 */
void VirtualDecayScenario()
{
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/2;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	Brick *b;
	b = addPreon(p0,0, p4, +1, -1, -1, NOGRAV, LEPT, PREON, SYNCH+BURST, UNDEF);
	b->p17 = SIDE;
	//
	p4.x -= 1;
	p4.y -= 1;
	p4.z -= 1;
	//
//	b = addPreon(p0,4, p4, -1, -1, +1, NOGRAV, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
	b = addPreon(p0,4, p4, +1, -1, -1, NOGRAV, LEPT, PREON, SYNCH+BURST, UNDEF);
	b->p17 = SIDE;
}

void AnnihilScenario()
{
	showGrid = true;
	showBox = false;
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p4, +1, -1, +1, +1, LEPT, PREON, 5*SYNCH+BURST, UNDEF);
	//
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,5, p4, +1, +1, -1, +1, ANTILEPT, PREON, SYNCH+BURST, UNDEF);
}
