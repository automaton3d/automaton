#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include "automaton.h"

__device__ int signum(int x)
{
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

__device__ char dirs[6][3] = { { +1, 0, 0 }, { -1, 0, 0 }, { 0, +1, 0 }, { 0, -1, 0}, { 0, 0, +1 }, { 0, 0, -1 } };

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
__device__ bool isAllowed(int dir, char p[3], unsigned char d0)
{
    int d1 = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
    int x = p[0] + dirs[dir][0];
    int y = p[1] + dirs[dir][1];
    int z = p[2] + dirs[dir][2];
    int d2 = x * x + y * y + z * z;
    if (d2 <= d1)
        return false;
    //
    // Wrapping test
    //
    if (x == SIDE / 2 + 1 || x == -SIDE / 2 || y == SIDE / 2 + 1 || y == -SIDE / 2 || z == SIDE / 2 + 1 || z == -SIDE / 2)
        return false;
    //
    // Root
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
        int x0 = x + SIDE / 2;
        int y0 = y + SIDE / 2;
        int z0 = z + SIDE / 2;
        //
        switch (level % 3)
        {
        case 0:
            if (x0 != SIDE / 2 && y0 != SIDE / 2)
                return (z0 > SIDE / 2 && dir == 4) || (z0 < SIDE / 2 && dir == 5);
            break;
        case 1:
            if (y0 != SIDE / 2 && z0 != SIDE / 2)
                return (x0 > SIDE / 2 && dir == 0) || (x0 < SIDE / 2 && dir == 1);
            break;
        case 2:
            if (x0 != SIDE / 2 && z0 != SIDE / 2)
                return (y0 > SIDE / 2 && dir == 2) || (y0 < SIDE / 2 && dir == 3);
            break;
        }
    }
    return false;
}

#define S   (SIDE/2)

__device__ void branch(Cell* active_cell, Cell* passive_cell)
{
    if (passive_cell->ctrl > 0)
    {
        // Track decay
        //
        passive_cell->phi *= (1 - 1 / (2 * passive_cell->t));
        //
        // Minsky circle algorithm
        //
        int xNew = passive_cell->cosine - (passive_cell->sine >> SHIFT);
        int yNew = passive_cell->sine + (passive_cell->cosine >> SHIFT);
        passive_cell->cosine = xNew;
        passive_cell->sine = yNew;
        //
        passive_cell->ctrl--;
    }
    if (active_cell->f > 0 || !ISNULL(active_cell->p))
    {
        if (active_cell->t * active_cell->t > active_cell->synch)
        {
            if (ISNULL(active_cell->pole) && !ISNULL(active_cell->p))
                return;
            int dx = 0, dy = 0, dz = 0;
            Cell* neighbor;
            int difs[6] = { -S, -S, -S, -S, -S, -S, };
            Cell* neighbors[6];
            for (int dir = 0; dir < 6; dir++)
            {
                switch (dir)
                {
                    case 0:
                        if (active_cell->o[0] == S)
                            continue;
                        dx = +1;
                        if (passive_cell->type & 0x40)
                            neighbor = passive_cell - (SIDE - 1);
                        else
                            neighbor = passive_cell + 1;
                        break;
                    case 1:
                        if (active_cell->o[0] == -S)
                            continue;
                        dx = -1;
                        if (passive_cell->type & 0x80)
                            neighbor = passive_cell + (SIDE - 1);
                        else
                            neighbor = passive_cell - 1;
                        break;
                    case 2:
                        if (active_cell->o[1] == S)
                            continue;
                        continue;
                        dy = +1;
                        if (passive_cell->type & 0x10)
                            neighbor = passive_cell - (SIDE2 - SIDE);
                        else
                            neighbor = passive_cell + SIDE;
                        break;
                    case 3:
                        if (active_cell->o[1] == -S)
                            continue;
                        dy = -1;
                        if (passive_cell->type & 0x20)
                            neighbor = passive_cell + (SIDE2 - SIDE);
                        else
                            neighbor = passive_cell - SIDE;
                        break;
                    case 4:
                        if (active_cell->o[2] == S)
                            continue;
                        dz = +1;
                        if (passive_cell->type & 0x04)
                            neighbor = passive_cell - (SIDE3 - SIDE2);
                        else
                            neighbor = passive_cell + SIDE2;
                        break;
                    case 5:
                        if (active_cell->o[2] == -S)
                            continue;
                        dz = -1;
                        if (passive_cell->type & 0x08)
                            neighbor = passive_cell + (SIDE3 - SIDE2);
                        else
                            neighbor = passive_cell - SIDE2;
                        break;
                }
                neighbors[dir] = neighbor;
                //
                // Test if neighbor is virgin
                //
//                unsigned char d0
                if(isAllowed(dir, active_cell->o, 0))
//                if (ISNULL(neighbor->p) && neighbor->f == 0)
                {
                    neighbor->d0 = dir;
                    neighbor->f = active_cell->f;
                    neighbor->b = active_cell->b;
                    neighbor->charge = active_cell->charge;
                    //
                    neighbor->o[0] = active_cell->o[0] + dx;
                    neighbor->o[1] = active_cell->o[1] + dy;
                    neighbor->o[2] = active_cell->o[2] + dz;
                    //
                    int mod2 = neighbor->o[0] * neighbor->o[0] + neighbor->o[1] * neighbor->o[1] + neighbor->o[2] * neighbor->o[2];
                    RESET(neighbor->p);
                    COPY(neighbor->s, active_cell->s);
                    neighbor->synch = LIGHT2 * mod2;
                    //
                    if (!ISNULL(active_cell->p))
                    {
                        int d1 = active_cell->pole[0] * active_cell->pole[0] + active_cell->pole[1] * active_cell->pole[1] + active_cell->pole[2] * active_cell->pole[2];
                        neighbor->pole[0] = active_cell->pole[0] - dx;
                        neighbor->pole[1] = active_cell->pole[1] - dy;
                        neighbor->pole[2] = active_cell->pole[2] - dz;
                        int d2 = neighbor->pole[0] * neighbor->pole[0] + neighbor->pole[1] * neighbor->pole[1] + neighbor->pole[2] * neighbor->pole[2];
                        difs[dir] = d1 - d2;
                    }
                }
            }
            //
            // Transfer momentum
            //
            if (!ISNULL(active_cell->p))
            {
                int max = -S;
                Cell* choice;
                for (int dir = 0; dir < 6; dir++)
                {
                    if (difs[dir] > max)
                    {
                        max = difs[dir];
                        choice = neighbors[dir];
                    }
                }
                COPY(choice->p, active_cell->p);
            }
            //
            passive_cell->f = 0;
            RESET(passive_cell->p);
        }
        active_cell->t++;
        passive_cell->t++;
    }
}

__global__ void expand(Cell* lattice)
{
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < SIDE2)
    {
        for (int step = 0; step < 1; step++)
        {
            Cell* cell = lattice + idx * (long long)SIDE3;
            Cell* active_cube, * passive_cube;
            if (cell->active)
            {
                active_cube = cell;
                passive_cube = cell + SIDE3 * SIDE2;
            }
            else
            {
                passive_cube = cell;
                active_cube = cell + SIDE3 * SIDE2;
            }
            //
            for (int z = 0; z < SIDE; z++)
            {
                for (int y = 0; y < SIDE; y++)
                {
                    for (int x = 0; x < SIDE; x++)
                    {
                        branch(active_cube, passive_cube);
                        active_cube++;
                        passive_cube++;
                    }
                }
            }
        }
    }
}
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdlib.h>
#include <math.h>
#include "automaton.h"

#define S   (SIDE/2)

__device__ int signum(int x)
{
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

/*
 * Tests whether the direction dir is a valid path in the visit-once-tree.
 */
__device__ bool isAllowed(int dir, char vdir[3], short o[3], unsigned char d0)
{
    // Calculate new origin vector
    //
    int d1 = o[0] * o[0] + o[1] * o[1] + o[2] * o[2];
    int x = o[0] + vdir[0];
    int y = o[1] + vdir[1];
    int z = o[2] + vdir[2];
    //
    // Test for expansion
    //
    int d2 = x * x + y * y + z * z;
    if (d2 <= d1)
        return false;
    //
    // Wrapping test
    //
    if (x == S + 1 || x == -S || y == S + 1 || y == -S || z == S + 1 || z == -S)
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

__device__ void branch(Cell* active_cell, Cell* passive_cell)
{
    if (passive_cell->ctrl > 0)
    {
        // Track decay
        //
        passive_cell->phi *= (1 - 1 / (2 * passive_cell->t));
        //
        // Minsky circle algorithm
        //
        int xNew = passive_cell->cosine - (passive_cell->sine >> SHIFT);
        int yNew = passive_cell->sine + (passive_cell->cosine >> SHIFT);
        passive_cell->cosine = xNew;
        passive_cell->sine = yNew;
        //
        passive_cell->ctrl--;
    }
    if (active_cell->f > 0 || !ISNULL(active_cell->p))
    {
        Cell* neighbor;
        int difs[6] = { -S, -S, -S, -S, -S, -S, };
        Cell* neighbors[6];
        if (active_cell->t > active_cell->synch)
        {
            if (ISNULL(active_cell->pole) && !ISNULL(active_cell->p))
              return;
            //
            // Last branch
            //
            unsigned char d0 = active_cell->d0;
            //
            // Explore von Neumann directions
            //
            int difs[6] = { -S, -S, -S, -S, -S, -S, };
            Cell* neighbor;
            Cell* neighbors[6];
            for (int dir = 0; dir < 6; dir++)
            {
                char vdir[3] = { 0, 0, 0 };
                switch (dir)
                {
                    case 0:
                        vdir[0] = +1;
                        if (active_cell->type & 0x40)
                            neighbor = passive_cell - (SIDE - 1);
                        else
                            neighbor = passive_cell + 1;
                        break;
                    case 1:
                        vdir[0] = -1;
                        if (active_cell->type & 0x80)
                            neighbor = passive_cell + (SIDE - 1);
                        else
                            neighbor = passive_cell - 1;
                        break;
                    case 2:
                        vdir[1] = +1;
                        if (active_cell->type & 0x10)
                            neighbor = passive_cell - (SIDE2 - SIDE);
                        else
                            neighbor = passive_cell + SIDE;
                        break;
                    case 3:
                        vdir[1] = -1;
                        if (active_cell->type & 0x20)
                            neighbor = passive_cell + (SIDE2 - SIDE);
                        else
                            neighbor = passive_cell - SIDE;
                        break;
                    case 4:
                        vdir[2] = +1;
                        if (active_cell->type & 0x04)
                            neighbor = passive_cell - (SIDE3 - SIDE2);
                        else
                            neighbor = passive_cell + SIDE2;
                        break;
                    case 5:
                        vdir[2] = -1;
                        if (active_cell->type & 0x08)
                            neighbor = passive_cell + (SIDE3 - SIDE2);
                        else
                            neighbor = passive_cell - SIDE2;
                        break;
                }
                neighbors[dir] = neighbor;
                //
                // Test if neighbor is virgin
                //
                if(isAllowed(dir, vdir, active_cell->o, d0))
                {
                    neighbor->d0 = dir;
                    neighbor->f = active_cell->f;
                    neighbor->b = active_cell->b;
                    neighbor->charge = active_cell->charge;
                    //
                    neighbor->o[0] = active_cell->o[0] + vdir[0];
                    neighbor->o[1] = active_cell->o[1] + vdir[1];
                    neighbor->o[2] = active_cell->o[2] + vdir[2];
                    //
                    int mod2 = neighbor->o[0] * neighbor->o[0] + neighbor->o[1] * neighbor->o[1] + neighbor->o[2] * neighbor->o[2];
                    RESET(neighbor->p);
                    COPY(neighbor->s, active_cell->s);
                    neighbor->synch = LIGHT * sqrt((float)mod2);
                    //
                    if (!ISNULL(active_cell->p))
                    {
                        int d1 = active_cell->pole[0] * active_cell->pole[0] + active_cell->pole[1] * active_cell->pole[1] + active_cell->pole[2] * active_cell->pole[2];
                        neighbor->pole[0] = active_cell->pole[0] - vdir[0];
                        neighbor->pole[1] = active_cell->pole[1] - vdir[1];
                        neighbor->pole[2] = active_cell->pole[2] - vdir[2];
                        int d2 = neighbor->pole[0] * neighbor->pole[0] + neighbor->pole[1] * neighbor->pole[1] + neighbor->pole[2] * neighbor->pole[2];
                        difs[dir] = d1 - d2;
                    }
                }
            }
            //
            // Transfer momentum
            //
            if (!ISNULL(active_cell->p))
            {
                int max = -S;
                Cell* choice;
                for (int dir = 0; dir < 6; dir++)
                {
                    if (difs[dir] > max)
                    {
                        max = difs[dir];
                        choice = neighbors[dir];
                    }
                }
                COPY(choice->p, active_cell->p);
            }
            //
            passive_cell->f = 0;
            RESET(passive_cell->p);
        }
    }
    active_cell->t++;
    passive_cell->t++;
}

__global__ void expand(Cell* lattice)
{
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < SIDE2)
    {
        for (int step = 0; step < LIGHT; step++)
        {
            Cell* cell = lattice + idx * (long long)SIDE3;
            Cell* active_cube, * passive_cube;
            if (cell->active)
            {
                active_cube = cell;
                passive_cube = cell + SIDE3 * SIDE2;
            }
            else
            {
                passive_cube = cell;
                active_cube = cell + SIDE3 * SIDE2;
            }
            //
            for (int z = 0; z < SIDE; z++)
            {
                for (int y = 0; y < SIDE; y++)
                {
                    for (int x = 0; x < SIDE; x++)
                    {
                        branch(active_cube, passive_cube);
                        active_cube++;
                        passive_cube++;
                    }
                }
            }
        }
    }
}
