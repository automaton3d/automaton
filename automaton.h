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
#define SHIFT		(ORDER/2)
#define S           (SIDE/2)

// Physical symbols (used with variable code)

#define PHOTON		1
#define GLUON		2
#define NEUTRINO	3
#define Z			4
#define W			5
#define ELECTRON    6
#define QUARK       7
#define FERMION     (ELECTRON | QUARK | NEUTRINO)
#define BOSON       (PHOTON | GLUON | Z | W)
#define COLLAPSE    0x02

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

// Charge masks

#define C_MASK      0x07
#define Q_MASK      0x08
#define W_MASK      0x10
#define D_MASK      0x20

// Color symbols

#define NEUTRAL     1
#define NEUTRAL_BAR	2

// Automaton cell structure

typedef struct
{
    short t;
    unsigned short floor;
    unsigned char dir;
    unsigned char wrap;
    bool active;
    unsigned char f;
    short b;
    unsigned char charge;
    char o[3], p[3], s[3];
    char phi;
    unsigned char noise;
    unsigned char code;
    unsigned short synch;
    char sine, cosine;
    unsigned char ctrl;

    // Superluminal variables

    unsigned char flash;
    char pole[3];

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

// Kernels

__global__ void commute(Cell* lattice);
__global__ void expand(Cell* lattice);
__global__ void interact(Cell* lattice);
__global__ void compare(Cell* lattice);
__global__ void replicate(Cell* lattice);
__global__ void hologram(Cell* lattice);
__global__ void interop(Cell* lattice, vec3* dev_color, int floor);

// Functions

void animation();
void closeApp();
void updateCamera();
void printResults(bool full);
void display();
void closeApp();
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
