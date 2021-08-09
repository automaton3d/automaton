#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "automaton.h"
#include "cglm/vec3.h"

__global__ void interop(struct Cell* lattice, vec3 *dev_color)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;
	if(id < SIDE3)
	{
		struct Cell* cell = lattice + id;
		if (cell->active)
		{
			cell = cell->h;
		}
		bool p = false, f = false;
		//
		// Calculate voxel color
		//



		cell = cell->v;
		cell = cell->v;
		cell = cell->v;
		cell = cell->v;
		cell = cell->v;

		if (!ISNULL(cell->p))
		{
			p = true;
		}
		else if (cell->f > 0)
		{
			f = true;
		}

		/*
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
			cell = cell->v;
		}
		*/
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
			*ptr++ = 0;
			*ptr++ = 1;
			*ptr++ = 0;
		}
	}
}

