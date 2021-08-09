#pragma once

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

struct Cell
{
    bool active;
    unsigned char f;
    int t, b;
    unsigned char q, w, c, d;
    char o[3], p[3], s[3];
    char phi;
    short noise;
    unsigned char code;
    char pole[3];
    unsigned char synch;
    char sine, cosine;
    unsigned char ctrl;
    struct Cell* px, * py, * pz, * nx, * ny, * nz;
    struct Cell* h, * v;
};

// Macros

#define ISNULL(v)		(v[0]==0 && v[1]==0 && v[2]==0)
#define ISEQUAL(v,u)	(v[0]==u[0] && v[1]==u[1] && v[2]==u[2])
#define RESET(v)		v[0]=0;v[1]=0;v[2]=0;
#define COPY(u,v)		u[0]=v[0];u[1]=v[1];u[2]=v[2];

#define CELL            sizeof(struct Cell)

__global__ void commute(struct Cell* lattice);
__global__ void expand(struct Cell* lattice);
__global__ void interact(struct Cell* lattice);
__global__ void compare(struct Cell* lattice);
__global__ void replicate(struct Cell* lattice);
__global__ void hologram(struct Cell* lattice);
__global__ void interop(struct Cell* lattice, vec3* dev_color);

//__global__ void interop(struct Cell* lattice, void* colors);

void animation();
void closeApp();
void updateCamera();

__global__ void expand(struct Cell* lattice);
