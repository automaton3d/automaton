/*
 * Interaction routines
 * 
 * The code is intentionally left not optimized to enhance
 * where the rules were supressed.
*/
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "curand.h"
#include "curand_kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include "automaton.h"

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
	if (d1 != d2)
	{
		// SUPRESSED
		// Interactions between pairs in different sectors are not allowed
		//
		return;
	}
	//
	// Non-trivial colors?
	//
	if (c1 != NEUTRAL && c1 != NEUTRAL_BAR && c2 != NEUTRAL && c2 != NEUTRAL_BAR)
	{
		// Cohesion of gluons?
		//
		if (c1 == c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		//
		// Complementary colors?
		//
		else if (c1 == ~c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		//
		// Colors are diverse
		//
		else
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		//
		// Swap colors
		// Blindly exchange colors, ignoring all other charges
		//
		int c1 = stable1->charge & C_MASK;
		int c2 = stable2->charge & C_MASK;
		draft1->charge &= ~C_MASK;
		draft2->charge &= ~C_MASK;
		draft1->charge |= c2;
		draft2->charge |= c1;
		//
		polepole(draft1, draft2);
	}
	//
	// Gluon x [photon, Z, W]
	//
	else if (c1 != NEUTRAL && c1 != NEUTRAL_BAR)
	{
		if (c1 == c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		else if (c1 == ~c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		else
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			// SUPRESSED
		}
		// SUPRESSED
		// There is no evidence for gluon interacting with other bosons
	}
	//
	// [photon,Z,W] x [gluon]
	//
	else if (c2 != NEUTRAL && c2 != NEUTRAL_BAR)
	{
		if (c1 == c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		else if (c1 == ~c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		else
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
		}
		// SUPRESSED
		// There is no evidence for gluon interacting with other bosons
	}
	//
	// [photon,Z,W] x [photon,Z,W]
	//
	else
	{
		if (q1 == q2)
		{
			if (w1 == w2)
			{
				// SUPRESSED
			}
			else
			{
				// SUPRESSED
			}
			// SUPRESSED
		}
		else
		{
			if (w1 == w2)
			{
				// SUPRESSED
			}
			else
			{
				// SUPRESSED
			}
			// SUPRESSED
		}
		// SUPRESSED
		// There is no evidence for interaction between these bosons
	}
}

__device__ void fermionxboson(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
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
	if (d1 != d2)
	{
		// SUPRESSED
		// Only bubbles in the same sector are allowed to interact in this way
		//
		return;
	}
	//
	// Quark x gluon?
	//
	if (c1 != NEUTRAL && c1 != NEUTRAL_BAR && c2 != NEUTRAL && c2 != NEUTRAL_BAR)
	{
		if (c1 == c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
				}
				else
				{
				}
			}
			else
			{
				if (w1 == w2)
				{
				}
				else
				{
				}
			}
		}
		else if (c1 == ~c2)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
				}
				else
				{
				}
			}
			else
			{
				if (w1 == w2)
				{
				}
				else
				{
				}
			}
		}
		//
		// Electron x photon
		//
		else
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			//
			// Reissue both from CP
			//
			cpcp(draft1, draft2, true);
		}
	}
	//
	// Quark x [photon, Z, W]?
	//
	else if (c1 != NEUTRAL && c1 != NEUTRAL_BAR)
	{
		if (q1 == q2)
		{
			if (w1 == w2)
			{
			}
			else
			{
			}
		}
		else
		{
			if (w1 == w2)
			{
			}
			else
			{
			}
		}
	}
	//
	// Electron x gluon?
	//
	else if (c2 != NEUTRAL && c2 != NEUTRAL_BAR)
	{
		if (q1 == q2)
		{
			if (w1 == w2)
			{
				// SUPRESSED
			}
			else
			{
				// SUPRESSED
			}
			// SUPRESSED
		}
		else
		{
			if (w1 == w2)
			{
				// SUPRESSED
			}
			else
			{
				// SUPRESSED
			}
			// SUPRESSED
		}
		// SUPRESSED
	}
	//
	// Electron x [photon, Z, W]?
	//
	else
	{
		if (q1 == q2)
		{
			if (w1 == w2)
			{
				if (c1 == c2 == 0 && w1 == 1)
				{
					polepole(draft1, draft2);
				}
				else if (c1 == c2 == 7 && w1 == 0)
				{
					polepole(draft1, draft2);
				}
			}
			else
			{
				// SUPRESSED
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
				else if (c1 == c2 == 7 && w1 == 0)
				{
					polepole(draft1, draft2);
				}
			}
			else
			{
				// SUPRESSED
			}
		}
		//
		// Is it a propeller?
		//
		if (stable1->b == stable2->b)
		{
			// Inertia
			//
			inertia(draft1, draft2);
		}
	}
}

__device__ void fermionxfermion(Cell* stable1, Cell* stable2, Cell* draft1, Cell* draft2)
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
	//
	// Matter/antimatter flags
	//
	bool matter1 = c1 == 0 || c1 == 1 || c1 == 2 || c1 == 4;
	bool matter2 = c2 == 0 || c2 == 1 || c2 == 2 || c2 == 4;
	//
	// Same sector?
	//
	if (d1 == d2)
	{
		// quark x quark?
		//
		if (c1 != NEUTRAL && c1 != NEUTRAL_BAR && c2 != NEUTRAL && c2 != NEUTRAL_BAR)
		{
			if (c1 == c2)
			{
				// Quark cohesion?
				//
				if (q1 == q2)
				{
					if (w1 == w2)
					{
						// SUPRESSED
					}
					else
					{
						// SUPRESSED
					}
					polepole(draft1, draft2);
				}
				//
				// Different electric charge
				//
				else
				{
					if (w1 == w2)
					{
						// SUPRESSED
					}
					else
					{
						// Quark annihilation?
						//
						if (c1 == ~c2)
						{
							cpcp(draft1, draft2, true);
						}
						else
						{
							// SUPRESSED
						}
					}
				}
			}
			//
			// Complementary colors?
			//
			else if (c1 == ~c2)
			{
				if (q1 == q2)
				{
					if (w1 == w2)
					{
						// SUPRESSED
					}
					else
					{
						// SUPRESSED
					}
				}
				//
				// Opposite electric charges
				//
				else
				{
					if (w1 == w2)
					{
						// SUPPRESSED
					}
					//
					// Quark annihilation
					//
					else
					{
						cpcp(draft1, draft2, true);
					}
				}
			}
			//
			// Diverse colors
			//
			else
			{
				if (q1 == q2)
				{
					if (w1 == w2)
					{
						// SUPPRESSED
					}
					else
					{
						// SUPPRESSED
					}
					// SUPPRESSED
				}
				else
				{
					if (w1 == w2)
					{
						// SUPPRESSED
					}
					else
					{
						// SUPPRESSED
					}
					// SUPPRESSED
				}
				int c1 = stable1->charge & C_MASK;
				int c2 = stable2->charge & C_MASK;
				draft1->charge &= ~C_MASK;
				draft2->charge &= ~C_MASK;
				draft1->charge |= c2;
				draft2->charge |= c1;
				cpcp(draft1, draft2, false);
			}
		}
		//
		// quark x electron?
		//
		else if (c1 != NEUTRAL && c1 != NEUTRAL_BAR)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
					// SUPRESSED
				}
				else
				{
					// SUPRESSED
				}
				// SUPRESSED
			}
			else
			{
				if (w1 == w2)
				{
					if (matter1 != matter2)
					{
						polepole(draft1, draft2);
					}
					else
					{
						// SUPRESSED
					}
				}
				else
				{
					if (matter1 == matter2)
					{
						polepole(draft1, draft2);
					}
					else
					{
						// SUPRESSED
					}
				}
			}
		}
		//
		// electron x quark
		//
		else if (c2 != NEUTRAL && c2 != NEUTRAL_BAR)
		{
			if (q1 == q2)
			{
				if (w1 == w2)
				{
				}
				else
				{
				}
			}
			else
			{
				if (w1 == w2)
				{
					if (matter1 == matter2)
					{
						polepole(draft1, draft2);
					}
					else
					{
						// SUPRESSED
					}
				}
				else
				{
					if (matter1 != matter2)
					{
						polepole(draft1, draft2);
					}
					else
					{
						// SUPRESSED
					}
				}
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
			int c1 = stable1->charge & C_MASK;
			int c2 = stable2->charge & C_MASK;
			draft1->charge &= ~C_MASK;
			draft2->charge &= ~C_MASK;
			draft1->charge |= c2;
			draft2->charge |= c1;
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
			int w1 = stable1->charge & W_MASK;
			int w2 = stable2->charge & W_MASK;
			draft1->charge &= ~W_MASK;
			draft2->charge &= ~W_MASK;
			draft1->charge |= w2;
			draft2->charge |= w1;
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
	// Play pseudo dices
	//
	if (stable1->noise > abs(stable1->phi) &&
		stable2->noise > abs(stable2->phi) &&
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
			if (stable1->flash)
			{
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
						compareCols(stable1, stable2, draft1, draft2);
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
