#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>

#include "automaton.h"

__device__ void initLattice(int idx, struct Cell* cell, bool active)
{
    int noise = 0;
    for (int z = 0; z < SIDE; z++)
    {
        for (int y = 0; y < SIDE; y++)
        {
            for (int x = 0; x < SIDE; x++)
            {
                cell->active = active;
                cell->t = 0;
                cell->noise = noise;
                cell->f = 0;
                cell->synch = 0;
                RESET(cell->p);
                RESET(cell->s);
                if (z == 0 && (x + SIDE * y) == idx)
                {
                    cell->f = 1;
                    if (x < SIDE / 2)
                    {
                        cell->d = true;
                    }
                    else
                    {
                        cell->d = false;
                    }
                    //
                    unsigned char tiling = (x % 2) ^ (y % 2);
                    if (tiling)
                    {
                        cell->c = 0;
                        cell->w = false;
                        cell->q = true;
                    }
                    else
                    {
                        cell->c = 7;
                        cell->w = true;
                        cell->q = false;
                    }
                    //
                    // Initialize spin and momentum
                    //
                    if (x == SIDE - 1)
                    {
                        cell->s[2] = (cell->d) ? -SIDE / 2 : +SIDE / 2;
                        cell->p[2] = (noise % 2) ? +SIDE / 2 : -SIDE / 2;
                    }
                    else
                    {
                        switch (noise % 6)
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
                cell->sine = 0;
                cell->cosine = SIDE / 2;
                //
                if (x == SIDE - 1)
                    cell->px = cell - (SIDE - 1);
                else
                    cell->px = cell + 1;
                //
                if (x == 0)
                    cell->nx = cell + (SIDE - 1);
                else
                    cell->nx = cell - 1;
                if (y == SIDE - 1)
                    cell->py = cell - (SIDE - 1) * SIDE;
                else
                    cell->py = cell + SIDE;
                if (y == 0)
                    cell->ny = cell + (SIDE - 1);
                else
                    cell->ny = cell - SIDE;
                if (z == SIDE - 1)
                    cell->pz = cell - (SIDE - 1) * SIDE2;
                else
                    cell->pz = cell + SIDE2;
                if (z == 0)
                    cell->nz = cell + (SIDE - 1) * SIDE2;
                else
                    cell->nz = cell - SIDE2;
                if (noise == SIDE2 - 1)
                    cell->v = cell - (SIDE2 - 1) * SIDE3;
                else
                    cell->v = cell + SIDE3;
                //
                // Neighbor
                //
                if(active)
                    cell->h = cell + SIDE3 * SIDE2;
                else
                    cell->h = cell - SIDE3 * SIDE2;
                //
                // Elevator
                //
                if (idx == SIDE2 - 1)
                    cell->v = cell - (SIDE2 - 1) * SIDE3;
                else
                    cell->v = cell + SIDE3;
                //
                noise++;
                cell++;
            }
        }
    }
}

__global__ void hologram(struct Cell* lattice)
{
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < SIDE2)
    {
        struct Cell* cell = lattice + idx * (long long) SIDE3;
        initLattice(idx, cell, true);
        cell += SIDE2 * SIDE3;
        initLattice(idx, cell, false);
    }
}

