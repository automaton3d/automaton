/*
 * scenarios.c
 */

#include "scenarios.h"

#include <limits.h>
#include "brick.h"
#include "common.h"
#include "params.h"
#include "tuple.h"
#include "init.h"
#include "utils.h"

static Tuple p0, p3, p4;

const char *sceneNames[] =
{
		"Burst test",
		"Single unpaired preon",
		"Gravitons",
		"Lone pairs",
		"Vacuum demo",
		"No UxU interaction",
		"UxU interaction",
		"Inertia demo",
		"EM boson",
		"Coulomb force",
		"Magnetic force",
		"Weak force",
		"Strong force",
		"Electrons",
		"Annihilation",
		"Virtual decay of P",
		"Gravity acceleration",
		"KNP cancellation",
		"Alignment",
		"Big bang",
};

int scene = -1;
static Brick *b;

//unsigned long elt_case = ULONG_MAX;
//unsigned long magn_case = ULONG_MAX;
//unsigned long em_case = ULONG_MAX;

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
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	b = addPreon(p0,0, p3, p4, +1, +1, +1, +1, LEPT, PREON, BURST, DESTROY);
	b->p15 = b->p0;
	b->p18 = signature(b);
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
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, ANTILEPT, PREON, BURST, UNDEF);
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
	addPreon(p0,0, p3, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
	p4.x = -5;
	p4.y = -5;
	p4.z = -5;
	addPreon(p0,1, p3, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
}

/*
 * PxP interaction demo.
 */
void LonePScenario()
{
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = SIDE/2;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, +1, -1, +1, LEPT, PREON, BURST, UNDEF);
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	addPreon(p0,2, p3, p4, +1, -1, -1, +1, ANTILEPT, PREON, BURST, UNDEF);
	//
	p0.x = SIDE/2;
	p0.y = SIDE/3;
	p0.z = 2;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	addPreon(p0,4, p3, p4, -1, +1, -1, +1, LEPT, PREON, BURST, UNDEF);
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	addPreon(p0,6, p3, p4, +1, -1, -1, +1, ANTILEPT, PREON, BURST, UNDEF);
}

/*
 * Pure vacuum scenario. Only PxP interactions may occur (vacuum fluctuations).
 */
void VacuumScenario()
{
	int p8 = +1;
	resetTuple(&p3);
	resetTuple(&p4);
	for(int w = 0; w < NPREONS; w += 2)
	{
		p0.x = rndCoord();
		p0.y = rndCoord();
		p0.z = rndCoord();
		addPreon(p0,w, p3, p4, -1, +1, -1, p8, LEPT, PREON, BURST, UNDEF);
		addPreon(p0,w+1, p3, p4, +1, -1, -1, p8, ANTILEPT, PREON, BURST, UNDEF);
		p8 *= -1;
	}
}

/*
 * Shows a couple of preons that do not interact because
 * one is real while the other is virtual.
 */
void NoUXUScenario()
{
	showGrid = true;
	showBox = false;
	//
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	resetTuple(&p3);
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, ANTILEPT, PREON, BURST, UNDEF);
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,3, p3, p4, -1, -1, -1, -1, ANTILEPT, PREON, BURST, UNDEF);
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
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, -1, LEPT, PREON, 4*SYNCH+BURST, UNDEF);
	//
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,5, p3, p4, -1, -1, -1, -1, LEPT, PREON, BURST, UNDEF);
}

void InertiaScenario()
{
	// Create a U
	//
	p0.x = 4;
	p0.y = 5;
	p0.z = 6;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, LEPT, PREON, 3*SYNCH+BURST, UNDEF);
	//
	// Create a P
	//
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/3;
	//
	p3.x = 1;
	p3.y = 1;
	p3.z = 1;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	b = addPreon(p0,5, p3, p4, -1, +1, -1, +1, LEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	invertTuple(&p4);
	b = addPreon(p0,6, p3, p4, -1, -1, +1, +1, ANTILEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
}

void EMScenario()
{
	// Create emitter U
	//
	p0.x = 4;
	p0.y = 5;
	p0.z = 6;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	// Create gluing U
	//
	p0.x = 4;
	p0.y = 5;
	p0.z = 5*SIDE/8;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,3, p3, p4, -1, -1, -1, +1, LEPT, PREON, 3*SYNCH+BURST, UNDEF);
	//
	// Create a vacuum P
	//
	p0.x = 3*SIDE/8;
	p0.y = 3*SIDE/8;
	p0.z = SIDE/3;
	//
	resetTuple(&p4);
	b = addPreon(p0,5, p3, p4, -1, +1, -1, -1, LEPT, PREON, BURST, UNDEF);
	b = addPreon(p0,6, p3, p4, -1, -1, +1, -1, ANTILEPT, PREON, BURST, UNDEF);
//	em_case = 509;
}

void CoulombScenario()
{
	// Create harvesting U
	//
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/3;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, LEPT, PREON, 4*SYNCH+BURST, UNDEF);
	//
	// Create gluing U
	//
	p0.x = 2*SIDE/3;
	p0.y = SIDE/3;
	p0.z = 3;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,1, p3, p4, -1, -1, -1, +1, LEPT, PREON, 8*SYNCH+BURST, UNDEF);
	//
	// Create remote U
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = 7*SIDE/8;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p3, p4, -1, -1, -1, +1, LEPT, PREON, 10*SYNCH+BURST, UNDEF);
	//
	// Create vacuum P
	//
	p0.x = SIDE/4;
	p0.y = SIDE/2;
	p0.z = 2*SIDE/3;
	//
	resetTuple(&p4);
	b = addPreon(p0,5, p3, p4, -1, +1, -1, -1, LEPT, PREON, BURST, UNDEF);
	b = addPreon(p0,6, p3, p4, -1, -1, +1, -1, ANTILEPT, PREON, BURST, UNDEF);
}

void MagneticScenario()
{
	// Create harvesting U
	//
	p0.x = SIDE/2;
	p0.y = SIDE/2;
	p0.z = SIDE/3;
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, -1, -1, -1, +1, LEPT, PREON, 4*SYNCH+BURST, UNDEF);
	//
	// Create gluing U
	//
	p0.x = 2*SIDE/3;
	p0.y = SIDE/3;
	p0.z = 3;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,1, p3, p4, -1, -1, -1, +1, LEPT, PREON, 8*SYNCH+BURST, UNDEF);
	//
	// Create remote U
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = 7*SIDE/8;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p3, p4, -1, -1, -1, +1, LEPT, PREON, 10*SYNCH+BURST, UNDEF);
	//
	// Create vacuum P
	//
	p0.x = SIDE/4;
	p0.y = SIDE/2;
	p0.z = 2*SIDE/3;
	//
	resetTuple(&p4);
	b = addPreon(p0,5, p3, p4, -1, +1, -1, -1, LEPT, PREON, BURST, UNDEF);
	b = addPreon(p0,6, p3, p4, -1, -1, +1, -1, ANTILEPT, PREON, BURST, UNDEF);
}

void WeakScenario()
{
	// Create U
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = 7*SIDE/8;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p3, p4, -1, -1, -1, REAL, LEPT, PREON, SYNCH+BURST, UNDEF);
	//
	// Create P
	//
	p0.x = SIDE/4;
	p0.y = SIDE/2;
	p0.z = 2*SIDE/3;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	b = addPreon(p0,5, p3, p4, -1, +1, -1, REAL, LEPT, PREON, BURST, UNDEF);
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,6, p3, p4, -1, -1, +1, REAL, ANTILEPT, PREON, BURST, UNDEF);
}

void StrongScenario()
{
	// Create U
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = 7*SIDE/8;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p3, p4, -1, -1, -1, REAL, 0x20, PREON, SYNCH+BURST, UNDEF);
	//
	// Create P
	//
	p0.x = SIDE/4;
	p0.y = SIDE/2;
	p0.z = 2*SIDE/3;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	b = addPreon(p0,5, p3, p4, -1, +1, -1, REAL, 0x20, PREON, BURST, UNDEF);
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,6, p3, p4, -1, -1, +1, REAL, 0x1f, PREON, BURST, UNDEF);
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
		resetTuple(&p3);
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(p0,i, p3, p4, +1, -1, -1, +1, LEPT, PREON, BURST, UNDEF);
		//
		eraseLayer(i + 1);
		p0.x = rndCoord();
		p0.y = rndCoord();
		p0.z = rndCoord();
		//
		p4.x = rndSignal() * rndCoord();
		p4.y = rndSignal() * rndCoord();
		p4.z = rndSignal() * rndCoord();
		addPreon(p0,i+1, p3, p4, -1, +1, +1, +1, ANTILEPT, PREON, BURST, UNDEF);
	}
}

void AnnihilScenario()
{
	showGrid = true;
	showBox = false;
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,0, p3, p4, +1, -1, +1, +1, LEPT, PREON, 4*SYNCH+BURST, UNDEF);
	//
	p0.x = rndCoord();
	p0.y = rndCoord();
	p0.z = rndCoord();
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,5, p3, p4, +1, +1, -1, +1, ANTILEPT, PREON, BURST, UNDEF);
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
	resetTuple(&p3);
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	Brick *b;
	b = addPreon(p0,0, p3, p4, +1, +1, -1, VIRTUAL, LEPT, PREON, BURST, UNDEF);
	b->p17 = SIDE;
	//
	invertTuple(&p4);
	//
	b = addPreon(p0,4, p3, p4, +1, -1, +1, VIRTUAL, ANTILEPT, PREON, BURST, UNDEF);
	b->p17 = SIDE;
}

/*
 * Gravity acceleration test.
 */
void GravityScenario()
{
	showGrid = true;
	//
	// Create a graviton
	//
	p0.x = 2;
	p0.y = 2;
	p0.z = 3;
	//
	p4.x = 5;
	p4.y = 5;
	p4.z = 5;
	addPreon(p0,0, p3, p4, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, GRAV, BURST, UNDEF);
	//
	// Create an interacting U
	//
	p0.x = 2*SIDE/3;
	p0.y = 2*SIDE/3;
	p0.z = SIDE/2+1;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	addPreon(p0,2, p3, p4, -1, -1, -1, REAL, ANTILEPT, PREON, BURST, UNDEF);
	//
	// Create a vacuum P
	//
	p0.x = SIDE/6;
	p0.y = SIDE/6;
	p0.z = SIDE/3;
	//
	resetTuple(&p4);
	b = addPreon(p0,5, p3, p4, -1, +1, -1, VIRTUAL, LEPT, PREON, BURST+5*SYNCH, UNDEF);
	b = addPreon(p0,6, p3, p4, -1, -1, +1, VIRTUAL, ANTILEPT, PREON, BURST+5*SYNCH, UNDEF);
}

/*
 * KNP cancellation.
 */
void CancelScenario()
{
	// Create 1rst KNP
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = SIDE/2;
	//
	p3.x = 1;
	p3.y = 0;
	p3.z = 0;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	b = addPreon(p0,0, p3, p4, -1, +1, -1, REAL, LEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,2, p3, p4, +1, -1, +1, REAL, ANTILEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	// Create 2nd KNP
	//
	p0.x = SIDE/2;
	p0.y = SIDE/3;
	p0.z = 2;
	//
	p3.x = -1;
	p3.y = 0;
	p3.z = 0;
	//
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	b = addPreon(p0,4, p3, p4, -1, +1, -1, REAL, LEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,6, p3, p4, +1, -1, +1, REAL, ANTILEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
}

/*
 * Alignment.
 */
void AlignmentlScenario()
{
	// Create 1rst KNP
	//
	p0.x = SIDE/3;
	p0.y = SIDE/3;
	p0.z = SIDE/2;
	//
	p3.x = 1;
	p3.y = 1;
	p3.z = 1;
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	b = addPreon(p0,0, p3, p4, -1, +1, -1, REAL, LEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,2, p3, p4, +1, -1, +1, REAL, ANTILEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	// Create 2nd KNP
	//
	p0.x = SIDE/2;
	p0.y = SIDE/3;
	p0.z = 2;
	//
	p3.x = 1;
	p3.y = 1;
	p3.z = 1;
	//
	//
	p4.x = rndSignal() * rndCoord();
	p4.y = rndSignal() * rndCoord();
	p4.z = rndSignal() * rndCoord();
	//
	b = addPreon(p0,4, p3, p4, -1, +1, -1, REAL, LEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
	//
	p4.x *= -1;
	p4.y *= -1;
	p4.z *= -1;
	b = addPreon(p0,6, p3, p4, +1, -1, +1, REAL, ANTILEPT, PREON, BURST, UNDEF);
	b->p16 = BOUND;
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
	resetTuple(&p3);
	resetTuple(&p4);
	for(int i = 0; i < NPREONS; i ++)
		addPreon(p0,i, p3, p4, (i & HEL)?+1:-1, (i & ELT)?+1:-1, (i & CHI)?+1:-1, (i & GRV)?+1:-1, (i & CLR)?LEPT:ANTILEPT, PREON, BURST, UNDEF);
}

//
// Scenario selection by the user interface uses these function pointers.
//
void (*scenarios[])() =
{
	BurstScenario,
	UScenario,
	GRAVScenario,
	LonePScenario,
	VacuumScenario,
	NoUXUScenario,
	UXUScenario,
	InertiaScenario,
	EMScenario,
	CoulombScenario,
	MagneticScenario,
	WeakScenario,
	StrongScenario,
	ElectronsScenario,
	AnnihilScenario,
	VirtualDecayScenario,
	GravityScenario,
	CancelScenario,
	AlignmentlScenario,
	BigBangScenario,
};

