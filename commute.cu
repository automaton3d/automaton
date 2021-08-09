#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void commute(struct Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		struct Cell* cell = lattice + id;
		if (!cell->active)
			cell = cell->h;
		//
		for (int v = 0; v < SIDE2; v++)
		{
			cell->active = false;
			cell->h->active = true;
			cell->h->q = cell->q;
			cell->h->w = cell->w;
			cell->h->c = cell->c;
			cell->h->d = cell->d;
			COPY(cell->h->o, cell->o);
			COPY(cell->h->p, cell->p);
			COPY(cell->h->s, cell->s);
			COPY(cell->h->pole, cell->pole);
			cell->h->code = 0;
			cell->h->noise ^= cell->noise;
			cell->h->f = cell->f;
			cell->h->b = cell->b;
			if (ISNULL(cell->o) && !ISNULL(cell->p))
			{
				cell->h->f = 1;
				cell->h->b = 0;
			}
			cell = cell->v;
		}
	}
}

