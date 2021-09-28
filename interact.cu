#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "curand.h"
#include "curand_kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include "automaton.h"

__device__ void compareColumns(Cell *stable1, Cell *stable2, Cell *draft1, Cell *draft2)
{
	// Isolate charge bits
	//
	unsigned c1 = (stable1->charge & C_MASK) & 7;
	unsigned q1 = ((stable1->charge & Q_MASK) >> 3) & 1;
	unsigned w1 = ((stable1->charge & W_MASK) >> 4) & 1;
	unsigned d1 = (stable1->charge & D_MASK) >> 5;
	unsigned c2 = (stable2->charge & C_MASK) & 7;
	unsigned q2 = ((stable2->charge & Q_MASK) >> 3) & 1;
	unsigned w2 = ((stable2->charge & W_MASK) >> 4) & 1;
	unsigned d2 = (stable2->charge & D_MASK) >> 5;
	//
	// Play pseudo dices
	//
	if (stable1->noise > abs(stable1->phi) &&
		stable2->noise > abs(stable2->phi) &&
		(!ISNULL(stable1->p) || !ISNULL(stable2->p)))
	{
		bool sig1 = (c1 == NEUTRAL && c2 == NEUTRAL && q1 == 1 && q2 == 1 && d1 == 0);
		bool sig2 = (c1 == N_BAR && c2 == N_BAR && q1 == 1 && q2 == 1 && d1 == 1);
		bool sig3 = (c2 != NEUTRAL && c2 != N_BAR && q1 == q2);
		//
		int c1 = (q1 == q2 && w1 == w2 && sig1 == sig2);
		int c2 = (q1 != q2 && w1 != w2 && sig1 != sig2);
		int c3 = (d1 == 0 && w1 == 0 && w1 != w2);
		int c4 = (d1 != d2 && w1 == 1 && w1 != w2);
		//
		// Same sector?
		//
		if (d1 == d2)
		{
			// Non-overlapping?
			//
			if (!ISEQUAL(stable1->o, stable2->o))
			{
				// Fermionic x Fermionic case
				//
				if (q1 != q2)
				{
					// Annihilation
					//
					draft1->b = (stable1->b * stable2->b) % SIDE2;
					draft2->b = draft1->b;
					//
					// Reissue R1 and R2 from this
					//
					RESET(draft1->o);
					RESET(draft2->o);
				}
				//
				// Are the two cells similar?
				//
				else if (q1 == q2 && w1 == w2 && c1 == c2)
				{
					// Cohesion
					//
					if (stable1->b != stable2->b)
					{
						// Calculate the new unique bonding value
						//
						draft1->b = (stable1->b * stable2->b) % SIDE2;
						draft2->b = draft1->b;
					}
					//
					// Exchange spins: s1 <-> s2
					//
					draft1->s[0] = stable2->s[0];
					draft2->s[0] = stable1->s[0];
					draft1->s[1] = stable2->s[1];
					draft2->s[1] = stable1->s[1];
					draft1->s[2] = stable2->s[2];
					draft2->s[2] = stable1->s[2];
					//
					// Reissue R1 from pole(R1) and R2 from pole(R2)
					//
					RESET(draft1->o);
					RESET(draft2->o);
					COPY(draft1->pole, stable1->p);
					COPY(draft2->pole, stable2->p);
				}
			}
			//
			// Bosonic x Bosonic case
			//
			else if (stable1->f > 1 && stable2->f > 1)
			{
				// gluon-gluon?
				//
				if (c1 == c2 && stable1->code == GLUON && stable2->code == GLUON)
				{
					// Swap colors
					//
					int color1 = stable1->charge & C_MASK;
					int color2 = stable2->charge & C_MASK;
					draft1->charge &= ~C_MASK;
					draft2->charge &= ~C_MASK;
					draft1->charge |= color2;
					draft2->charge |= color1;
					//
					// Reissue R1 from pole(R1)
					//
					RESET(draft1->o);
					draft1->dir = 0;
					draft1->t = 0;
					draft2->dir = 0;
					draft2->t = 0;
				}
				else if (!ISNULL(stable1->p) && !ISNULL(stable2->p))
				{
					// Chiral?
					//
					if (c1 || c2 || c3 || c4)
					{
						// Reissue R1 and R2 from cstable1
						//
						draft2->b = draft1->b;
						RESET(draft1->o);
						RESET(draft2->o);
					}
					else
					{
						// TODO
					}
				}
				else if (sig1 != 0 && sig1 != 3 && sig2 != 0 && sig2 != 3)
				{
					// Swap colors
					//
					int c1 = stable1->charge & C_MASK;
					int c2 = stable2->charge & C_MASK;
					draft1->charge &= ~C_MASK;
					draft2->charge &= ~C_MASK;
					draft1->charge |= c2;
					draft2->charge |= c1;
					//
					draft2->b = draft1->b;
					//
					// Reissue R1 and R2 from cstable1
					//
					draft2->b = draft1->b;
					RESET(draft1->o);
					RESET(draft2->o);
				}
			}
			//
			// F x B
			//
			if (stable1->f == 1 && stable2->f > 1)
			{
				// F x B
				//
				if ((stable1->charge & C_MASK) != 0 && (stable1->charge & C_MASK) != C_MASK && (stable2->charge & C_MASK) != 0 &&
					(stable2->charge & C_MASK) != C_MASK)
				{
					// Swap colors
					//
					int c1 = stable1->charge & C_MASK;
					int c2 = stable2->charge & C_MASK;
					draft1->charge &= ~C_MASK;
					draft2->charge &= ~C_MASK;
					draft1->charge |= c2;
					draft2->charge |= c1;
					draft2->b = draft2->b;
					//
					// Reissue R1 from pole(R1) and R2 from pole(R2)
					//
					RESET(draft1->o);
					RESET(draft2->o);
				}
				else
				{
					draft2->b = draft1->b;
					//
					// Reissue R1 and R2 from this
					//
					RESET(draft1->pole);
					RESET(draft1->o);
					RESET(draft2->pole);
					RESET(draft2->o);
				}
			}
			else if (stable1->f > 1 && stable2->f == 1)
			{
				// B x F
				//
				if ((stable1->charge & C_MASK) != 0 && (stable1->charge & C_MASK) != C_MASK && (stable2->charge & C_MASK) != 0 &&
					(stable2->charge & C_MASK) != C_MASK)
				{
					// Swap colors
					//
					int c1 = stable1->charge & C_MASK;
					int c2 = stable2->charge & C_MASK;
					draft1->charge &= ~C_MASK;
					draft2->charge &= ~C_MASK;
					draft1->charge |= c2;
					draft2->charge |= c1;
					//
					// Reissue R1 from pole(R1) and R2 from pole(R2)
					//
					RESET(draft1->o);
					RESET(draft2->o);
				}
				else
				{
					draft2->b = draft1->b;
					//
					// Reissue R1 and R2 from this
					//
					RESET(draft1->pole);
					RESET(draft2->pole);
				}
			}
			else if (stable1->b == stable2->b)
			{
				// Messenger interactions
				//
				if (!ISNULL(stable1->p))
				{
					// REISSUE(stable, POLE(stable))
					//
					RESET(draft1->pole);
					//
					// REISSUE(draft, TRANSPORT(draft, stable));
					//
					draft2->pole[0] = draft1->o[0] - stable2->o[0];
					draft2->pole[1] = draft1->o[1] - stable2->o[1];
					draft2->pole[2] = draft1->o[2] - stable2->o[2];
				}
				else
				{
					// REISSUE(draft, POLE(draft));
					//
					RESET(draft2->pole);
					//
					// REISSUE(stable, TRANSPORT(stable, draft));
					//
					draft1->pole[0] = stable2->o[0] - stable1->o[0];
					draft1->pole[1] = stable2->o[1] - stable1->o[1];
					draft1->pole[2] = stable2->o[2] - stable1->o[2];
				}
			}
		}
		else
		{
			// Inter-sector
			//
			if ((d1 == 0 && sig1 == sig2) || (d1 == 1 && sig1 == sig3))
			{
				// Swap colors
				//
				int c1 = stable1->charge & C_MASK;
				int c2 = stable2->charge & C_MASK;
				draft1->charge &= ~C_MASK;
				draft2->charge &= ~C_MASK;
				draft1->charge |= c2;
				draft2->charge |= c1;
				//
				// Reissue R1 and R2 from this
				//
				RESET(draft1->pole);
				RESET(draft2->pole);
			}
			//
			// Chiral?
			//
			else if (q1 == q2 && w1 == w2)//c1 || c2 || c3 || c4)
			{
				int c1 = stable1->charge & W_MASK;
				int c2 = stable2->charge & W_MASK;
				draft1->charge &= ~W_MASK;
				draft2->charge &= ~W_MASK;
				draft1->charge |= c2;
				draft2->charge |= c1;
				//
				// Reissue R1 and R2 from this
				//
				RESET(draft1->pole);
				RESET(draft2->pole);
			}
		}
	}
}

__global__ void interact(Cell* lattice)
{
	long xyz = blockDim.x * blockIdx.x + threadIdx.x;
	if (xyz < SIDE3)
	{
		Cell* draft = lattice + xyz;
		Cell* stable = draft + SIDE2 * SIDE3;
		if (draft->active)
		{
			Cell* temp = draft;
			draft = stable;
			stable = temp;
		}
		//
		// Interactions only allowed at the last tick of a light step
		//
		if (draft->t % LIGHT != 0)
			return;
		//
		// Compare columns for interaction match
		//
		Cell* stable1 = stable;
		Cell* draft1 = draft;
		for (int i = 0; i < SIDE2; i++)
		{
			// If the re-emission cell was reached, reset the pole vector to the free bubble
			//
			if (ISNULL(stable1->pole))
			{
				COPY(draft1->pole, stable1->p);
				RESET(draft1->o);
				draft1->t = 0;
			}
			else
			{
				Cell* stable2 = stable;
				Cell* draft2 = draft;
				for (int j = 0; j < SIDE2; j++)
				{
					if (i != j && stable1->f > 0 && stable2->f > 0 &&
						stable1->b != stable2->b && !ISEQUAL(stable1->o, stable2->o))
						compareColumns(stable1, stable2, draft1, draft2);
					//
					stable2 = nextV(stable2);
					draft2 = nextV(draft2);
				}
			}
			//
			stable1 = nextV(stable1);
			draft1 = nextV(draft1);
		}
	}
}
