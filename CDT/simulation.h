/*
 * automaton.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#define _GNU_SOURCE

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "tuple.h"
#include "vector3d.h"

// Lattice symbols

#define ORDER     3  //269 (see Section 2.3)
#define SIDE      (1<<ORDER)
#define SIDE2     (SIDE*SIDE)
#define SIDE3     (SIDE*SIDE2)
#define SIDE4     (SIDE*SIDE3)
#define SIDE5     (SIDE*SIDE4)
#define SIDE6     (SIDE*SIDE5)
#define MASK      (SIDE-1)
#define DIAG      (2*MASK)
#define LIGHT     (2*DIAG)
#define LIGHT2    (LIGHT*LIGHT)
#define S         (SIDE/2)
#define TOL       (SIDE/1024)    // Provisional value
#define LIMIT     (3*(SIDE2-2*SIDE+1))

// Particle symbols (ored in code)

#define PHOTON    0x0001
#define GLUON     0x0002
#define NEUTRINO  0x0004
#define ZB        0x0008
#define WB        0x0010
#define UP        0x0020
#define DOWN      0x0040

#define FERMION   0x0080

// Charge masks

#define C_MASK    0x07
#define Q_MASK    0x08
#define W0_MASK   0x10
#define W1_MASK   0x20

// Macros (helps readability)

#define ISNULL(v)     (v[0]==0 && v[1]==0 && v[2]==0)
#define ISEQUAL(v,u)  (v[0]==u[0] && v[1]==u[1] && v[2]==u[2])
#define RESET(v)      {v[0]=0;v[1]=0;v[2]=0;}
#define COPY(u,v)     {u[0]=v[0];u[1]=v[1];u[2]=v[2];}
#define MOD2(v)       (v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
#define NEG(v)        {v[0]=-v[0];v[1]=-v[1];v[2]=-v[2];}
#define DOT(u,v)      (u[0]*v[0]+u[1]*v[1]+u[2]*v[2])
#define GETC(u)       (u->ch & C_MASK)
#define GETW1(u)      ((u->ch & W1_MASK) == W1_MASK)
#define GETW0(u)      ((u->ch & W0_MASK) == W0_MASK)
#define GETQ(u)       ((u->ch & Q_MASK) == Q_MASK)
#define SUB(v,v1,v2)  {v[0]=v2[0]-v1[0];v[1]=v2[1]-v1[1];v[2]=v2[2]-v1[2];}
#define CROSS(a,b,c)  {a[0]=b[1]*c[2]-b[2]*c[1];a[1]=b[2]*c[0]-b[0]*c[2];a[2]=b[0]*c[1]-b[1]*c[0];}
#define ISMATTER(u)   (GETC(u)>2&&GETC(u)!=4)

// Handedness

#define RIGHT  0
#define LEFT  1

// Tensor structure

typedef struct Tensor
{
  // Physical properties

  unsigned char ch;// charge
  int p[3], s[3];  // momentum, spin
  unsigned a;      // affinity

  // Wavefront

  unsigned t;      // lifetime
  unsigned tt;     // lifetime
  int o[3];        // origin
  unsigned syn;    // synchronism
  int u, v;        // Euler
  unsigned f;      // frequency

  // Footprint

  int p0[3];       // momentum propensity
  int prone[3];    // sought for direction

  // Superluminal variables

  unsigned char flash; // flash
  unsigned pole[3];    // pole
  unsigned target;     // affinity collapsing

  // Pointers

  unsigned offset;     // offset inside espacito (constant)
  struct Tensor *wires[6];  // wires to other cells (constant);

  // Interaction data

  unsigned char kind;  // kind of fragment
  unsigned noise;      // pseudorandom seed

} Tensor;

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
Tuple getPole(Vector3d v, Tuple cen, int r);
void copy();
void flash();
void expand();
void update();
void interact();
void initAutomaton();
void initEspacito();
void initScreen();
void printCell(Tensor *cell);

#endif /* SIMULATION_H_ */
