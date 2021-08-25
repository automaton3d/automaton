#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void compare(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell* cell = lattice + id;
		Cell* active_stack, * passive_stack;
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
		for (int i = 0; i < SIDE2; i++)
		{
			// Shift 'vertically'
			//
			Cell temp = *cell;
			for (int j = 0; j < SIDE2; j++)
			{
				Cell* next = nextV(cell);
				if (j == SIDE2 - 1)
					next = &temp;
				cell->f = next->f;
				cell->b = next->b;
				cell->charge = next->charge;
				COPY(cell->o, next->o);
				COPY(cell->p, next->p);
				COPY(cell->s, next->s);
				cell->phi = next->phi;
				cell->code = next->code;
				cell = nextV(cell);
			}
			//
			// Compare 'columns'
			//
			Cell* active_cell = active_stack;
			Cell *passive_cell = passive_stack;
			for (int j = 0; j < SIDE2; j++)
			{
				/*
				if (active_cell->b == passive_cell->b && ISEQUAL(active_cell->o, passive_cell->o))
				{
					if (passive_cell->code == 0)
					{
						if (passive_cell->c == ~active_cell->c && passive_cell->w == ~active_cell->w &&
							passive_cell->q == ~active_cell->q)
							passive_cell->code = PHOTON;
						else if (passive_cell->c == active_cell->c && passive_cell->w == ~active_cell->w &&
							passive_cell->q == ~active_cell->q)
							passive_cell->code = GLUON;
						else if (passive_cell->c == active_cell->c && passive_cell->w == active_cell->w &&
							passive_cell->q == ~active_cell->q)
							passive_cell->code = NEUTRINO;
						else if (passive_cell->c == ~active_cell->c && passive_cell->w == active_cell->w &&
							passive_cell->q == ~active_cell->q)
							passive_cell->code = Z;
						else if (passive_cell->c == ~active_cell->c && passive_cell->w == active_cell->w &&
							passive_cell->q == active_cell->q)
							passive_cell->code = W;
						//
						if (passive_cell->code != 0)
							passive_cell->f++;
					}
					else if (passive_cell->code == active_cell->code)
					{
						passive_cell->f++;
					}
				}
				*/
		//		active_cell = nextV(active_cell);
			//	passive_cell = nextV(passive_cell);
			}
			active_stack = nextV(active_stack);
			passive_stack = nextV(passive_stack);
		}
	}
}
