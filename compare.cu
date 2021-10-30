#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "automaton.cuh"

/*
 * Compares two columns to update variables b, f and code.
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
		Cell* ptr1, * ptr2;
		for (int i = 0; i < SIDE2; i++)
		{
			// Shift 'vertically' the passive column
			//
			ptr2 = draft;
			Cell temp = *ptr2;
			for (int j = 0; j < SIDE2; j++)
			{
				Cell* next = nextV(ptr2);
				if (j == SIDE2 - 1)
					next = &temp;
				ptr2->f = next->f;
				ptr2->a = next->a;
				ptr2->charge = next->charge;
				ptr2->code = next->code;
				//
				// Next pointer value
				//
				ptr2 = next;
			}
			//
			// Compare 'columns'
			//
			ptr1 = stable;
			ptr2 = draft;
			for (int j = 0; j < SIDE2; j++)
			{
				assert(stable->u == draft->u);
				assert(stable->v == draft->v);
				// Same sector?
				//
				if (((ptr1->charge ^ ptr2->charge) & D_MASK) == 0)
				{
					// Do they have same affinity?
					//
					if (ptr1->a == ptr2->a)
					{
						if (ptr1->code == COLLAPSE)
						{
							ptr2->code = 0;
							ptr2->f = 1;
							ptr2->a = ptr2->floor;
						}
						//
						// Are bubbles superposing
						//
						else if(ISEQUAL(ptr1->o, ptr2->o))
						{
							// Virgin?
							//
							if (ptr2->code == 0)
							{
								// Pair formation
								//
								unsigned char cc =
									(ptr2->charge & C_MASK) ^ (ptr1->charge & C_MASK);
								unsigned char ww =
									(ptr2->charge & W_MASK) ^ (ptr1->charge & W_MASK);
								unsigned char qq =
									(ptr2->charge & Q_MASK) ^ (ptr1->charge & Q_MASK);
								//
								if (cc == 0 && ww == 0 && qq == Q_MASK)
								{
									ptr2->a = ptr1->a;
									ptr2->code = NEUTRINO;
									ptr2->f++;
								}
								else if (cc == 0 && ww == W_MASK && qq == Q_MASK)
								{
									ptr2->a = ptr1->a;
									ptr2->code = GLUON;
									ptr2->f++;
								}
								else if (cc == C_MASK && ww == 0 && qq == 0)
								{
									ptr2->a = ptr1->a;
									ptr2->code = W;
									ptr2->f++;
								}
								else if (cc == C_MASK && ww == 0 && qq == Q_MASK)
								{
									ptr2->a = ptr1->a;
									ptr2->code = Z;
									ptr2->f++;
								}
								else if (cc == C_MASK && ww == W_MASK && qq == Q_MASK)
								{
									ptr2->a = ptr1->a;
									ptr2->code = PHOTON;
									ptr2->f++;
								}
							}
						}
					}
				}
				//
				// Next pointer values
				//
				ptr1 = nextV(ptr1);
				ptr2 = nextV(ptr2);
			}
		}
	}
}
