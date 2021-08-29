#include <stdio.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "automaton.h"
#include "cglm/vec3.h"

__global__ void interop(Cell* lattice, vec3 *dev_color, int all)
{
	long id = blockDim.x * blockIdx.x + threadIdx.x;
	if (id < SIDE3)
	{
		bool p = false, f = false;
		Cell* cell = lattice + id;
		//
		// Calculate voxel color
		//
		if (all)
		{
			for (int i = 0; i < SIDE2; i++)
			{
				if (!ISNULL(cell->p))
				{
					p = true;
					break;
				}
				else if (cell->f > 0)
				{
					f = true;
				}
				cell = nextV(cell);
			}
		}
		else
		{
			for (int i = 0; i < 150; i++)
				cell = nextV(cell);
			//
			if (!ISNULL(cell->p))
				p = true;
			else if (cell->f > 0)
				f = true;
		}

		//
		// Update voxel color
		//
		float* ptr = (float*)(dev_color + id);
		if (p)
		{
			*ptr++ = 1;
			*ptr++ = 0;
			*ptr++ = 0;
		}
		else if(f)
		{
			*ptr++ = 0;
			*ptr++ = 0;
			*ptr++ = 1;
		}
		else
		{
			*ptr++ = 0.6;
			*ptr++ = 0.6;
			*ptr++ = 0.6;
		}
	}
}

