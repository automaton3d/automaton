/*
 * init.cu
 */
#include <stdlib.h>
#include <assert.h>
#include "init.h"
#include "common.h"
#include "automaton.h"
#include "params.h"
#include "tuple.h"
#include "plot3d.h"
#include "brick.h"
#include "utils.h"
#include "scenarios.h"

pthread_mutex_t cam_mutex = PTHREAD_MUTEX_INITIALIZER;

__device__ int d_prime;

/*
 * Initializes sine wave parameters.
 */
void initSineWave()
{
	double wT = 2 * M_PI / SIDE;
	double k = 2 * cos(wT);
	double u1 = SIDE * sin(-2 * wT);
	double u2 = SIDE * sin(-wT);
	cudaMemcpyToSymbol(&K, &k, sizeof(k), 0, cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(&U1, &u1, sizeof(u1), 0, cudaMemcpyHostToDevice);
	cudaMemcpyToSymbol(&U2, &u2, sizeof(u2), 0, cudaMemcpyHostToDevice);
}

__global__ void buildGrid(Brick *t0)
{
        int x = blockDim.x * blockIdx.x + threadIdx.x;
        int y = blockDim.y * blockIdx.y + threadIdx.y;
        int z = blockDim.z * blockIdx.z + threadIdx.z;
	int offset = SIDE2*x + SIDE*y + z;
	if(offset >= SIDE3)
		return;
        Brick *t = t0 + offset*NPREONS;
        for(int w = 0; w < NPREONS; w++, t++)
        {
                memset(t, 0, sizeof(Brick));
		t->p0.x = x;
		t->p0.y = y;
		t->p0.z = z;
                t->p19 = w;
        }
}

/*
 * Inserts a preon in a specified address of the pri0 lattice.
 *
 * @b	brick
 * @p4	spin
 * @p6	electric charge
 * @p7	chirality
 * @p8  gravity
 * @p9	color
 * @p21 status
 * @p24 schedule
 * @p25	messenger
 */
__global__ void addPreon(Brick *t, Brick *t0, Tuple pos, int w, Tuple p4, char p5, char p6, char p7, int p8, 
unsigned char p9, int p21, unsigned p24, int p25)
{
	Tuple xyz;
        xyz.x = blockDim.x * blockIdx.x + threadIdx.x;
        xyz.y = blockDim.y * blockIdx.y + threadIdx.y;
        xyz.z = blockDim.z * blockIdx.z + threadIdx.z;
        int offset = SIDE2*xyz.x + SIDE*xyz.y + xyz.z;
	if(offset < SIDE3 && isEqual(xyz, pos))
	{
		t = t0 + offset*NPREONS + w;
		cleanBrick(t);
		t->p4 = p4;
		t->p5 = p5;
		t->p6 = p6;
		t->p7 = p7;
		t->p8 = p8;
		t->p9 = p9;
		t->p15.x = -1;
		t->p21 = p21;
		t->p24 = p24;
		t->p25 = p25;
	}
}

/*
 * Initializes the automaton program.
 */
void initAutomaton()
{
	initSineWave();
	size_t size = NCELLS * sizeof(Brick);
	//
	// Init principal lattice
	//
	cudaMalloc(&d_pri0, size);
	buildGrid<<<gridDim,blockDim>>>(d_pri0);
	//
	int limit = floor(sqrt(3) * (1 << (ORDER - 1)));
	cudaMemcpyToSymbol(d_limit, &limit, sizeof(limit));
	//
	int prime = getPrime(SIDE);
	cudaMemcpyToSymbol(d_prime, &prime, sizeof(prime));
	//
	b = (Brick *)malloc(sizeof(Brick));
	//
	// Initial state of the universe
	//
	assert(scenario>=0);
	switch(scenario)
	{
		case 0:
			BurstScenario();
			break;
		case 1:
			UScenario();
			break;
		case 2:
			VacuumScenario();
			break;
		case 3:
			NoUXUScenario();
			break;
		case 4:
			UXUScenario();
			break;
		case 5:
			GRAVScenario();
			break;
		case 6:
			LonePScenario();
			break;
		case 7:
			BigBangScenario();
			break;
	}
	//
	// Init dual lattice
	//
	cudaMalloc(&d_dual0, size);
	buildGrid<<<gridDim,blockDim>>>(d_dual0);
}


