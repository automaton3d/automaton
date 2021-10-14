#include <stdio.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "automaton.cuh"
#include "curand.h"
#include "curand_kernel.h"
#include "cglm/vec3.h"

__global__ void interop(Cell* lattice, vec3 *dev_color, int floor)
{
	long id = blockDim.x * blockIdx.x + threadIdx.x;
	if (id < SIDE3)
	{
		curandState state;
		curand_init(0, id, 0, &state);
		bool p = false, flash = false;
		Cell* cell = lattice + id;
		//
		// Calculate voxel color
		//
		if (floor < 0)
		{
			for (int i = 0; i < SIDE2; i++)
			{
				int rnd = curand(&state)& (SIDE2 - 1);
				if (rnd < SIDE/4)
				{
					if (!ISNULL(cell->p))
					{
						floor = i;
						p = true;
						break;
					}
					else if (cell->f > 0)
					{
						floor = i;
						flash = true;
					}
				}
				cell = nextV(cell);
			}
		}
		else
		{
			for (int i = 0; i < floor; i++)
				cell = nextV(cell);
			//
			if (!ISNULL(cell->p))
				p = true;
			else if (cell->flash)
				flash = true;
		}
		//
		// Update voxel color
		//
		float* ptr = (float*)(dev_color + id);
		if (p)
		{
			*ptr++ = 1;
			*ptr++ = 0;
			*ptr = 0;
		}
		else if(flash)
		{
			*ptr++ = 0;
			*ptr++ = 1;
			*ptr = 0;
		}
		else
		{
			*ptr++ = 0.6;
			*ptr++ = 0.6;
			*ptr = 0.8;
		}
	}
}

