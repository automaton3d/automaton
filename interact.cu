#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "curand.h"
#include "curand_kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include "automaton.h"

__device__ unsigned int rnd;

__global__ void interact(Cell* lattice)
{
	long id = blockDim.x * blockIdx.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell* cell = lattice + id;
		Cell *stable, *draft;
		if (cell->active)
		{
			stable = cell;
			draft = cell + SIDE3 * SIDE2;
		}
		else
		{
			draft = cell;
			stable = cell + SIDE3 * SIDE2;
		}
		//
		// Interactions only allowed at the last tick of a light step
		//
		if (draft->t % LIGHT != 0)
		{
			return;
		}
		curandState state;
		// curand_init(0, id, 0, &state);

		//
		// Compare the two columns for interaction matches
		//
		for (int v = 0; v < SIDE2; v++)
		{
			#ifdef INTERACT
			int sig1 = ((stable->charge ^ draft->charge) & C_MASK) == 0 && (draft->charge & C_MASK) == 0 &&
				((stable->charge ^ draft->charge) & Q_MASK) == 0 && (draft->charge & Q_MASK) == Q_MASK && 
				((stable->charge ^ draft->charge) & D_MASK) == 0 && (draft->charge & D_MASK) == 0;
			int sig2 = ((stable->charge ^ draft->charge) & C_MASK) == 0 &&
				(draft->charge & C_MASK) == C_MASK && ((stable->charge ^ draft->charge) & Q_MASK) == 0 && 
				(draft->charge & Q_MASK) == 0 && ((stable->charge ^ draft->charge) & D_MASK) == 0 && (draft->charge & D_MASK) == D_MASK;
			int sig3 = ((stable->charge ^ draft->charge) & C_MASK) == 0 && (draft->charge & C_MASK) != 0 && (draft->charge & C_MASK) != C_MASK &&
				((stable->charge ^ draft->charge) & Q_MASK) == 0;
			//
			int c1 = ((stable->charge ^ draft->charge) & Q_MASK) == 0 && ((stable->charge ^ draft->charge) & W_MASK) == 0 && sig1 == sig2;
			int c2 = ((stable->charge ^ draft->charge) & Q_MASK) != 0 && ((stable->charge ^ draft->charge) & W_MASK) != 0 && sig1 != sig2;
			int c3 = (stable->charge & D_MASK) == 0 && (stable->charge & W_MASK) == 0 && ((stable->charge ^ draft->charge) & W_MASK) != 0;
			int c4 = (stable->charge & D_MASK) == D_MASK && (stable->charge & W_MASK) == W_MASK && ((stable->charge ^ draft->charge) & W_MASK) != 0;
			//
			if(ISNULL(stable->pole))
				COPY(draft->pole, stable->p);
			//
			// Play pseudo dices
			//
			if (stable->noise > abs(stable->phi) && draft->noise > abs(draft->phi) && (!ISNULL(stable->p) || !ISNULL(draft->p)))
			{
				if (((stable->charge ^ draft->charge) & D_MASK) == 0)
				{
					// Same sector?
					//
					if (stable->f == 1 && draft->f == 1)
					{
						// F x F
						//
						if (((stable->charge ^ draft->charge) & Q_MASK) != 0)
						{
							// Annihilation?
							//
							stable->b = (stable->b * draft->b) % SIDE2; // ??? erro ???
							draft->b = stable->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(stable->pole);// ???
							COPY(draft->pole, stable->p);
						}
						else if (sig1 || sig2 || sig3)
						{
							// Similar?
							//
							// Cohesion
							//
							if (stable->b != draft->b)
							{
								stable->b = (stable->b * draft->b) % SIDE2;
								draft->b = stable->b;
							}
							//
							// s1 <-> s2
							//
							int temp;
							temp = stable->s[0];
							draft->s[0] = stable->s[0];
							stable->s[0] = temp;
							temp = stable->s[1];
							draft->s[1] = stable->s[1];
							stable->s[1] = temp;
							temp = stable->s[2];
							draft->s[2] = stable->s[2];
							stable->s[2] = temp;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(stable->o);
							RESET(draft->o);	//???
							COPY(draft->pole, stable->p); // ???
						}
					}
					else if (stable->f > 1 && draft->f > 1)
					{
						// B x B
						//
						if (((stable->charge ^ ~draft->charge) & C_MASK) == 0 && stable->code == draft->code && draft->code == GLUON)
						{
							// gluon-gluon?
							//
							// Swap colors
							//
							int temp = stable->charge & C_MASK;
							stable->charge &= ~C_MASK;
							stable->charge |= (draft->charge & C_MASK);
							draft->charge &= ~C_MASK;
							draft->charge |= temp;
							//
							// Reissue R1 from pole(R1)
							//
							RESET(stable->o);
							draft->dir = 0;	// replicar !!!
							draft->t = 0;	// replicar !!!
						}
						else if (!ISNULL(stable->p) && !ISNULL(draft->p))
						{
							if (c1 || c2 || c3 || c4)
							{
								// chiral?
								//
								// Reissue R1 and R2 from cp1
								//
								draft->b = stable->b;
								RESET(stable->o);
								//
								// Reissue R2 from cp1
								//
								RESET(draft->o);
							}
							else
							{
								// TODO
							}
						}
						else if (sig1 != 0 && sig1 != 3 && sig2 != 0 && sig2 != 3)
						{
							int temp = stable->charge & C_MASK;
							stable->charge &= ~C_MASK;
							stable->charge |= (draft->charge & C_MASK);
							draft->charge &= ~C_MASK;
							draft->charge |= temp;
							draft->b = stable->b;
							//
							// Reissue R1 and R2 from cp1
							//
							draft->b = stable->b;
							RESET(stable->o);
							RESET(draft->o);
						}
					}
					if (stable->f == 1 && draft->f > 1)
					{
						// F x B
						//
						if ((stable->charge & C_MASK) != 0 && (stable->charge & C_MASK) != C_MASK && (draft->charge & C_MASK) != 0 && 
							(draft->charge & C_MASK) != C_MASK)
						{
							int temp = stable->charge & C_MASK;
							stable->charge &= ~C_MASK;
							stable->charge |= (draft->charge & C_MASK);
							draft->charge &= ~C_MASK;
							draft->charge |= temp;
							draft->b = stable->b;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(stable->o);
							RESET(draft->o);
						}
						else
						{
							draft->b = stable->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(stable->pole);
							RESET(stable->o);
							RESET(draft->pole);
							RESET(draft->o);
						}
					}
					else if (stable->f > 1 && draft->f == 1)
					{
						// B x F
						//
						if ((stable->charge & C_MASK) != 0 && (stable->charge & C_MASK) != C_MASK && (draft->charge & C_MASK) != 0 && 
							(draft->charge & C_MASK) != C_MASK)
						{
							int temp = stable->charge & C_MASK;
							stable->charge &= ~C_MASK;
							stable->charge |= (draft->charge & C_MASK);
							draft->charge &= ~C_MASK;
							draft->charge |= temp;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(stable->o);
							RESET(draft->o);
						}
						else
						{
							draft->b = stable->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(stable->pole);
							RESET(draft->pole);
						}
					}
					else if (stable->b == draft->b)
					{
						// Messenger interactions
						//
						if (!ISNULL(stable->p))
						{
							// REISSUE(stable, POLE(stable))
							//
							RESET(stable->pole);
							//
							// REISSUE(draft, TRANSPORT(draft, stable));
							//
							draft->pole[0] = stable->o[0] - draft->o[0];
							draft->pole[1] = stable->o[1] - draft->o[1];
							draft->pole[2] = stable->o[2] - draft->o[2];
						}
						else
						{
							// REISSUE(draft, POLE(draft));
							//
							RESET(draft->pole);
							//
							// REISSUE(stable, TRANSPORT(stable, draft));
							//
							stable->pole[0] = draft->o[0] - stable->o[0];
							stable->pole[1] = draft->o[1] - stable->o[1];
							stable->pole[2] = draft->o[2] - stable->o[2];
						}
					}
				}
				else
				{
					// Inter-sector
					//
					if ((((stable->charge & D_MASK) == 0 && sig1 == 2) || ((stable->charge & D_MASK) == 1 && sig1 == 3)))
					{
						int temp = stable->charge & C_MASK;
						stable->charge &= ~C_MASK;
						stable->charge |= (draft->charge & C_MASK);
						draft->charge &= ~C_MASK;
						draft->charge |= temp;
						//
						// Reissue R1 and R2 from this
						//
						RESET(stable->pole);
						RESET(draft->pole);
					}
					else if (c1 || c2 || c3 || c4)
					{
						// Chiral?
						//
						int temp = stable->charge & W_MASK;
						stable->charge &= ~W_MASK;
						stable->charge |= (draft->charge & W_MASK);
						draft->charge &= ~W_MASK;
						draft->charge |= temp;
						//
						// Reissue R1 and R2 from this
						//
						RESET(stable->pole);
						RESET(draft->pole);
					}
				}
			}
			#endif
			if (!ISNULL(stable->p) && !ISNULL(stable->o))
			{
				rnd = curand(&state);
				if (rnd % 100 < 50 && draft->t > 0)
				{
					rnd = curand(&state);
					//printf("t=%d LIGHT=%d: %d\n", draft->t , draft->t / LIGHT, rnd);
				}
			}
			//
			// Next register
			//
			stable = nextV(stable);
			draft = nextV(draft);
		}
	}
}
