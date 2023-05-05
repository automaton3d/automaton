/*
 * automaton.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <pthread.h>
#include <stdint.h>
#include "tuple.h"
#include "vec3.h"

// Lattice symbols

#define ORDER    3  //202 (see Section 2.3)
#define SIDE     (1<<ORDER)
#define SIDE2    (SIDE*SIDE)
#define SIDE3    (SIDE*SIDE2)
#define SIDE4    (SIDE*SIDE3)
#define SIDE5    (SIDE*SIDE4)
#define SIDE6    (SIDE*SIDE5)
#define SIDE_2   (SIDE/2)
#define MASK     (SIDE-1)
#define DIAG     (2*MASK)
#define LIGHT    (2*DIAG)
#define LIGHT2   (LIGHT*LIGHT)

// Particle singles and pairs (used in k)

#define EMPTY    0x0000
#define COLLAPSE 0x0001
#define FERMION  0x0002
#define SPHOTON  0x0004	// super photon
#define PHOTON   0x0008
#define GLUON    0x0010
#define NEUTRINO 0x0020
#define ZB       0x0040
#define WB       0x0080
#define UP       0x0100
#define DOWN     0x0200
#define FOWLS    0x0400 // Orbis x Dark Sector

// Charge masks

#define C_MASK   0x07
#define Q_MASK   0x08
#define W0_MASK  0x10
#define W1_MASK  0x20

// Handedness

#define RIGHT 0
#define LEFT  1

// Macros (helps readability)

#define ZERO(v)      (v[0]==0&&v[1]==0&&v[2]==0)
#define EQ(v,u)      (v[0]==u[0]&&v[1]==u[1]&&v[2]==u[2])
#define ISSAT(v)     (v[0]==SIDE&&v[1]==SIDE&&v[2]==SIDE)
#define ISMILD(v)    (abs(v[0])==SIDE_2&&abs(v[1])==SIDE_2&&abs(v[2])==SIDE_2)
#define C(u)         (u->ch&C_MASK)      // color
#define _C(u)        ((~u->ch&C_MASK)&7) // anticolor
#define W1(u)        ((u->ch&W1_MASK)==W1_MASK)
#define W0(u)        ((u->ch&W0_MASK)==W0_MASK)
#define Q(u)         ((u->ch&Q_MASK)==Q_MASK)
#define MAT(u)       (C(u)>2&&C(u)!=4)
#define CMPL(u,v)    ((((~u)^W1_MASK)&0x3f)==v)
#define BUSY(c)      (c->k>COLLAPSE)

#define RSET(v)      {v[0]=0;v[1]=0;v[2]=0;}
#define NEG(v)       {v[0]=-v[0];v[1]=-v[1];v[2]=-v[2];}
#define SAT(v)       {v[0]=SIDE;v[1]=SIDE;v[2]=SIDE;}
#define SUB(v,v1,v2) {v[0]=v2[0]-v1[0];v[1]=v2[1]-v1[1];v[2]=v2[2]-v1[2];}
#define MILD(v)      {v[0]=SIDE_2;v[1]=SIDE_2;v[2]=SIDE_2;}
#define CROSS(a,b,c) {a[0]=b[1]*c[2]-b[2]*c[1];a[1]=b[2]*c[0]-b[0]*c[2];a[2]=b[0]*c[1]-b[1]*c[0];}
#define CP(u,v)      {u[0]=v[0];u[1]=v[1];u[2]=v[2];}

// Cell structure.
// Uses practical types instead of conceptual ones.

typedef struct Cell
{
  // Pointers

  struct
    Cell *ws[6];  // wires to other cells (const.);
  unsigned oE;  // offset inside espacito (const.)
  unsigned oL;  // offset of espacito in lattice (const.)

  // Physical properties

  uint8_t ch;     // charge
  int p[3], s[3]; // momentum, spin
  unsigned a;     // affinity

  // Wavefront

  unsigned n;     // low level tick
  int o[3];       // origin
  unsigned syn;   // synchronism
  boolean occ;    // occupancy control

  // Sine wave

  int u;          // Euler product formula
  int pmf;        // sine PMF
  int pow, den;   // auxiliary

  // Footprint

  int pP[3];      // empodion momentum
  int m[3];       // sought for direction

  // Superluminal variables

  uint8_t f;      // flash
  int po[3];      // pole
  unsigned obj;   // affinity collapsing

  // Interaction control

  unsigned k;     // calculated kind of fragment
  unsigned r;     // pseudo random seed

  pthread_mutex_t mutex;

} Cell;

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
void copy();
void flash();
void expand();
void update();
void interact();
void initAutomaton();
void initEspacito();
void initScreen();
void printCell(Cell *cell);
long OFFSET(int x, int y, int z);
Tuple getPole(Vec3 v, Tuple cen, int r);
void *work(void * parm);
void model();
void interact (Cell *stb, Cell*drf, Cell *nxt, Cell *lst);
void pairFormation(int t, Cell *stb, Cell *drf, Cell *nxt, Cell *lst);
void wires(Cell *ptr, int x0, int y0, int z0, Cell *latt, int oE);


#endif /* SIMULATION_H_ */
