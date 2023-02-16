#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "curand.h"
#include "curand_kernel.h"

#include "automaton.cuh"

__device__ curandState state;

__device__ void initCell(Cell* cell, int xyz)
{
    curand_init(0, xyz, 0, &state);
    int x = xyz & (SIDE-1);
    int y = (xyz >> ORDER) & (SIDE - 1);
    int z = (xyz >> (2 * ORDER));
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
    if (cell->floor == SIDE2 - 1)
        cell->wrap |= 0x02;
    if (!cell->active)
        cell->wrap |= 0x01;
    //
    // Initialize simple variables
    //
    cell->t = 0;
    cell->noise = cell->floor;
    cell->f = 0;
    cell->a = cell->floor;
    cell->sync = 0;
    cell->chrg = 0x00;
    cell->flash = 0;
    cell->v = 0;
    cell->u = SIDE2 / 2;
    RESET(cell->p);
    RESET(cell->s);
    RESET(cell->o);
    //
    // Cell belongs to the hologram?
    //
    if (z == SIDE/2 && (x + SIDE * y) == cell->floor)
    {
        // Initialize charges, spin and momentum
        //
        cell->f = 1;
        if (x < SIDE / 2)
        {
            cell->chrg |= D_MASK;
        }
        //
        unsigned char tiling = (x % 2) ^ (y % 2);
        if (tiling)
        {
            cell->chrg |= Q_MASK;
        }
        else
        {
            cell->chrg |= C_MASK | W_MASK;
        }
        //
        // Initialize spin and momentum
        //
        if (x == SIDE - 1)
        {
            // Enforce monopole
            //
            cell->s[2] = (cell->chrg & D_MASK) ? -SIDE / 2 : +SIDE / 2;
            cell->p[2] = (cell->floor % 2) ? +SIDE / 2 : -SIDE / 2;
        }
        else
        {
            // Isotropic distribution
            //
            switch ((x + SIDE*y) % 6)
            {
            case 0:
                cell->p[0] = x - SIDE / 2;
                cell->p[1] = y - SIDE / 2;
                cell->p[2] = SIDE / 2;
                //
                cell->s[0] = y - SIDE / 2;
                cell->s[1] = x - SIDE / 2;
                cell->s[2] = -SIDE / 2;
                break;
            case 1:
                cell->p[0] = SIDE / 2;
                cell->p[1] = x - SIDE / 2;
                cell->p[2] = y - SIDE / 2;
                //
                cell->s[0] = -SIDE / 2;
                cell->s[1] = y - SIDE / 2;
                cell->s[2] = x - SIDE / 2;
                break;
            case 2:
                cell->p[0] = y - SIDE / 2;
                cell->p[1] = SIDE / 2;
                cell->p[2] = x - SIDE / 2;
                //
                cell->s[0] = x - SIDE / 2;
                cell->s[1] = y - SIDE / 2;
                cell->s[2] = SIDE / 2;
                break;
            case 3:
                cell->p[0] = y - SIDE / 2;
                cell->p[1] = x - SIDE / 2;
                cell->p[2] = -SIDE / 2;
                //
                cell->s[0] = x - SIDE / 2;
                cell->s[1] = y - SIDE / 2;
                cell->s[2] = SIDE / 2;
                break;
            case 4:
                cell->p[0] = -SIDE / 2;
                cell->p[1] = y - SIDE / 2;
                cell->p[2] = x - SIDE / 2;
                //
                cell->s[0] = SIDE / 2;
                cell->s[1] = x - SIDE / 2;
                cell->s[2] = y - SIDE / 2;
                break;
            case 5:
                cell->p[0] = x - SIDE / 2;
                cell->p[1] = -SIDE / 2;
                cell->p[2] = y - SIDE / 2;
                //
                cell->s[0] = y - SIDE / 2;
                cell->s[1] = SIDE / 2;
                cell->s[2] = x - SIDE / 2;
                break;
            }
        }
    }
}

__shared__ Cell *mirror;

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
            cell->floor = floor;
            initCell(cell, xyz);
            cell = nextV(cell);
        }
        //
        // Buid the dual lattice
        //
        cell += SIDE2 * SIDE3;
        for (int floor = 0; floor < SIDE2; floor++)
        {
            cell->active = false;
            cell->floor = floor;
            initCell(cell, xyz);
            cell = nextV(cell);
        }
    }
}

