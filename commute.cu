#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void commute(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell* cell = lattice + id;
		Cell* passive_stack, * active_stack;
		if (cell->active)
		{
			active_stack = cell;
			passive_stack = cell + SIDE3 * SIDE2;
		}
		else
		{
			passive_stack = cell;
			active_stack = cell + SIDE3 * SIDE2;
		}
		for (int v = 0; v < SIDE2; v++)
		{
			active_stack->active = false;
			passive_stack->active = true;
			active_stack->charge = passive_stack->charge;
			COPY(active_stack->o, passive_stack->o);
			COPY(active_stack->p, passive_stack->p);
			COPY(active_stack->s, passive_stack->s);
			COPY(active_stack->pole, passive_stack->pole);
			active_stack->code = 0;
			active_stack->noise ^= passive_stack->noise;
			active_stack->f = passive_stack->f;
			active_stack->b = passive_stack->b;
			if (ISNULL(passive_stack->o) && !ISNULL(passive_stack->p))
			{
				active_stack->f = 1;
				active_stack->b = 0;
			}
			passive_stack = nextV(passive_stack);
			active_stack = nextV(active_stack);
		}
	}
}

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void commute(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell* cell = lattice + id;
		Cell* passive_stack, * active_stack;
		if (cell->active)
		{
			active_stack = cell;
			passive_stack = cell + SIDE3 * SIDE2;
		}
		else
		{
			passive_stack = cell;
			active_stack = cell + SIDE3 * SIDE2;
		}
		for (int v = 0; v < SIDE2; v++)
		{
			active_stack->active = false;
			passive_stack->active = true;
			active_stack->charge = passive_stack->charge;
			COPY(active_stack->o, passive_stack->o);
			COPY(active_stack->p, passive_stack->p);
			COPY(active_stack->s, passive_stack->s);
			COPY(active_stack->pole, passive_stack->pole);
			active_stack->code = 0;
			active_stack->noise ^= passive_stack->noise;
			active_stack->f = passive_stack->f;
			active_stack->b = passive_stack->b;
			active_stack->synch = passive_stack->synch;
			if (ISNULL(passive_stack->o) && !ISNULL(passive_stack->p))
			{
				active_stack->f = 1;
				active_stack->b = 0;
			}
			passive_stack = nextV(passive_stack);
			active_stack = nextV(active_stack);
		}
	}
}

