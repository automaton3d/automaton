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
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void replicate(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell *cell = lattice + id;
		Cell* active_stack, *passive_stack;
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
		//
		for (int v = 0; v < SIDE2; v++)
		{
			active_stack->f = passive_stack->f;
			active_stack->b = passive_stack->b;
			active_stack->code = passive_stack->code;
			active_stack->noise |= passive_stack->noise;
			COPY(active_stack->pole, passive_stack->pole);
			//
			active_stack = nextV(active_stack);
			passive_stack = nextV(passive_stack);
		}
	}
}
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void replicate(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell *cell = lattice + id;
		Cell* active_stack, *passive_stack;
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
		//
		for (int v = 0; v < SIDE2; v++)
		{
			active_stack->f = passive_stack->f;
			active_stack->b = passive_stack->b;
			active_stack->code = passive_stack->code;
			active_stack->noise |= passive_stack->noise;
			COPY(active_stack->pole, passive_stack->pole);
			COPY(active_stack->p, passive_stack->p);	// pacth não é isso....
			//
			active_stack = nextV(active_stack);
			passive_stack = nextV(passive_stack);
		}
	}
}
