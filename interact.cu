#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void interact(Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		Cell* cell = lattice + id;
		Cell *active_stack, *passive_stack;
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
		for (int v = 0; v < 1/*SIDE2*/; v++)
		{
			int sig1 = ((active_stack->charge ^ passive_stack->charge) & C_MASK) == 0 && (passive_stack->charge & C_MASK) == 0 &&
				((active_stack->charge ^ passive_stack->charge) & Q_MASK) == 0 && (passive_stack->charge & Q_MASK) == Q_MASK && 
				((active_stack->charge ^ passive_stack->charge) & D_MASK) == 0 && (passive_stack->charge & D_MASK) == 0;
			int sig2 = ((active_stack->charge ^ passive_stack->charge) & C_MASK) == 0 &&
				(passive_stack->charge & C_MASK) == C_MASK && ((active_stack->charge ^ passive_stack->charge) & Q_MASK) == 0 && 
				(passive_stack->charge & Q_MASK) == 0 && ((active_stack->charge ^ passive_stack->charge) & D_MASK) == 0 && (passive_stack->charge & D_MASK) == D_MASK;
			int sig3 = ((active_stack->charge ^ passive_stack->charge) & C_MASK) == 0 && (passive_stack->charge & C_MASK) != 0 && (passive_stack->charge & C_MASK) != C_MASK &&
				((active_stack->charge ^ passive_stack->charge) & Q_MASK) == 0;
			//
			int c1 = ((active_stack->charge ^ passive_stack->charge) & Q_MASK) == 0 && ((active_stack->charge ^ passive_stack->charge) & W_MASK) == 0 && sig1 == sig2;
			int c2 = ((active_stack->charge ^ passive_stack->charge) & Q_MASK) != 0 && ((active_stack->charge ^ passive_stack->charge) & W_MASK) != 0 && sig1 != sig2;
			int c3 = (active_stack->charge & D_MASK) == 0 && (active_stack->charge & W_MASK) == 0 && ((active_stack->charge ^ passive_stack->charge) & W_MASK) != 0;
			int c4 = (active_stack->charge & D_MASK) == D_MASK && (active_stack->charge & W_MASK) == W_MASK && ((active_stack->charge ^ passive_stack->charge) & W_MASK) != 0;
			//
			if(ISNULL(active_stack->pole))
				COPY(passive_stack->pole, active_stack->p);
			//
			// Play pseudo dices
			//
			if (false)//active_stack->noise > abs(active_stack->phi) && passive_stack->noise > abs(passive_stack->phi) && (!ISNULL(active_stack->p) || !ISNULL(passive_stack->p)))
			{
				if (((active_stack->charge ^ passive_stack->charge) & D_MASK) == 0)
				{
					// Same sector?
					//
					if (active_stack->f == 1 && passive_stack->f == 1)
					{
						// F x F
						//
						if (((active_stack->charge ^ passive_stack->charge) & Q_MASK) != 0)
						{
							// Annihilation?
							//
							active_stack->b = (active_stack->b * passive_stack->b) % SIDE2; // ??? erro ???
							passive_stack->b = active_stack->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(active_stack->pole);// ???
							COPY(passive_stack->pole, active_stack->p);
						}
						else if (sig1 || sig2 || sig3)
						{
							// Similar?
							//
							// Cohesion
							//
							if (active_stack->b != passive_stack->b)
							{
								active_stack->b = (active_stack->b * passive_stack->b) % SIDE2;
								passive_stack->b = active_stack->b;
							}
							//
							// s1 <-> s2
							//
							int temp;
							temp = active_stack->s[0];
							passive_stack->s[0] = active_stack->s[0];
							active_stack->s[0] = temp;
							temp = active_stack->s[1];
							passive_stack->s[1] = active_stack->s[1];
							active_stack->s[1] = temp;
							temp = active_stack->s[2];
							passive_stack->s[2] = active_stack->s[2];
							active_stack->s[2] = temp;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(active_stack->o);
							RESET(passive_stack->o);	//???
							COPY(passive_stack->pole, active_stack->p); // ???
						}
					}
					else if (active_stack->f > 1 && passive_stack->f > 1)
					{
						// B x B
						//
						if (((active_stack->charge ^ ~passive_stack->charge) & C_MASK) == 0 && active_stack->code == passive_stack->code && passive_stack->code == GLUON)
						{
							// gluon-gluon?
							//
							// Swap colors
							//
							int temp = active_stack->charge & C_MASK;
							active_stack->charge &= ~C_MASK;
							active_stack->charge |= (passive_stack->charge & C_MASK);
							passive_stack->charge &= ~C_MASK;
							passive_stack->charge |= temp;
							//
							// Reissue R1 from pole(R1)
							//
							RESET(active_stack->o);
							passive_stack->d0 = 0;	// replicar !!!
							passive_stack->t = 0;	// replicar !!!
						}
						else if (!ISNULL(active_stack->p) && !ISNULL(passive_stack->p))
						{
							if (c1 || c2 || c3 || c4)
							{
								// chiral?
								//
								// Reissue R1 and R2 from cp1
								//
								passive_stack->b = active_stack->b;
								RESET(active_stack->o);
								//
								// Reissue R2 from cp1
								//
								RESET(passive_stack->o);
							}
							else
							{
								// TODO
							}
						}
						else if (sig1 != 0 && sig1 != 3 && sig2 != 0 && sig2 != 3)
						{
							int temp = active_stack->charge & C_MASK;
							active_stack->charge &= ~C_MASK;
							active_stack->charge |= (passive_stack->charge & C_MASK);
							passive_stack->charge &= ~C_MASK;
							passive_stack->charge |= temp;
							passive_stack->b = active_stack->b;
							//
							// Reissue R1 and R2 from cp1
							//
							passive_stack->b = active_stack->b;
							RESET(active_stack->o);
							RESET(passive_stack->o);
						}
					}
					if (active_stack->f == 1 && passive_stack->f > 1)
					{
						// F x B
						//
						if ((active_stack->charge & C_MASK) != 0 && (active_stack->charge & C_MASK) != C_MASK && (passive_stack->charge & C_MASK) != 0 && 
							(passive_stack->charge & C_MASK) != C_MASK)
						{
							int temp = active_stack->charge & C_MASK;
							active_stack->charge &= ~C_MASK;
							active_stack->charge |= (passive_stack->charge & C_MASK);
							passive_stack->charge &= ~C_MASK;
							passive_stack->charge |= temp;
							passive_stack->b = active_stack->b;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(active_stack->o);
							RESET(passive_stack->o);
						}
						else
						{
							passive_stack->b = active_stack->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(active_stack->pole);
							RESET(active_stack->o);
							RESET(passive_stack->pole);
							RESET(passive_stack->o);
						}
					}
					else if (active_stack->f > 1 && passive_stack->f == 1)
					{
						// B x F
						//
						if ((active_stack->charge & C_MASK) != 0 && (active_stack->charge & C_MASK) != C_MASK && (passive_stack->charge & C_MASK) != 0 && 
							(passive_stack->charge & C_MASK) != C_MASK)
						{
							int temp = active_stack->charge & C_MASK;
							active_stack->charge &= ~C_MASK;
							active_stack->charge |= (passive_stack->charge & C_MASK);
							passive_stack->charge &= ~C_MASK;
							passive_stack->charge |= temp;
							//
							// Reissue R1 from pole(R1) and R2 from pole(R2)
							//
							RESET(active_stack->o);
							RESET(passive_stack->o);
						}
						else
						{
							passive_stack->b = active_stack->b;
							//
							// Reissue R1 and R2 from this
							//
							RESET(active_stack->pole);
							RESET(passive_stack->pole);
						}
					}
					else if (active_stack->b == passive_stack->b)
					{
						// Messenger interactions
						//
						if (!ISNULL(active_stack->p))
						{
							// REISSUE(active_stack, POLE(active_stack))
							//
							RESET(active_stack->pole);
							//
							// REISSUE(passive_stack, TRANSPORT(passive_stack, active_stack));
							//
							passive_stack->pole[0] = active_stack->o[0] - passive_stack->o[0];
							passive_stack->pole[1] = active_stack->o[1] - passive_stack->o[1];
							passive_stack->pole[2] = active_stack->o[2] - passive_stack->o[2];
						}
						else
						{
							// REISSUE(passive_stack, POLE(passive_stack));
							//
							RESET(passive_stack->pole);
							//
							// REISSUE(active_stack, TRANSPORT(active_stack, passive_stack));
							//
							active_stack->pole[0] = passive_stack->o[0] - active_stack->o[0];
							active_stack->pole[1] = passive_stack->o[1] - active_stack->o[1];
							active_stack->pole[2] = passive_stack->o[2] - active_stack->o[2];
						}
					}
				}
				else
				{
					// Inter-sector
					//
					if ((((active_stack->charge & D_MASK) == 0 && sig1 == 2) || ((active_stack->charge & D_MASK) == 1 && sig1 == 3)))
					{
						int temp = active_stack->charge & C_MASK;
						active_stack->charge &= ~C_MASK;
						active_stack->charge |= (passive_stack->charge & C_MASK);
						passive_stack->charge &= ~C_MASK;
						passive_stack->charge |= temp;
						//
						// Reissue R1 and R2 from this
						//
						RESET(active_stack->pole);
						RESET(passive_stack->pole);
					}
					else if (c1 || c2 || c3 || c4)
					{
						// Chiral?
						//
						int temp = active_stack->charge & W_MASK;
						active_stack->charge &= ~W_MASK;
						active_stack->charge |= (passive_stack->charge & W_MASK);
						passive_stack->charge &= ~W_MASK;
						passive_stack->charge |= temp;
						//
						// Reissue R1 and R2 from this
						//
						RESET(active_stack->pole);
						RESET(passive_stack->pole);
					}
				}
			}
			active_stack->t = 0;
			passive_stack->t = 0;
			active_stack->synch = -1;
			passive_stack->synch = -1;
			active_stack = nextV(active_stack);
			passive_stack = nextV(passive_stack);
		}
	}
}
