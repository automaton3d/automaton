#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void replicate(struct Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		struct Cell* cell = lattice + id;
		if (!cell->active)
			cell = cell->h;
		for (int v = 0; v < SIDE2; v++)
		{
			cell->f = cell->h->f;
			cell->b = cell->h->b;
			COPY(cell->pole, cell->h->pole);
			cell->code = cell->h->code;
			cell->noise |= cell->h->noise;
			cell = cell->v;
		}
	}
}
