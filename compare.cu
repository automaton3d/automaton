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
				ptr2->chrg = next->chrg;
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
				if (((ptr1->chrg ^ ptr2->chrg) & D_MASK) == 0)
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
								ptr2->code = 
									((ptr2->chrg & C_MASK) ^ (ptr1->chrg & C_MASK)) |
									((ptr2->chrg & W_MASK) ^ (ptr1->chrg & W_MASK)) |
									((ptr2->chrg & Q_MASK) ^ (ptr1->chrg & Q_MASK));
								//
								// Affinity is the same now
								//
								ptr2->a = ptr1->a;
								//
								// Adjust frequency
								//
								ptr2->f++;
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
