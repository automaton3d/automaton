#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include <assert.h>
#include "automaton.cuh"

/*
 * Lets the two columns equal. 
 */
__global__ void replicate(Cell* lattice)
{
	long xyz = blockDim.x * blockIdx.x + threadIdx.x;
	if (xyz < SIDE3)
	{
		// Calculate pointers
		//
		Cell* draft = lattice + xyz;
		Cell* stable = draft + SIDE2 * SIDE3;
		//
		// Not last tick?
		//
		if (draft->t % LIGHT != 0)
			return;
		//
		// Scan the two columns
		//
		for (int v = 0; v < SIDE2; v++)
		{
			// Copy only variables that changed in compare()
			//
			stable->a = draft->a;
			stable->f = draft->f;
			stable->code = draft->code;
			assert(stable->u == draft->u);
			assert(stable->v == draft->v);
			//
			// Next register
			//
			stable = nextV(stable);
			draft = nextV(draft);
		}
	}
}
