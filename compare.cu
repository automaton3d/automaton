#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void compare(Cell* lattice)
{
	long id = blockDim.x * blockIdx.x + threadIdx.x;
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
		if (passive_stack->t % LIGHT != 0)
		{
			return;
		}
		#ifdef COMPARE
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
				if (active_cell->b == passive_cell->b && ISEQUAL(active_cell->o, passive_cell->o))
				{
					if (passive_cell->code == 0)
					{
						if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == C_MASK && 
							  ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == W_MASK &&
							  ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == Q_MASK)
							passive_cell->code = PHOTON;
						else if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == 0 &&
							    ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == W_MASK &&
							    ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == Q_MASK)
							passive_cell->code = PHOTON;
						else if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == 0 &&
							     ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == 0 &&
							     ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == Q_MASK)
							passive_cell->code = PHOTON;
						else if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == 0 &&
							     ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == 0 &&
							     ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == Q_MASK)
							passive_cell->code = PHOTON;
						else if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == C_MASK &&
							     ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == 0 &&
							     ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == Q_MASK)
							passive_cell->code = PHOTON;
						else if (((passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK)) == C_MASK &&
							     ((passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK)) == 0 &&
							     ((passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK)) == 0)
							passive_cell->code = PHOTON;
						//
						if (passive_cell->code != 0)
							passive_cell->f++;
					}
					else if (passive_cell->code == active_cell->code)
					{
						passive_cell->f++;
					}
				}
				active_cell = nextV(active_cell);
				passive_cell = nextV(passive_cell);
			}
			active_stack = nextV(active_stack);
			passive_stack = nextV(passive_stack);
		}
	#endif
	}
}
