/*
 * scenarios.c
 *
 *  Created on: 16 de out de 2017
 *      Author: Alexandre
 */

#include "scenarios.h"
#include "common.h"
#include "params.h"
#include "tuple.h"
#include "init.h"
#include "tile.h"
#include "utils.h"

static Tuple p6;

void BurstScenario()
{
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, UNDEF, UNDEF, UNDEF, p6, false, UNDEF, true, 0);
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
	p6.x = rndSignal() * rndCoord();
	p6.y = rndSignal() * rndCoord();
	p6.z = rndSignal() * rndCoord();
	//
	int x = rndCoord();
	int y = rndCoord();
	int z = rndCoord();
	addPreon(x,y,z,0, -1, UNDEF, UNDEF, p6, false, PREON, false, SYNCH);
}

void REALScenario()
{
	p6.x = rndSignal() * rndCoord();
	p6.y = rndSignal() * rndCoord();
	p6.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p6, true, PREON, false, SYNCH);
}

/*
 * Clash of two Us.
 */
void UXUScenario()
{
	p6.x = rndSignal() * rndCoord();
	p6.y = rndSignal() * rndCoord();
	p6.z = rndSignal() * rndCoord();
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, -1, UNDEF, UNDEF, p6, false, PREON, false, 5*SYNCH);
	//
	p6.x = rndSignal() * rndCoord();
	p6.y = rndSignal() * rndCoord();
	p6.z = rndSignal() * rndCoord();
	addPreon(2*SIDE/3,2*SIDE/3,SIDE/2+2,3, -1, UNDEF, UNDEF, p6, false, PREON, false, SYNCH);
}

void GRAVScenario()
{
	p6.x = 10;
	p6.x = -3;
	p6.x = 7;
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, true, UNDEF, 0, p6, false, GRAV, false, 0);
}

void SEEDScenario()
{
	p6.x = rndSignal() * rndCoord();
	p6.y = rndSignal() * rndCoord();
	p6.z = rndSignal() * rndCoord();

	Tile *t = pri0;
	for(int x = 0; x < SIDE; x++)
		for(int y = 0; y < SIDE; y++)
			for(int z = 0; z < SIDE; z++)
			{
				t->p7 = false;
				t->p23 = 10000;
				t += NPREONS;
			}
	addPreon(SIDE/3,SIDE/3,SIDE/2,0, 0, UNDEF, 0, p6, true, SEED, false, SYNCH);
	addPreon(SIDE/2,SIDE/2,SIDE/3,0, UNDEF, UNDEF, 0, p6, true, SEED, false, 0);
}

void ElectronScenario()
{
	createVacuum();
	for(int i = 0; i < NPREONS; i++)
	{
		p6.x = rndSignal() * rndCoord();
		p6.y = rndSignal() * rndCoord();
		p6.z = rndSignal() * rndCoord();
		addPreon(rndCoord(),rndCoord(),rndCoord(),i, -1, UNDEF, UNDEF, p6, true, PREON, false, SYNCH);
	}
}
