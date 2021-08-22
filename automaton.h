#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "cglm/vec3.h"

// CA symbols

#define ORDER		4           // 212 for the true universe
#define SIDE		(1<<ORDER)
#define MASK		(SIDE-1)
#define DIAG		(2*MASK)
#define LIGHT		(2*DIAG)
#define LIGHT2		(LIGHT*LIGHT)
#define SIDE2		(SIDE*SIDE)
#define SIDE3		(SIDE*SIDE2)
#define TMAX		((4*SIDE2+3)*LIGHT)
#define TMED		((TMAX-2*SIDE2)*LIGHT)
#define SHIFT		(ORDER/2)
#define PHOTON		1
#define GLUON		2
#define NEUTRINO	3
#define Z			4
#define W			5

// GPU symbols

#define GRID1       4
#define BLOCK1      64
#define GRID2       16
#define BLOCK2      (32*8)

// Charge masks

#define C_MASK      0x07
#define Q_MASK      0x08
#define W_MASK      0x10
#define D_MASK      0x20

// Automaton cell structure

typedef struct
{
    unsigned char d0;
    char z;  // debug
    unsigned char type;
    bool active;
    unsigned char f;
    int t, b;
    unsigned char charge;
    char o[3], p[3], s[3];
    char phi;
    short noise;
    unsigned char code;
    char pole[3];
    short synch;
    char sine, cosine;
    unsigned char ctrl;
} Cell;

// Macros

#define ISNULL(v)		(v[0]==0 && v[1]==0 && v[2]==0)
#define ISEQUAL(v,u)	(v[0]==u[0] && v[1]==u[1] && v[2]==u[2])
#define RESET(v)		v[0]=0;v[1]=0;v[2]=0;
#define COPY(u,v)		u[0]=v[0];u[1]=v[1];u[2]=v[2];
#define nextV(c)        c->type&0x02?c-(SIDE3*(SIDE2-1)):c+SIDE3
#define CELL            sizeof(Cell)

// Kernels

__global__ void commute(Cell* lattice);
__global__ void expand(Cell* lattice);
__global__ void interact(Cell* lattice);
__global__ void compare(Cell* lattice);
__global__ void replicate(Cell* lattice);
__global__ void hologram(Cell* lattice);
__global__ void interop(Cell* lattice, vec3* dev_color, int all);
__device__ bool isAllowed(int dir, char p[3], unsigned char d0);

// Functions

void animation();
void closeApp();
void updateCamera();
void printResults();
#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "cglm/vec3.h"

// CA symbols

#define ORDER		4           // 212 for the true universe
#define SIDE		(1<<ORDER)
#define MASK		(SIDE-1)
#define DIAG		(2*MASK)
#define LIGHT		(2*DIAG)
#define LIGHT2		(LIGHT*LIGHT)
#define SIDE2		(SIDE*SIDE)
#define SIDE3		(SIDE*SIDE2)
#define TMAX		((4*SIDE2+3)*LIGHT)
#define TMED		((TMAX-2*SIDE2)*LIGHT)
#define SHIFT		(ORDER/2)
#define PHOTON		1
#define GLUON		2
#define NEUTRINO	3
#define Z			4
#define W			5

// GPU symbols

#define GRID1       4
#define BLOCK1      64
#define GRID2       16
#define BLOCK2      (32*8)

// Charge masks

#define C_MASK      0x07
#define Q_MASK      0x08
#define W_MASK      0x10
#define D_MASK      0x20

// Automaton cell structure

typedef struct
{
    unsigned char d0;
    char z;  // debug
    unsigned char type;
    bool active;
    unsigned char f;
    int t, b;
    unsigned char charge;
    short o[3], p[3], s[3];
    char phi;
    short noise;
    unsigned char code;
    short pole[3];
    int synch;
    char sine, cosine;
    unsigned char ctrl;
} Cell;

// Macros

#define ISNULL(v)		(v[0]==0 && v[1]==0 && v[2]==0)
#define ISEQUAL(v,u)	(v[0]==u[0] && v[1]==u[1] && v[2]==u[2])
#define RESET(v)		v[0]=0;v[1]=0;v[2]=0;
#define COPY(u,v)		u[0]=v[0];u[1]=v[1];u[2]=v[2];
#define nextV(c)        c->type&0x02?c-(SIDE3*(SIDE2-1)):c+SIDE3
#define CELL            sizeof(Cell)

// Kernels

__global__ void commute(Cell* lattice);
__global__ void expand(Cell* lattice);
__global__ void interact(Cell* lattice);
__global__ void compare(Cell* lattice);
__global__ void replicate(Cell* lattice);
__global__ void hologram(Cell* lattice);
__global__ void interop(Cell* lattice, vec3* dev_color, int all);
__device__ bool isAllowed(int dir, short p[3], unsigned char d0);

// Functions

void animation();
void closeApp();
void updateCamera();
void printResults(bool full);
