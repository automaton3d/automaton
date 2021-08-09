#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__global__ void interact(struct Cell* lattice)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if (id < SIDE3)
	{
		struct Cell* cell = lattice + id;
		int sig1 = cell->h->c == cell->c && cell->c == 0 && cell->h->q == cell->q && cell->q == 1 && cell->h->d == cell->d && cell->d == 0;
		int sig2 = cell->h->c == cell->c && cell->c == 7 && cell->h->q == cell->q && cell->q == 0 && cell->h->d == cell->d && cell->d == 1;
		int sig3 = cell->h->c == cell->c && cell->c != 0 && cell->c != 7 && cell->h->q == cell->q;
		//
		int c1 = cell->h->q == cell->q && cell->h->w == cell->w && sig1 == sig2;
		int c2 = cell->h->q != cell->q && cell->h->w != cell->w && sig1 != sig2;
		int c3 = cell->h->d == 0 && cell->h->w == 0 && cell->h->w != cell->w;
		int c4 = cell->h->d == 1 && cell->h->w == 1 && cell->h->w != cell->w;
		//
		// Play pseudo dices
		//
		if (cell->h->noise > abs(cell->h->phi) && cell->noise > abs(cell->phi) && (!ISNULL(cell->h->p) || !ISNULL(cell->p)))
		{
			if (cell->h->d == cell->d)
			{
				// Same sector?
				//
				if (cell->h->f == 1 && cell->f == 1)
				{
					// F x F
					//
					if (cell->h->q != cell->q)
					{
						// Annihilation?
						//
						cell->h->b = (cell->h->b * cell->b) % SIDE2;
						cell->b = cell->h->b;
						//
						// Reissue R1 and R2 from this
						//
						RESET(cell->h->pole);
						RESET(cell->pole);
					}
					else if (sig1 || sig2 || sig3)
					{
						// Similar?
						//
						// Cohesion
						//
						if (cell->h->b != cell->b)
						{
							cell->h->b = (cell->h->b * cell->b) % SIDE2;
							cell->b = cell->h->b;
						}
						//
						// s1 <-> s2
						//
						int temp;
						temp = cell->h->s[0];
						cell->s[0] = cell->h->s[0];
						cell->h->s[0] = temp;
						temp = cell->h->s[1];
						cell->s[1] = cell->h->s[1];
						cell->h->s[1] = temp;
						temp = cell->h->s[2];
						cell->s[2] = cell->h->s[2];
						cell->h->s[2] = temp;
						//
						// Reissue R1 from pole(R1) and R2 from pole(R2)
						//
						RESET(cell->h->o);
						RESET(cell->o);
					}
				}
				else if (cell->h->f > 1 && cell->f > 1)
				{
					// B x B
					//
					if (cell->h->c == ~cell->c && cell->h->code == cell->code && cell->code == GLUON)
					{
						// gluon-gluon?
						//
						// Swap colors
						//
						int temp = cell->h->c;
						cell->h->c = cell->c;
						cell->c = temp;
						//
						// Reissue R1 from pole(R1)
						//
						RESET(cell->h->o);
					}
					else if (!ISNULL(cell->h->p) && !ISNULL(cell->p))
					{
						if (c1 || c2 || c3 || c4)
						{
							// chiral?
							//
							// Reissue R1 and R2 from cp1
							//
							cell->b = cell->h->b;
							RESET(cell->h->o);
							//
							// Reissue R2 from cp1
							//
							RESET(cell->o);
						}
						else
						{
							// TODO
						}
					}
					else if (sig1 != 0 && sig1 != 3 && sig2 != 0 && sig2 != 3)
					{
						int temp = cell->h->c;
						cell->h->c = cell->c;
						cell->c = temp;
						cell->b = cell->h->b;
						//
						// Reissue R1 and R2 from cp1
						//
						cell->b = cell->h->b;
						RESET(cell->h->o);
						RESET(cell->o);
					}
				}
				if (cell->h->f == 1 && cell->f > 1)
				{
					// F x B
					//
					if (cell->h->c != 0 && cell->h->c != 7 && cell->c != 0 && cell->c != 7)
					{
						int temp = cell->h->c;
						cell->h->c = cell->c;
						cell->c = temp;
						cell->b = cell->h->b;
						//
						// Reissue R1 from pole(R1) and R2 from pole(R2)
						//
						RESET(cell->h->o);
						RESET(cell->o);
					}
					else
					{
						cell->b = cell->h->b;
						//
						// Reissue R1 and R2 from this
						//
						RESET(cell->h->pole);
						RESET(cell->h->o);
						RESET(cell->pole);
						RESET(cell->o);
					}
				}
				else if (cell->h->f > 1 && cell->f == 1)
				{
					//
					// B x F
					//
					if (cell->h->c != 0 && cell->h->c != 7 && cell->c != 0 && cell->c != 7)
					{
						int temp = cell->h->c;
						cell->h->c = cell->c;
						cell->c = temp;
						cell->b = cell->h->b;
						//
						// Reissue R1 from pole(R1) and R2 from pole(R2)
						//
						RESET(cell->h->o);
						RESET(cell->o);
					}
					else
					{
						cell->b = cell->h->b;
						//
						// Reissue R1 and R2 from this
						//
						RESET(cell->h->pole);
						RESET(cell->pole);
					}
				}
				else if (cell->h->b == cell->b)
				{
					// Messenger interactions
					//
					if (!ISNULL(cell->h->p))
					{
						// REISSUE(cell->h, POLE(cell->h))
						//
						RESET(cell->h->pole);
						//
						// REISSUE(cell, TRANSPORT(cell, cell->h));
						//
						cell->pole[0] = cell->h->o[0] - cell->o[0];
						cell->pole[1] = cell->h->o[1] - cell->o[1];
						cell->pole[2] = cell->h->o[2] - cell->o[2];
					}
					else
					{
						// REISSUE(cell, POLE(cell));
						//
						RESET(cell->pole);
						//
						// REISSUE(cell->h, TRANSPORT(cell->h, cell));
						//
						cell->h->pole[0] = cell->o[0] - cell->h->o[0];
						cell->h->pole[1] = cell->o[1] - cell->h->o[1];
						cell->h->pole[2] = cell->o[2] - cell->h->o[2];
					}
				}
			}
			else
			{
				// Inter-sector
				//
				if (((cell->h->d == 0 && sig1 == 2) || (cell->h->d == 1 && sig1 == 3)))
				{
					int temp = cell->h->c;
					cell->h->c = cell->c;
					cell->c = temp;
					//
					// Reissue R1 and R2 from this
					//
					RESET(cell->h->pole);
					RESET(cell->pole);
				}
				else if (c1 || c2 || c3 || c4)
				{
					// Chiral?
					//
					int temp = cell->h->w;
					cell->h->w = cell->w;
					cell->w = temp;
					//
					// Reissue R1 and R2 from this
					//
					RESET(cell->h->pole);
					RESET(cell->pole);
				}
			}
		}
	}
	/*
	else {
		if ((cell->t / LIGHT) % 2 == 1 && cell->t > 1 && cell->t < TMAX) {
			//
			// Shift 'vertically'
			//
			cell->h->h = cell->h->v->h;
			cell->h->v = cell->h->v->v;
		}
	}
	//
	// Update time counter
	//
	if (cell->t <= LIGHT)
		cell->t--;
	else
		cell->t -= LIGHT;
	if (cell->t == 0) {
		cell->t = TMAX;
		cell->active = !cell->active;
	}
	*/
}
