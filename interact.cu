/*
 * Interaction routines
 * 
 */
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "automaton.cuh"

/*
 * Both re-emmited from CP.
 */
__device__ void cpcp(Cell* draft1, Cell* draft2, bool collapse)
{
	// Copy momentum information for flash
	//
	if (ALIGNED(draft1->o, draft1->p))
	{
		COPY(draft1->pole, draft1->p);
		COPY(draft2->pole, draft1->p);
	}
	else
	{
		COPY(draft1->pole, draft2->p);
		COPY(draft2->pole, draft2->p);
	}
	//
	// Start flash flooding
	//
	draft1->flash = SIDE;
	draft2->flash = SIDE;
	//
	if (collapse)
	{
		// Disintegrate the packet
		//
		draft1->code = COLLAPSE;
		draft2->code = COLLAPSE;
	}
}

/*
 * Both re-emmited from respective pole.
 */
__device__ void polepole(Cell* draft1, Cell* draft2)
{
	// Copy momentum information for flash
	//
	COPY(draft1->pole, draft1->p);
	COPY(draft2->pole, draft2->p);
	//
	// Start flash flooding
	//
	draft1->flash = SIDE;
	draft2->flash = SIDE;
}

/*
 * Inertia mechanism.
 */
__device__ void inertia(Cell* draft1, Cell* draft2)
{
	// Copy momentum information for flash
	//
	if (ALIGNED(draft1->o, draft1->p))
	{
		COPY(draft1->pole, draft2->p);
		COPY(draft2->pole, draft2->p);
	}
	else
	{
		COPY(draft1->pole, draft1->p);
		COPY(draft2->pole, draft1->p);
	}
	//
	// Start flash flooding
	//
	draft1->flash = SIDE;
	draft2->flash = SIDE;
}

__device__ void bosonxboson(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
{
	// Same sector?
	//
	if (((stable1->chrg ^ stable2->chrg) & D_MASK) == 0)
	{
		// Gluon x gluon?
		//
		if ((stable1->chrg & C_MASK) != 0 && (stable1->chrg & C_MASK) != C_MASK &&
			(stable2->chrg & C_MASK) != 0 && (stable2->chrg & C_MASK) != C_MASK)
		{
			// Exchange colors, ignoring other charges
			//
			draft1->chrg &= ~C_MASK;
			draft2->chrg &= ~C_MASK;
			draft1->chrg |= stable2->chrg & C_MASK;
			draft2->chrg |= stable1->chrg & C_MASK;
			//
			polepole(draft1, draft2);
		}
		//
		// Neutral 1?
		//
		else if ((stable1->chrg & C_MASK) == 0)
		{
			if((stable1->code & Q_MASK) == 1)
			{
				// Photon 1 or Z 1
				//
				if ((stable1->code & W_MASK) == 1)
				{
					// Photon 1
					//
					if ((stable2->code & W_MASK) == 0 && (stable2->code & Q_MASK) == 0 && (((stable2->chrg>>1) ^ stable2->chrg) & 1) == 0)
					{
						// Photon 1 x W 2
						//
						polepole(draft1, draft2);
						// TODO
					}
				}
				else
				{
					// Z 1
					//
					if ((stable2->code & W_MASK) == 0 && (stable2->code & Q_MASK) == 0 && (((stable2->chrg >> 1) ^ stable2->chrg) & 1) == 0)
					{
						// Z 1 x W 2
						//
						polepole(draft1, draft2);
						// TODO
					}
				}
			}
		}
		else if ((stable2->chrg & C_MASK) == 0)
		{
			if ((stable2->code & Q_MASK) == 1)
			{
				// Photon 2 or Z 2
				//
				if ((stable2->code & W_MASK) == 1)
				{
					// Photon 2
					//
					if ((stable1->code & W_MASK) == 0 && (stable1->code & Q_MASK) == 0 && (((stable1->chrg >> 1) ^ stable1->chrg) & 1) == 0)
					{
						// Photon 2 x W 1
						//
						polepole(draft1, draft2);
						// TODO
					}
				}
				else
				{
					// Z 2
					//
					if ((stable1->code & W_MASK) == 0 && (stable1->code & Q_MASK) == 0 && (((stable1->chrg >> 1) ^ stable1->chrg) & 1) == 0)
					{
						// Z 2 x W 1
						//
						polepole(draft1, draft2);
						// TODO
					}
				}
			}
		}
	}
}

__device__ void fermionxboson(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
{
	// Isolate charge bits
	//
	unsigned c1 = (stable1->chrg & C_MASK) & C_MASK;
	unsigned q1 = ((stable1->chrg & Q_MASK) >> 3) & 1;
	unsigned w1 = ((stable1->chrg & W_MASK) >> 4) & 1;
	unsigned d1 = (stable1->chrg & D_MASK) >> 5;
	unsigned c2 = (stable2->chrg & C_MASK) & C_MASK;
	unsigned q2 = ((stable2->chrg & Q_MASK) >> 3) & 1;
	unsigned w2 = ((stable2->chrg & W_MASK) >> 4) & 1;
	unsigned d2 = (stable2->chrg & D_MASK) >> 5;
	//
	if (d1 != d2)
	{
		// SUPRESSED
		// (Only same sector are allowed to interact in this way)
		//
		return;
	}
	//
	// Quark x gluon?
	//
	if (c1 != NEUTRAL && c1 != ~NEUTRAL && c2 != NEUTRAL && c2 != ~NEUTRAL)
	{
		if (c1 == ~c2)		// TODO why??
		{
			// Blindly exchange colors, ignoring all other charges
			//
			draft1->chrg &= ~C_MASK;
			draft2->chrg &= ~C_MASK;
			draft1->chrg |= c2;
			draft2->chrg |= c1;
			//
			polepole(draft1, draft2);
		}
	}
	//
	// Quark x [photon, Z, W]?
	//
	else if (c1 != NEUTRAL && c1 != ~NEUTRAL)
	{
		// Is it a propeller?
		//
		if (stable1->a == stable2->a)
		{
			// Inertia
			//
			inertia(draft1, draft2);
		}
		else if (q1 == q2)
		{
			polepole(draft1, draft2);
		}
		else
		{
			polepole(draft1, draft2);
		}
	}
	//
	// Electron x [photon, Z, W]?
	//
	else
	{
		// Is it a propeller?
		//
		if (stable1->a == stable2->a)
		{
			// Inertia
			//
			inertia(draft1, draft2);
		}
		else if (q1 == q2)
		{
			if (w1 == w2)
			{
				if (c1 == c2 == 0 && w1 == 1)
				{
					polepole(draft1, draft2);
				}
				else if (c1 == c2 == C_MASK && w1 == 0)
				{
					polepole(draft1, draft2);
				}
			}
		}
		else
		{
			if (w1 == w2)
			{
				if (c1 == c2 == 0 && w1 == 1)
				{
					polepole(draft1, draft2);
				}
				else if (c1 == c2 == C_MASK && w1 == 0)
				{
					polepole(draft1, draft2);
				}
			}
		}
	}
}

__device__ void fermionxfermion(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
{
	// Isolate charge bits
	//
	unsigned c1 = (stable1->chrg & C_MASK) & 7;
	unsigned q1 = ((stable1->chrg & Q_MASK) >> 3) & 1;
	unsigned w1 = ((stable1->chrg & W_MASK) >> 4) & 1;
	unsigned d1 = (stable1->chrg & D_MASK) >> 5;
	unsigned c2 = (stable2->chrg & C_MASK) & 7;
	unsigned q2 = ((stable2->chrg & Q_MASK) >> 3) & 1;
	unsigned w2 = ((stable2->chrg & W_MASK) >> 4) & 1;
	unsigned d2 = (stable2->chrg & D_MASK) >> 5;
	//
	//
	// Matter/antimatter flags
	//
	#ifdef SOL1
	bool matter1 = ((stable1->chrg >> 2) | ((stable1->chrg >> 1) & stable1->chrg)) & 1;
	bool matter1 = ((stable2->chrg >> 2) | ((stable2->chrg >> 1) & stable2->chrg)) & 1;
	#else
	bool matter1 = c1 == 0 || c1 == 1 || c1 == 2 || c1 == 4;
	bool matter2 = c2 == 0 || c2 == 1 || c2 == 2 || c2 == 4;
	#endif
	//
	// Same sector?
	//
	if (d1 == d2)
	{
		// quark x quark?
		//
		if (c1 != NEUTRAL && c1 != ~NEUTRAL && c2 != NEUTRAL && c2 != ~NEUTRAL)
		{
			// Same color and electric charge?
			//
			if (c1 == c2 && q1 == q2)	// TODO: include affinity?? weak charge??
			{
				// Quark cohesion
				//
				polepole(draft1, draft2);
			}
			//
			// Complementary charges?
			//
			else if (c1 == ~c2 && w1 == ~w2 && q1 == ~q2)
			{
				// Quark annihilation?
				//
				cpcp(draft1, draft2, true);
			}
		}
		//
		// quark x electron?
		//
		else if (c1 != NEUTRAL && c1 != ~NEUTRAL)
		{
			if (q1 == q2 && matter1 == matter2)
			{
				// Implement repulsion
			}
			else if (q1 != q2 && matter1 == matter2)
			{
				// Implement attraction
			}
		}
		//
		// Electron x electron
		//
		else if ((c1 == NEUTRAL || c1 == ~NEUTRAL) && c1 == c2)
		{
			if (q1 == q2 && matter1 == matter2)
			{
				// Implement cohesion
				//
				polepole(draft1, draft2);
			}
			else if (q1 != q2 && w1 != w2 && matter1 != matter2)
			{
				// Electron annihilation?
				//
				cpcp(draft1, draft2, true);
			}
		}
	}
	//
	// Different sectors
	//
	else
	{
		bool s1 = (c1 == c2 == 0 && q1 == q2 == 1 && d1 == d2 == 0);
		bool s2 = (c1 == c2 == 7 && q1 == q2 == 0 && d1 == d2 == 1);
		bool s3 = (c1 == c2 != 0 != 7 && q1 == q2);
		//
		bool c1 = (q1 == q2 && w1 == w2 && s1 == s2);
		bool c2 = (q1 != q2 && w1 != w2 && s1 != s2);
		bool c3 = (d1 == 0 && w1 == 0 && w1 != w2);
		bool c4 = (d1 == 1 && w1 == 1 && w1 != w2);
		//
		if ((d1 == 0 && s1 == s2) || (d1 == 1 && s1 == s3 && s1 != s2))
		{
			// Swap colors
			//
			int c1 = stable1->chrg & C_MASK;
			int c2 = stable2->chrg & C_MASK;
			draft1->chrg &= ~C_MASK;
			draft2->chrg &= ~C_MASK;
			draft1->chrg |= c2;
			draft2->chrg |= c1;
			//
			cpcp(draft1, draft2, true);
		}
		//
		// Chiral?
		//
		else if (c1  ||  c2  ||  c3  ||  c4) 
		{
			// Change hands
			//
			int w1 = stable1->chrg & W_MASK;
			int w2 = stable2->chrg & W_MASK;
			draft1->chrg &= ~W_MASK;
			draft2->chrg &= ~W_MASK;
			draft1->chrg |= w2;
			draft2->chrg |= w1;
			//
			polepole(draft1, draft2);
		}
	}
}

/*
 * Compares two cells in adjacent columns. 
 */
__device__ void compareCols(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
{
	// Play pseudo dices against the sine phase
	//
	if (stable1->noise > abs(stable1->v) &&
		stable2->noise > abs(stable1->v) &&
		(!ISNULL(stable1->p) || !ISNULL(stable2->p)))
	{
		// Preserve momentum for parallel transport
		//
		COPY(draft1->pole, stable2->p);
		COPY(draft2->pole, stable1->p);
		//
		if (stable1->code == BOSON && stable2->code == BOSON)
			bosonxboson(stable1, stable2, draft1, draft2);
		else if (stable1->code == BOSON && stable2->code == FERMION)
			fermionxboson(stable2, stable1, draft2, draft1);
		else if (stable1->code == FERMION && stable2->code == BOSON)
			fermionxboson(stable1, stable2, draft1, draft2);
		else
			fermionxfermion(stable1, stable2, draft1, draft2);
	}
}

/*
 * Confronts cells for interactions. 
 */
__global__ void interact(Cell* lattice)
{
	long xyz = blockDim.x * blockIdx.x + threadIdx.x;
	if (xyz < SIDE3)
	{
		Cell* draft = lattice + xyz;
		Cell* stable = draft + SIDE2 * SIDE3;
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
		//	if (stable1->flash)
			//{
				//RESET(draft1->o);	// ????
				//draft1->t = 0;		// ????
		//	}
			//else
			//{
				Cell* stable2 = stable;
				Cell* draft2 = draft;
				for (int j = 0; j < SIDE2; j++)
				{
					if (i != j && stable1->f > 0 && stable2->f > 0 &&
						stable1->a != stable2->a && !ISEQUAL(stable1->o, stable2->o))
						compareCols(stable1, stable2, draft1, draft2);
					//
					stable2 = nextV(stable2);
					draft2 = nextV(draft2);
				}
			//}
			//
			stable1 = nextV(stable1);
			draft1 = nextV(draft1);
		}
	}
}
