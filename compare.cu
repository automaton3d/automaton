#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void compare(struct Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		struct Cell* root = lattice + id;
		struct Cell *active_stack, *passive_stack;
		if (root->active)
		{
			active_stack = root;
			passive_stack = root->h;
		}
		else
		{
			active_stack = root->h;
			passive_stack = root;
		}
		struct Cell* active_cell = active_stack;
		struct Cell* passive_cell = passive_stack;;
		for (int i = 0; i < SIDE2; i++)
		{
			// Shift 'vertically'
			//
			passive_cell = passive_stack;
			Cell t1;
			Cell t2 = *passive_cell;
			for (int j = 0; j < SIDE2; j++)
			{
				t1 = *passive_cell->v;
				passive_cell->f = t2.f;
				passive_cell->b = t2.b;
				passive_cell->q = t2.q;
				passive_cell->w = t2.w;
				passive_cell->c = t2.c;
				passive_cell->d = t2.d;
				COPY(passive_cell->o, t2.o);
				COPY(passive_cell->p, t2.p);
				COPY(passive_cell->s, t2.s);
				passive_cell->phi = t2.phi;
				passive_cell->code = t2.code;
				t2 = t1;
				passive_cell = passive_cell->v;
			}
			//
			// Compare 'columns'
			//
			passive_cell = passive_stack;
			for (int j = 0; j < SIDE2; j++)
			{
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
				active_cell = active_cell->v;
				passive_cell = passive_cell->v;
			}
		}
	}
}
