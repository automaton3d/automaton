#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>

#include "automaton.h"

struct Cell* lattice;

__global__ void initCA(struct Cell* lattice)
{
    // Create lattice
    //
    size_t idx = blockIdx.x* blockDim.x + threadIdx.x;
    int h = idx % 2;
    int v = idx >> 1;
    size_t offset = SIDE3*v + SIDE2*SIDE3*h;
    struct Cell* cell = lattice + offset;
    int floor = idx >> 1;
    int i = 0;
    bool active = h == 0;
    for (int z = 0; z < SIDE; z++)
        for (int y = 0; y < SIDE; y++)
            for (int x = 0; x < SIDE; x++)
            {
                cell->active = active;
                cell->t = 0;
                cell->noise = i;
                cell->f = 0;
                RESET(cell->p);
                RESET(cell->s);
                if (z == 0 && (x + SIDE*y) == floor)
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
                    // Initialize spin
                    //
                    if (x == SIDE - 1)
                    {
                        cell->s[2] = (cell->d) ? -SIDE / 2 : +SIDE / 2;
                        cell->p[2] = (i % 2) ? +SIDE/2 : -SIDE/2;
                    }
                    else
                    {
                        switch (i % 6)
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
                if(x == SIDE-1)
                    cell->px = cell - (SIDE - 1) * CELL;
                else
                    cell->px = cell + CELL;
                //
                if(x == 0)
                    cell->nx = cell + (SIDE - 1) * CELL;
                else
                    cell->nx = cell - CELL;
                if(y == SIDE-1)
                    cell->py = cell - (SIDE - 1) * SIDE * CELL;
                else
                    cell->py = cell + SIDE * CELL;
                if(y == 0)
                    cell->ny = cell + (SIDE - 1) * CELL;
                else
                    cell->ny = cell - SIDE * CELL;
                if(z == SIDE-1)
                    cell->pz = cell -(SIDE - 1) * SIDE2 * CELL;
                else
                    cell->pz = cell + SIDE2 * CELL;
                if(z == 0)
                    cell->nz = cell + (SIDE - 1) * SIDE2 * CELL;
                else
                    cell->nz = cell - SIDE2 * CELL;
                if(floor == SIDE2-1)
                    cell->v = cell - (SIDE2 - 1) * SIDE3 * CELL;
                else
                    cell->v = cell + SIDE3 * CELL;
                //
                // Neighbor
                //
                if(cell->active)
                    cell->h = cell + SIDE3 * SIDE2 * CELL;
                else
                    cell->h = cell - SIDE3 * SIDE2 * CELL;
                //
                // Elevator
                //
                if(floor == SIDE2-1)
                    cell->v = cell - (SIDE2 - 1) * SIDE3 * CELL;
                else
                    cell->v = cell + SIDE3 * CELL;
                //
                i++;
                cell++;
            }
}

struct Cell* dev_lattice;

void initApp()
{
    size_t free, total;
    cudaMemGetInfo(&free, &total);
    printf("free=%lld, total=%lld\n", free, total);
    //
    size_t heapsize = 2 * SIDE2 * SIDE3 * sizeof(struct Cell);
    printf("Program launched: %d, %d, %zd, %zd\n", SIDE2, SIDE3, sizeof(struct Cell), heapsize); fflush(stdout);
    cudaError_t cudaStatus;
    cudaStatus = cudaMalloc((void**)&dev_lattice, heapsize);
    if (cudaStatus != cudaSuccess)
    {
        perror("cudaMalloc failed");
        exit(1);
    }

    cudaMemGetInfo(&free, &total);
    printf("free=%lld, total=%lld\n", free, total);



    printf("device lattice allocated\n"); fflush(stdout);
    initCA << <GRIDDIM, BLOCKDIM >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("KERNEL error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    lattice = (struct Cell*)malloc(heapsize);
    if (lattice == NULL)
    {
        perror("host ram unavailable");
        exit(1);
    }
    printf("host lattice allocated\n"); fflush(stdout);
    cudaMemcpy(lattice, dev_lattice, 2 * SIDE2 * SIDE3 * sizeof(struct Cell), cudaMemcpyDeviceToHost);
 }

void closeApp()
{
    cudaFree(dev_lattice);
    cudaDeviceReset();
    free(lattice);
    printf("finished.\n");
}


__global__ void interact()
{
    if (threadIdx.x < SIDE && threadIdx.y < SIDE && blockIdx.x < SIDE)
    {
        // Execute
    }
}

int main()
{
    initApp();
    for (int i = 0; i < 100; i++)
    {
        expand<<<GRIDDIM, BLOCKDIM >>>(lattice);
    }
    struct Cell* cell = lattice;
    for (int h = 0; h < 2; h++)
        for (int v = 0; v < SIDE2; v++)
            for (int z = 0; z < SIDE; z++)
                for (int y = 0; y < SIDE; y++)
                    for (int x = 0; x < SIDE; x++)
                    {
                        if (cell->active && !ISNULL(cell->p))
                        {
                            printf("%d, %d, %d, v=%d, h=%d noise=%d p=(%d,%d,%d)\n", x, y, z, v, h, cell->noise, cell->p[0], cell->p[1], cell->p[2]);
                            fflush(stdout);
                        }
                        cell++;
                    }
    closeApp();
    return 0;
}
