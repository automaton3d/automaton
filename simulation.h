/*
 * simulation.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <windows.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include "main3d.h"

// Lattice symbols.

#define ORDER    4  //202 (see Section 2.3)
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
#define NDIR     6    // von Neumman directions

// Roles.

#define EMPTY     0x00
#define SEED      0x01
#define WAVE      0x02
#define GRID      0x04
#define TRAVELLER 0x08

// Particle singles and pairs (used in k).

#define NONE     0x0000
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

// Charge masks.

#define C_MASK   0x07
#define Q_MASK   0x08
#define W0_MASK  0x10
#define W1_MASK  0x20

// Handedness.

#define RIGHT 0
#define LEFT  1

// Macros (helps readability).

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
#define ANNIHIL(u,v) (C(u)==_C(v))
#define GET_ROLE(c)  ((c)->a1 == 0 ? EMPTY : (!ZERO((c)->s) ? (ZERO((c)->p) ? WAVE : SEED) : (ISSAT((c)->po) ? GRID : TRAVELLER)))
#define DOT(u, v)    ((u)[0] * (v)[0] + (u)[1] * (v)[1] + (u)[2] * (v)[2])
#define MOD2(v)      ((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2])
#define RSET(v)      {v[0]=0;v[1]=0;v[2]=0;}
#define SAT(v)       {v[0]=SIDE;v[1]=SIDE;v[2]=SIDE;}
#define SUB(v,v1,v2) {v[0]=v2[0]-v1[0];v[1]=v2[1]-v1[1];v[2]=v2[2]-v1[2];}
#define MILD(v)      {v[0]=SIDE_2;v[1]=SIDE_2;v[2]=SIDE_2;}
#define CROSS(a,b,c) {a[0]=b[1]*c[2]-b[2]*c[1];a[1]=b[2]*c[0]-b[0]*c[2];a[2]=b[0]*c[1]-b[1]*c[0];}
#define CP(u,v)      {u[0]=v[0];u[1]=v[1];u[2]=v[2];}
#define CPNEG(u,v)   {u[0]=-v[0];u[1]=-v[1];u[2]=-v[2];}

// Cell structure.
// Uses practical types instead of conceptual ones.

typedef struct Cell
{
  unsigned off;     // offset (const.)

  // Physical properties.

  char ch;          // charge
  int p[3], s[3];   // momentum, spin
  unsigned a1, a2;  // affinity

  // Wavefront.

  unsigned n;       // low level tick
  int o[3];         // origin
  unsigned syn;     // synchronism

  unsigned occ;     // occupancy

  // Sine wave.

  int u;            // Euler product formula

  // Footprint.

  int pP[3];        // empodion momentum
  int m[3];         // sought for direction

  // Superluminal variables.

  int po[3];        // pole
  unsigned obj;     // affinity collapsing

  // Interaction control.

  unsigned k;       // calculated kind of fragment

} Cell;

/// Functions ///

void *SimulationLoop();
void DeleteAutomaton();
void copy();
void update();
void initSimulation();
void initScreen();
void model(int phase);
Cell *neighbor(Cell *ptr, int dir);
void phase1();
void phase2();
void phase3();
void phase4();
void interact (Cell *stb, Cell*drf, Cell *nxt, Cell *lst);
void managePairs(int t, Cell *stb, Cell *drf, Cell *nxt, Cell *lst);
void sanityCheck();
void printCell(Cell *cell);
void drawModel(HDC hdc);
void fromAxisAngle(const float axis[3], float angleRadians, float *result);

#endif /* SIMULATION_H_ */