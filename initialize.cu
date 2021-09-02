#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>

#include "automaton.h"

__device__ void initLattice(int idx, Cell* cell, bool active)
{
    for (int z = 0; z < SIDE; z++)
    {
        for (int y = 0; y < SIDE; y++)
        {
            for (int x = 0; x < SIDE; x++)
            {
                cell->type = 0;
                if (z == 0)
                    cell->type |= 0x08;
                else if (z == SIDE - 1)
                    cell->type |= 0x04;
                if (y == 0)
                    cell->type |= 0x20;
                else if (y == SIDE - 1)
                    cell->type |= 0x10;
                if (x == 0)
                    cell->type |= 0x80;
                else if (x == SIDE - 1)
                    cell->type |= 0x40;
                if (idx == SIDE2 - 1)
                    cell->type |= 0x02;
                if (!active)
                    cell->type |= 0x01;
                //
                cell->active = active;
                cell->t = 0;
                cell->noise = idx;
                cell->f = 0;
                cell->b = 0;
                cell->synch = -1;
                cell->charge = 0;
                cell->ctrl = 0;
                RESET(cell->p);
                RESET(cell->s);
                if (z == 0 && (x + SIDE * y) == idx)
                {
                    cell->f = 1;
                    if (x < SIDE / 2)
                    {
                        cell->charge |= D_MASK;
                    }
                    else
                    {
                        cell->charge &= ~D_MASK;
                    }
                    //
                    unsigned char tiling = (x % 2) ^ (y % 2);
                    if (tiling)
                    {
                        cell->charge |= Q_MASK;
                    }
                    else
                    {
                        cell->charge |= C_MASK | W_MASK;
                    }
                    //
                    // Initialize spin and momentum
                    //
                    if (x == SIDE - 1)
                    {
                        cell->s[2] = (cell->charge & D_MASK) ? -SIDE / 2 : +SIDE / 2;
                        cell->p[2] = (idx % 2) ? +SIDE / 2 : -SIDE / 2;
                    }
                    else
                    {
                        switch (idx % 6)
                        {
                        case 0:
                            cell->s[0] = +SIDE / 2;
                            cell->p[1] = +SIDE / 2;
                            break;
                        case 1:
                            cell->s[0] = -SIDE / 2;
                            cell->p[1] = -SIDE / 2;
                            break;
                        case 2:
                            cell->s[1] = +SIDE / 2;
                            cell->p[2] = +SIDE / 2;
                            break;
                        case 3:
                            cell->s[1] = -SIDE / 2;
                            cell->p[2] = -SIDE / 2;
                            break;
                        case 4:
                            cell->s[2] = +SIDE / 2;
                            cell->p[0] = +SIDE / 2;
                            break;
                        case 5:
                            cell->s[2] = -SIDE / 2;
                            cell->p[0] = -SIDE / 2;
                            break;
                        }
                    }
                }
                //
                COPY(cell->pole, cell->p);
                RESET(cell->o);
                cell->z = z;
                cell->sine = 0;
                cell->cosine = SIDE / 2;
                //
                cell++;
            }
        }
    }
}

__global__ void hologram(Cell* lattice)
{
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < SIDE2)
    {
        Cell* cell = lattice + idx * (long long) SIDE3;
        initLattice(idx, cell, true);
        cell += SIDE2 * SIDE3;
        initLattice(idx, cell, false);
    }
}

