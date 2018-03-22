/*
 * scenarios.c
 *
 *  Created on: 16 de out de 2017
 *      Author: Alexandre
 */

#include "scenarios.h"

#include "brick.h"
#include "common.h"
#include "params.h"
#include "tuple.h"
#include "init.h"
#include "utils.h"

static Tuple p7;

char *scenarios[] =
{
		"Burst test",
		"Vacuum demo",
		"Virtual preon",
		"Real preon",
		"UxU interaction",
		"The graviton",
		"Seed test",
		"Electrons demo",
		"Lone pair"
};

int scenario;

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
 * A pure burst test.
 */
void BurstScenario()
{
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, UNDEF, UNDEF, UNDEF, p7, false, UNDEF, true, 0);
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
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	//
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	addPreon(x,y,z,0, -1, UNDEF, UNDEF, p7, false, PREON, false, SYNCH+BURST);
}

void REALScenario()
{
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p7, true, PREON, false, SYNCH+BURST);
}

/*
 * Clash of two Us.
 */
void UXUScenario()
{
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p7, true, PREON, false, 5*SYNCH+BURST);
	//
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(2*SIDE/3,2*SIDE/3,SIDE/2+2,3, -1, UNDEF, UNDEF, p7, true, PREON, false, SYNCH+BURST);
}

void GRAVScenario()
{
	p7.x = 5;
	p7.y = 5;
	p7.z = 5;
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, false, UNDEF, 0, p7, false, GRAV, false, BURST);
	addPreon(SIDE/2,SIDE/2,SIDE/3,1, false, UNDEF, 0, p7, false, PREON, false, BURST);
}

void SEEDScenario()
{
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();

	Brick *t = pri0;
	for(int x = 0; x < SIDE; x++)
		for(int y = 0; y < SIDE; y++)
			for(int z = 0; z < SIDE; z++)
			{
				t->p8 = false;
				t->p23 = 10000;
				t += NPREONS;
			}
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, 0, UNDEF, 0, p7, true, SEED, false, SYNCH);
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, UNDEF, UNDEF, 0, p7, true, SEED, false, 0);
}

void ElectronScenario()
{
	createVacuum();
	for(int i = 0; i < NPREONS/2; i += 2)
	{
		eraseLayer(i);
		p7.x = rndSignal() * rndCoord();
		p7.y = rndSignal() * rndCoord();
		p7.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i, +1, UNDEF, UNDEF, p7, true, PREON, false, SYNCH);
		//
		eraseLayer(i + 1);
		p7.x = rndSignal() * rndCoord();
		p7.y = rndSignal() * rndCoord();
		p7.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i+1, -1, UNDEF, UNDEF, p7, true, PREON, false, SYNCH);
	}
}

void LonePScenario()
{
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, UNDEF, UNDEF, UNDEF, p7, false, PREON, false, SYNCH+BURST);
	//
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,3, UNDEF, UNDEF, UNDEF, p7, false, PREON, false, SYNCH+BURST);
	//
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,0, UNDEF, UNDEF, UNDEF, p7, false, PREON, false, SYNCH+BURST);
	//
	p7.x = rndSignal() * rndCoord();
	p7.y = rndSignal() * rndCoord();
	p7.z = rndSignal() * rndCoord();
	addPreon(SIDE/2,SIDE/3,2,3, UNDEF, UNDEF, UNDEF, p7, false, PREON, false, SYNCH+BURST);
}

