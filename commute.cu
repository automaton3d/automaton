#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "automaton.cuh"

/*
 * Reverses roles. 
 */
__global__ void commute(Cell* lattice)
{
	long xyz = blockDim.x * blockIdx.x + threadIdx.x;
	if (xyz < SIDE3)
	{
		// Calculate pointers
		//
		Cell* draft = lattice + xyz;
		Cell* stable = draft + SIDE2 * SIDE3;
		if (draft->active)
		{
			Cell* temp = draft;
			draft = stable;
			stable = temp;
		}
		//
		// Scan an entire column
		//
		for (int v = 0; v < SIDE2; v++)
		{
			// On the last tick, disassemble all pairs
			//
			if (draft->t % LIGHT == 0 && draft->f > 0)
				draft->f = 1;
			//
			// Copy all variables
			//
			stable->t = draft->t;
			stable->dir = draft->dir;
			stable->charge = draft->charge;
			stable->code = draft->code;
			stable->noise = draft->noise;
			stable->b = draft->b;
			stable->synch = draft->synch;
			stable->f = draft->f;
			COPY(stable->p, draft->p);
			COPY(stable->s, draft->s);
			COPY(stable->o, draft->o);
			stable->flash = draft->flash;



			if (v == 173 && stable->t > 61 && stable->f > 0 && MOD2(stable->o) < 2)
				printf("mod2(o)=%d\n", stable->t);
			//
			// Commute roles
			//
			stable->active = false;
			draft->active = true;
			//
			// Next register
			//
			draft = nextV(draft);
			stable = nextV(stable);
		}
	}
}

