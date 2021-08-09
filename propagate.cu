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

__device__ void calculate(struct Cell* cell)
{
    if (cell->f > 0 || !ISNULL(cell->p))
    {
        if (cell->ctrl > 0)
        {
            // Track decay
            //
            cell->phi *= (1 - 1 / (2 * cell->t));
            //
            // Minsky circle algorithm
            //
            int xNew = cell->cosine - (cell->sine >> SHIFT);
            int yNew = cell->sine + (cell->cosine >> SHIFT);
            cell->cosine = xNew;
            cell->sine = yNew;
            //
            cell->ctrl--;
        }
//        if ((cell->t*cell->t > cell->synch || ISNULL(cell->o)) && !ISNULL(cell->pole))
//        {
            int o2 = cell->o[0] * cell->o[0] + cell->o[1] * cell->o[1] + cell->o[2] * cell->o[2];
            for (int dir = 0; dir < 6; dir++)
            {
                // von Neumann
                //
                int a = 0, b = 0, c = 0;
                struct Cell* neighbor;
                switch (dir)
                {
                case 0: a = +1; neighbor = cell->px; break;
                case 1: a = -1; neighbor = cell->nx; break;
                case 2:	b = +1; neighbor = cell->py; break;
                case 3: b = -1; neighbor = cell->nx; break;
                case 4: c = +1; neighbor = cell->pz; break;
                case 5: c = -1; neighbor = cell->nz; break;
                }
                int mod2 = 
                    (cell->o[0] + a) * (cell->o[0] + a) + 
                    (cell->o[1] + b) * (cell->o[1] + b) + 
                    (cell->o[2] + c) * (cell->o[2] + c);
                if (mod2 >= o2)
                {
                    neighbor->active = false;
                    neighbor->f = cell->f;
                    neighbor->b = cell->b;
                    neighbor->q = cell->q;
                    neighbor->w = cell->w;
                    neighbor->c = cell->c;
                    neighbor->d = cell->d;
                    //
                    neighbor->o[0] = cell->o[0] + a;
                    neighbor->o[1] = cell->o[1] + b;
                    neighbor->o[2] = cell->o[2] + c;
                    //
//                    RESET(neighbor->p);
                    COPY(neighbor->p, cell->p);
                    COPY(neighbor->s, cell->s);
                    //
                    neighbor->synch = LIGHT2 * mod2;
                    //

                    /*
                    if (!ISNULL(cell->p))
                    {
                        bool found = 0;
                        if (abs(cell->p[0]) > abs(cell->p[1]))
                        {
                            if (abs(cell->p[0]) > abs(cell->p[2]))
                            {
                                if (a == signum(cell->p[0])) found = true;
                            }
                            else
                            {
                                if (c == signum(cell->p[2])) found = true;
                            }
                        }
                        else if (abs(cell->p[0]) < abs(cell->p[1]))
                        {
                            if (abs(cell->p[0]) > abs(cell->p[2]))
                            {
                                if (b == signum(cell->p[1])) found = true;
                            }
                            else
                            {
                                if (c == signum(cell->p[2])) found = true;
                            }
                        }
                        else
                        {
                            if (abs(cell->p[0]) > abs(cell->p[2]))
                            {
                                if (a == signum(cell->p[0])) found = true;
                            }
                            else
                            {
                                if (c == signum(cell->p[2])) found = true;
                            }
                        }
                        if (found)
                        {
                            neighbor->pole[0] = cell->pole[0] - a;
                            neighbor->pole[1] = cell->pole[1] - b;
                            neighbor->pole[2] = cell->pole[2] - c;
                            COPY(neighbor->p, cell->p);
                        }
                    }
                    */
                }
  //          }
            //
            // Erase origin cell
            //
            cell->f = 0;
            RESET(cell->p);
        }
    }
    cell->t++;
}

__global__ void expand(struct Cell* lattice)
{
    long idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < SIDE2)
    {
        for (int step = 0; step < LIGHT; step++)
        {
            struct Cell* cell = lattice + idx * (long long)SIDE3;
            if (cell->active)
                cell = cell->h;
            for (int z = 0; z < SIDE; z++)
            {
                for (int y = 0; y < SIDE; y++)
                {
                    for (int x = 0; x < SIDE; x++)
                        calculate(cell++);
                }
            }
        }
    }
}
