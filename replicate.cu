#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include <assert.h>
#include "automaton.h"

__global__ void replicate(Cell* lattice)
{
	long xyz = blockDim.x * blockIdx.x + threadIdx.x;
	if (xyz < SIDE3)
	{
		Cell* draft = lattice + xyz;
		Cell* stable = draft + SIDE2 * SIDE3;
		if (draft->active)
		{
			Cell* temp = draft;
			draft = stable;
			stable = temp;
		}
		if (draft->t % LIGHT != 0)
		{
			return;
		}
		//
		for (int v = 0; v < SIDE2; v++)
		{
			// Copy only variables that changed in compare()
			//
			stable->f = draft->f;
			stable->code = draft->code;
			assert(ISEQUAL(stable->pole, draft->pole));
			//
			// Next register
			//
			stable = nextV(stable);
			draft = nextV(draft);
		}
	}
}
