#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include <stdint.h>
//#include <iostream>

#include "automaton.cuh"

__device__ bool compareCells(Cell* cell, int xyz)
{
    int x = xyz & (SIDE - 1);
    int y = (xyz >> ORDER) & (SIDE - 1);
    int z = (xyz >> (2 * ORDER));
    //
    /*
    if (cell->t != 0)
        return false;
    if(cell->noise != cell->floor)
        return false;
//    cell->b = 0;
    if (cell->synch != 0)
        return false;
//    cell->ctrl = 0;
//    cell->flash = 0;
    if(cell->sine != 0)
        return false;
    if(cell->cosine != SIDE / 2)
        return false;
    */
    if(!ISNULL(cell->o))
        return false;
    //
    // Cell belongs to the hologram?
    //
    /*
    if (z == SIDE / 2 && (x + SIDE * y) == cell->floor)
    {
        // Initialize charges, spin and momentum
        //
        if(cell->f != 1)
            return false;
        if (x < SIDE / 2 && (cell->charge & D_MASK) == 0)
            return false;
        //
        unsigned char tiling = (x % 2) ^ (y % 2);
        if (tiling)
        {
            if ((cell->charge & Q_MASK) == 0)
                return false;
        }
        else
        {
            if ((cell->charge & C_MASK) != C_MASK)
                return false;
            if ((cell->charge & W_MASK) == 0)
                return false;
        }
        //
        // Test spin and momentum
        //
        char s[3] = { 0,0,0 }, p[3] = { 0,0,0 };
        if (x == SIDE - 1)
        {
            // Enforce monopole
            //
            s[2] = (cell->charge & D_MASK) ? -SIDE / 2 : +SIDE / 2;
            p[2] = (cell->floor % 2) ? +SIDE / 2 : -SIDE / 2;
            if (cell->s[2] != s[2])
                return false;
            if (cell->p[2] != p[2])
                return false;
        }
        else
        {
            // Isotropic distribution
            //
            switch ((x + SIDE * y) % 6)
            {
            case 0:
                p[0] = x - SIDE / 2;
                p[1] = y - SIDE / 2;
                p[2] = SIDE / 2;
                //
                s[0] = y - SIDE / 2;
                s[1] = x - SIDE / 2;
                s[2] = -SIDE / 2;
                break;
            case 1:
                p[0] = SIDE / 2;
                p[1] = x - SIDE / 2;
                p[2] = y - SIDE / 2;
                //
                s[0] = -SIDE / 2;
                s[1] = y - SIDE / 2;
                s[2] = x - SIDE / 2;
                break;
            case 2:
                p[0] = y - SIDE / 2;
                p[1] = SIDE / 2;
                p[2] = x - SIDE / 2;
                //
                s[0] = x - SIDE / 2;
                s[1] = y - SIDE / 2;
                s[2] = SIDE / 2;
                break;
            case 3:
                p[0] = y - SIDE / 2;
                p[1] = x - SIDE / 2;
                p[2] = -SIDE / 2;
                //
                s[0] = x - SIDE / 2;
                s[1] = y - SIDE / 2;
                s[2] = SIDE / 2;
                break;
            case 4:
                p[0] = -SIDE / 2;
                p[1] = y - SIDE / 2;
                p[2] = x - SIDE / 2;
                //
                s[0] = SIDE / 2;
                s[1] = x - SIDE / 2;
                s[2] = y - SIDE / 2;
                break;
            case 5:
                p[0] = x - SIDE / 2;
                p[1] = -SIDE / 2;
                p[2] = y - SIDE / 2;
                //
                s[0] = y - SIDE / 2;
                s[1] = SIDE / 2;
                s[2] = x - SIDE / 2;
                break;
            }
            if (cell->s[0] != s[0])
                return false;
            if (cell->s[1] != s[1])
                return false;
            if (cell->s[2] != s[2])
                return false;
            if (cell->p[0] != p[0])
                return false;
            if (cell->p[1] != p[1])
                return false;
            if (cell->p[2] != p[2])
                return false;
        }
    }
    else
    {
        // Does not belong to the hologram
        //
        if (!ISNULL(cell->p))
            return false;
        if (!ISNULL(cell->s))
            return false;
        if(cell->charge != 0)
            return false;
        if (cell->f != 0)
            return false;
    }
    */
    return true;
}

__global__ void poincare(Cell* lattice, int *count)
{
    // Calculate 3d index
    //
    bool test = false;
    long xyz = blockDim.x * blockIdx.x + threadIdx.x;
    if (xyz < SIDE3)
    {
        // Build one lattice
        //
        Cell* cell = lattice + xyz;
        for (int floor = 0; floor < SIDE2; floor++)
        {
            if (!compareCells(cell, xyz))
            {
                test = true; 
                break;
            }
            cell = nextV(cell);
        }
        if (test)
        {
            atomicAdd(count, 1);
        }
    }
}

