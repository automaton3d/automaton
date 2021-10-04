#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "automaton.h"

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
__device__ bool isAllowed(int dir, char vdir[3], char o[3], unsigned char d0)
{
    // Calculate new origin vector
    //
    int x = o[0] + vdir[0];
    int y = o[1] + vdir[1];
    int z = o[2] + vdir[2];
    //
    // Test for expansion
    //
    int d1 = MOD2(o);
    int d2 = x * x + y * y + z * z;
    if (d2 <= d1)
        return false;
    //
    // Wrapping test
    //
    if (x == S + 1 || x == -S || y == S + 1 || y == -S || z ==S + 1 || z == -S)
        return false;
    //
    // Root allows all six directions
    //
    int level = abs(x) + abs(y) + abs(z);
    if (level == 1)
        return true;
    //
    // x axis
    //
    if (x > 0 && y == 0 && z == 0 && dir == 0)
        return true;
    else if (x < 0 && y == 0 && z == 0 && dir == 1)
        return true;
    //
    // y axis
    //
    else if (x == 0 && y > 0 && z == 0 && dir == 2)
        return true;
    else if (x == 0 && y < 0 && z == 0 && dir == 3)
        return true;
    //
    // z axis
    //
    else if (x == 0 && y == 0 && z > 0 && dir == 4)
        return true;
    else if (x == 0 && y == 0 && z < 0 && dir == 5)
        return true;
    //
    // xy plane
    //
    else if (x > 0 && y > 0 && z == 0)
    {
        if (level % 2 == 1)
            return (dir == 0 && d0 == 2);
        else
            return (dir == 2 && d0 == 0);
    }
    else if (x < 0 && y > 0 && z == 0)
    {
        if (level % 2 == 1)
            return (dir == 1 && d0 == 2);
        else
            return (dir == 2 && d0 == 1);
    }
    else if (x > 0 && y < 0 && z == 0)
    {
        if (level % 2 == 1)
            return (dir == 0 && d0 == 3);
        else
            return (dir == 3 && d0 == 0);
    }
    else if (x < 0 && y < 0 && z == 0)
    {
        if (level % 2 == 1)
            return (dir == 1 && d0 == 3);
        else
            return (dir == 3 && d0 == 1);
    }
    //
    // yz plane
    //
    else if (x == 0 && y > 0 && z > 0)
    {
        if (level % 2 == 0)
            return (dir == 4 && d0 == 2);
        else
            return (dir == 2 && d0 == 4);
    }
    else if (x == 0 && y < 0 && z > 0)
    {
        if (level % 2 == 0)
            return (dir == 4 && d0 == 3);
        else
            return (dir == 3 && d0 == 4);
    }
    else if (x == 0 && y > 0 && z < 0)
    {
        if (level % 2 == 0)
            return (dir == 5 && d0 == 2);
        else
            return (dir == 2 && d0 == 5);
    }
    else if (x == 0 && y < 0 && z < 0)
    {
        if (level % 2 == 0)
            return (dir == 5 && d0 == 3);
        else
            return (dir == 3 && d0 == 5);
    }
    //
    // zx plane
    //
    else if (x > 0 && y == 0 && z > 0)
    {
        if (level % 2 == 1)
            return (dir == 4 && d0 == 0);
        else
            return (dir == 0 && d0 == 4);
    }
    else if (x < 0 && y == 0 && z > 0)
    {
        if (level % 2 == 1)
            return (dir == 4 && d0 == 1);
        else
            return (dir == 1 && d0 == 4);
    }
    else if (x > 0 && y == 0 && z < 0)
    {
        if (level % 2 == 1)
            return (dir == 5 && d0 == 0);
        else
            return (dir == 0 && d0 == 5);
    }
    else if (x < 0 && y == 0 && z < 0)
    {
        if (level % 2 == 1)
            return (dir == 5 && d0 == 1);
        else
            return (dir == 1 && d0 == 5);
    }
    else
    {
        // Spirals
        //
        int x0 = x + S;
        int y0 = y + S;
        int z0 = z + S;
        //
        switch (level % 3)
        {
        case 0:
            if (x0 != S && y0 != S)
                return (z0 > S && dir == 4) || (z0 < S && dir == 5);
            break;
        case 1:
            if (y0 != S && z0 != S)
                return (x0 > S && dir == 0) || (x0 < S && dir == 1);
            break;
        case 2:
            if (x0 != S && z0 != S)
                return (y0 > S && dir == 2) || (y0 < S && dir == 3);
            break;
        }
    }
    return false;
}

__device__ Cell* getPointer(int dir, Cell *draft, char* vdir)
{
    Cell* neighbor = draft;
    switch (dir)
    {
    case 0:
        *vdir = +1;
        if (draft->wrap & 0x40)
            neighbor -= (SIDE - 1);
        else
            neighbor++;
        break;
    case 1:
        *vdir = -1;
        if (draft->wrap & 0x80)
            neighbor += (SIDE - 1);
        else
            neighbor--;
        break;
    case 2:
        *(++vdir) = +1;
        if (draft->wrap & 0x10)
            neighbor -= (SIDE2 - SIDE);
        else
            neighbor += SIDE;
        break;
    case 3:
        *(++vdir) = -1;
        if (draft->wrap & 0x20)
            neighbor += (SIDE2 - SIDE);
        else
            neighbor -= SIDE;
        break;
    case 4:
        *(++(++vdir)) = +1;
        if (draft->wrap & 0x04)
            neighbor -= (SIDE3 - SIDE2);
        else
            neighbor += SIDE2;
        break;
    case 5:
        *(++(++vdir)) = -1;
        if (draft->wrap & 0x08)
            neighbor = draft + (SIDE3 - SIDE2);
        else
            neighbor = draft - SIDE2;
        break;
    }
    return neighbor;
}

__device__ void spread(Cell* stable, Cell* draft, int floor)
{
    // Update tracking info
    //
    if (draft->ctrl > 0)
    {
        // Track decay
        //
        draft->phi *= (1 - 1 / (2 * draft->t));
        //
        // Minsky circle algorithm
        //
        int xNew = draft->cosine - (draft->sine >> SHIFT);
        int yNew = draft->sine + (draft->cosine >> SHIFT);
        draft->cosine = xNew;
        draft->sine = yNew;
        //
        draft->ctrl--;
    }
    //
    // Spread cell contents if not empty
    //
    if (draft->f > 0)
    {
        draft->t++; 
        //
        // Re-emmited?
        //
        if (stable->flash && ALIGNED(stable->o, stable->pole))
        {
            RESET(draft->o);
            draft->t = 0;
        }
        //
        // Explore von Neumann directions
        //
        Cell* neighbor;
        for (int dir = 0; dir < 6; dir++)
        {
            char vdir[3] = { 0, 0, 0 };
            neighbor = getPointer(dir, draft, (char*)vdir);
            //
            // Test if branch is legal
            //
            if(isAllowed(dir, vdir, draft->o, draft->dir))
            {
                // Superluminal signal spreads not synchronized
                //
                if (stable->flash)
                {
                    neighbor->flash = stable->flash - 1;
                }
                //
                // Bubble cells spread synchronized
                //
                if (draft->t * draft->t > draft->synch)
                {
                    neighbor->t = draft->t;
                    neighbor->dir = dir;
                    neighbor->f = stable->f;
                    neighbor->b = stable->b;
                    neighbor->charge = stable->charge;
                    //
                    neighbor->o[0] = stable->o[0] + vdir[0];
                    neighbor->o[1] = stable->o[1] + vdir[1];
                    neighbor->o[2] = stable->o[2] + vdir[2];
                    //
                    COPY(neighbor->s, stable->s);
                    COPY(neighbor->p, stable->p);
                    //
                    // Schedule for spherical evolution
                    //
                    neighbor->synch = LIGHT2 * MOD2(neighbor->o);
                    //
                    draft->f = 0;
                    RESET(draft->p);
                }
            }
        }
    }
}

__global__ void expand(Cell* lattice)
{
    long xyz = blockDim.x * blockIdx.x + threadIdx.x;
    if (xyz < SIDE3)
    {
        Cell* draft = lattice + xyz;
        Cell* stable = lattice + xyz + SIDE2 * SIDE3;
        if (draft->active)
        {
            Cell* temp = draft;
            draft = stable;
            stable = temp;
        }
        for (int v = 0; v < SIDE2; v++)
        {
            spread(stable, draft, stable->floor);
            //
            // Next register
            //
            draft = nextV(draft);
            stable = nextV(stable);
        }
    }
}
