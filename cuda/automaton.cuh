#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "cglm/vec3.h"

// CA symbols

#define ORDER		4           // 268 for the true universe  :)
#define SIDE		(1<<ORDER)
#define MASK		(SIDE-1)
#define DIAG		(2*MASK)
#define LIGHT		(2*DIAG)
#define LIGHT2		(LIGHT*LIGHT)
#define SIDE2		(SIDE*SIDE)
#define SIDE3		(SIDE*SIDE2)
#define SHIFT		(ORDER/2)
#define S           (SIDE/2)
#define K1N         6551
#define K1D         125000
#define K2N         6533
#define K2D         62500

// Physical symbols (used with variable code)

#define PHOTON		0x0010
#define GLUON		0x0020
#define NEUTRINO	0x0040
#define Z			0x0080
#define W			0x0100
#define ELECTRON    6
#define QUARK       7
#define FERMION     (ELECTRON | QUARK | NEUTRINO)
#define BOSON       (PHOTON | GLUON | Z | W)
#define COLLAPSE    0x02

// Color symbols

#define C_MASK      0x07
#define Q_MASK      0x08
#define W_MASK      0x10
#define D_MASK      0x20
#define NEUTRAL     0x00

// Automaton cell structure

typedef struct
{
    bool active;

    // Physical properties

    unsigned chrg;          // charge bits d, c2, c1, c0, w, q
    char p[3], s[3];        // momentum and spin
    unsigned char f;        // frequency
    short a;                // affinity

    // Wavefront

    unsigned t;             // absolute timelife
    unsigned char dir;      // legal path
    char o[3];              // origin vector
    unsigned sync;          // synchronization 
    long u, v;              // harmonic phase

    // Superluminal variables

    unsigned char flash;    // messenger flag
    char pole[3];           // target

    // Lattice boson

    char vp0[3];

    // Auxiliary variables

    unsigned char noise;
    unsigned char code;
    unsigned short floor;
    unsigned char wrap;

} Cell;

// Macros

#define ISNULL(v)		(v[0]==0 && v[1]==0 && v[2]==0)
#define ISEQUAL(v,u)	(v[0]==u[0] && v[1]==u[1] && v[2]==u[2])
#define RESET(v)		{v[0]=0;v[1]=0;v[2]=0;}
#define COPY(u,v)		{u[0]=v[0];u[1]=v[1];u[2]=v[2];}
#define MOD2(v)         (v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
#define nextV(c)        {c->wrap&0x02?c-(SIDE3*(SIDE2-1)):c+SIDE3}
#define ALIGNED(u,v)    (u[1]*v[2]-u[2]*v[1]+u[2]*v[0]-u[0]*v[2]+u[0]*v[1]-u[1]*v[0]==0)

#define FLOOR           173

// GPU symbols

#if ORDER==5
#define GRID       256
#define BLOCK      128
#elif ORDER==4
#define GRID       32
#define BLOCK      128
#else
#define GRID       16
#define BLOCK      32
#endif

// Kernels

__global__ void commute(Cell* lattice);
__global__ void expand(Cell* lattice);
__global__ void interact(Cell* lattice);
__global__ void compare(Cell* lattice);
__global__ void replicate(Cell* lattice);
__global__ void hologram(Cell* lattice);
__global__ void interop(Cell* lattice, vec3* dev_color, int floor);
__global__ void poincare(Cell* lattice, int *count);

// Functions

void animation();
void closeApp();
void updateCamera();
void printResults(bool full);
void display();
void closeApp();
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void debug(int index, Cell* stable, Cell* draft);
