#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include "automaton.h"

/*
 * Compares two columns to update variables f and code.
 */
__global__ void compare(Cell* lattice)
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
		// Not the last tick?
		//
		if (draft->t % LIGHT != 0)
			return;
		//
		#define COMPARE
		#if defined(COMPARE)
		Cell* active_cell, * passive_cell;
		for (int i = 0; i < SIDE2; i++)
		{
			// Shift 'vertically' the passive column
			//
			passive_cell = draft;
			Cell temp = *passive_cell;
			for (int j = 0; j < SIDE2; j++)
			{
				Cell* next = nextV(passive_cell);
				if (j == SIDE2 - 1)
					next = &temp;
				passive_cell->f = next->f;
				passive_cell->b = next->b;
				passive_cell->charge = next->charge;
				COPY(passive_cell->o, next->o);
				COPY(passive_cell->p, next->p);
				COPY(passive_cell->s, next->s);
				passive_cell->phi = next->phi;
				passive_cell->code = next->code;
				//
				// Next pointer value
				//
				passive_cell = next;
			}
			//
			// Compare 'columns'
			//
			active_cell = stable;
			passive_cell = draft;
			for (int j = 0; j < SIDE2; j++)
			{
				// Test if the bubbles are superposing
				//
				if (active_cell->b == passive_cell->b &&
					ISEQUAL(active_cell->o, passive_cell->o))
				{
					// Virgin?
					//
					if (passive_cell->code == 0)
					{
						unsigned char cc = 
							(passive_cell->charge & C_MASK) ^ (active_cell->charge & C_MASK);
						unsigned char ww = 
							(passive_cell->charge & W_MASK) ^ (active_cell->charge & W_MASK);
						unsigned char qq = 
							(passive_cell->charge & Q_MASK) ^ (active_cell->charge & Q_MASK);
						//
						if (cc == 0 && ww == 0 && qq == Q_MASK)
							passive_cell->code = NEUTRINO;
						else if (cc == 0 && ww == W_MASK && qq == Q_MASK)
							passive_cell->code = GLUON;
						else if (cc == C_MASK && ww == 0 && qq == 0)
							passive_cell->code = W;
						else if (cc == C_MASK && ww == 0 && qq == Q_MASK)
							passive_cell->code = Z;
						else if (cc == C_MASK && ww == W_MASK && qq == Q_MASK)
							passive_cell->code = PHOTON;
						//
						if (passive_cell->code != 0 && passive_cell->f > 0)
							passive_cell->f++;
					}
					else if (passive_cell->code == active_cell->code && 
						passive_cell->f > 0)
					{
						passive_cell->f++;
					}
				}
				//
				// Next pointer values
				//
				active_cell = nextV(active_cell);
				passive_cell = nextV(passive_cell);
			}
		}
		#endif
	}
}
