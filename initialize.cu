#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include "curand.h"
#include "curand_kernel.h"

#include "automaton.h"

__device__ curandState state;

__device__ void initCell(Cell* cell, int floor, int xyz)
{
    curand_init(0, xyz, 0, &state);
    int x = xyz & (SIDE-1);
    int y = (xyz >> ORDER) & (SIDE - 1);
    int z = (xyz >> (2 * ORDER));
    cell->floor = floor;                        // DEBUG
    //
    // Variable wrap is a hint for wrapping in other kernels
    //
    cell->wrap = 0;
    if (z == 0)
        cell->wrap |= 0x08;
    else if (z == SIDE - 1)
        cell->wrap |= 0x04;
    if (y == 0)
        cell->wrap |= 0x20;
    else if (y == SIDE - 1)
        cell->wrap |= 0x10;
    if (x == 0)
        cell->wrap |= 0x80;
    else if (x == SIDE - 1)
        cell->wrap |= 0x40;
    if (floor == SIDE2 - 1)
        cell->wrap |= 0x02;
    if (!cell->active)
        cell->wrap |= 0x01;
    //
    cell->t = 0;
    cell->noise = floor;
    cell->f = 0;
    cell->b = 0;
    cell->synch = -1;
    cell->charge = 0;
    cell->ctrl = 0;
    RESET(cell->p);
    RESET(cell->s);
    if (z == SIDE/2 && (x + SIDE * y) == floor)
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
            cell->p[2] = (floor % 2) ? +SIDE / 2 : -SIDE / 2;
        }
        else
        {
            /*
            switch (floor % 6)
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
            */
            switch (curand(&state) % 3)
            {
            case 0: // xy
                cell->s[0] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[0] *= curand(&state) % SIDE/2;
                cell->s[1] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[1] *= curand(&state) % SIDE / 2;
                cell->s[2] = curand(&state) % 2 == 0 ? SIDE / 2 : -SIDE / 2;
                break;
            case 1: // yz
                cell->s[0] = curand(&state) % 2 == 0 ? SIDE / 2 : -SIDE / 2;
                cell->s[1] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[1] *= curand(&state) % SIDE / 2;
                cell->s[2] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[2] *= curand(&state) % SIDE / 2;
                break;
            case 2: // zx
                cell->s[0] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[0] *= curand(&state) % SIDE / 2;
                cell->s[1] = curand(&state) % 2 == 0 ? SIDE / 2 : -SIDE / 2;
                cell->s[2] = curand(&state) % 2 == 0 ? +1 : -1;
                cell->s[2] *= curand(&state) % SIDE / 2;
                break;
            }
            cell->p[0] = -cell->s[0];
            cell->p[1] = -cell->s[1];
            cell->p[2] = -cell->s[2];
        }
    }
    //
    RESET(cell->o);
    cell->flash = false;
    cell->sine = 0;
    cell->cosine = SIDE / 2;
}

__global__ void hologram(Cell* lattice)
{
    // Calculate 3d index
    //
    long xyz = blockDim.x * blockIdx.x + threadIdx.x;
    if (xyz < SIDE3)
    {
        // Build one lattice
        //
        Cell* cell = lattice + xyz;
        for (int floor = 0; floor < SIDE2; floor++)
        {
            cell->active = true;
            initCell(cell, floor, xyz);
            cell = nextV(cell);
        }
        //
        // Buid the dual lattice
        //
        cell += SIDE2 * SIDE3;
        for (int floor = 0; floor < SIDE2; floor++)
        {
            cell->active = false;
            initCell(cell, floor, xyz);
            cell = nextV(cell);
        }
    }
}

