/*
 * simulation.h
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "entropy.h"

#define ORDER		4
#define SIDE		13	//((1<<ORDER)+1)
#define CENTER	(SIDE/2)
#define FCENTER	(SIDE/2.0)

#define NORTH   	0
#define EAST    	1
#define SOUTH   	2
#define WEST    	3
#define UP      	4
#define DOWN    	5

#define POINCARE	2400
#define WIDTH		480		// graph width
#define ERA			(POINCARE/WIDTH)

// Macros

#define ZERO(v)		(!(v[0]|v[1]|v[2]))
#define COPY(u,v)   {u[0]=v[0];u[1]=v[1];u[2]=v[2];}

// Charge masks.

#define C0_MASK   	0x01
#define C1_MASK   	0x02
#define C2_MASK   	0x04
#define Q_MASK    	0x08
#define W0_MASK   	0x10
#define W1_MASK   	0x20
#define COLOR_MASK	(C0_MASK | C0_MASK | C0_MASK)
#define WEAK_MASK	(W0_MASK | W1_MASK)

namespace framework
{
	extern unsigned long timer;
}

namespace automaton
{
	// Signed natural

	typedef struct
	{
		unsigned a;				// Absolute value
		bool s;					// sign

	} SN;

	/// Cell structure ///

	typedef struct
	{
	    // Physical properties

		bool pole;
	    unsigned char charge;	// Charge bits w1,w0,q,c2,c1,c0
	    unsigned s[3];			// Rotation driver (spin)
	    unsigned aff;			// Affinity
	    unsigned pos[3];		// Relative position

	    // Wavefront

	    bool wv;				// Wavefront
	    unsigned d;				// Euclidean distance code
	    unsigned sin;			// Basic sine half cycle
	    SN A;					// Amplitude of frequency freq
	    unsigned freq;			// Frequency (angle increment)
	    unsigned angle;			// To find the amplitude

	    // Superluminal

	    unsigned c[3];			// Relocation offset
	    unsigned k, t;			// Tick counters
	    bool ctrl;				// Cycle control bit
	    bool reloc;				// Relocation flag

	    // Interference

	    unsigned m[3];			// Parallel transport offset
	    bool e;					// Empodion role
	    SN A_bar;				// Boson average amplitude
	    SN ph;					// Cumulative phase

	    // Interaction control

	    bool collapse;
	    unsigned net_c0;		// Net color 0 charge
	    unsigned net_c1;		// Net color 1 charge
	    unsigned net_c2;		// Net color 2 charge
	    unsigned net_q;			// Net electric charge
	    unsigned net_w0;		// Net weak 0 charge
	    unsigned net_w1;		// Net weak 1 charge
	    bool fxf;				// fermion x fermion flag
	    bool bxb;				// boson x boson flag
	    bool fxb;				// fermion x boson flag
	    bool wxw;				// segregation flag
	    bool boson;				// boson x non boson

	} Cell;

	/// Function prototypes ///

	void *SimulationLoop();
	void DeleteAutomaton();
	void update();
	void initSimulation();
	void initScreen();
	void simulation();
	void updateBuffer();
	bool isAllowed(int dir, int dst[3]);
	void displayLattice();
	void listLattice();
	int calc_index(int x, int y, int z, int w);
	Cell* get_neighbor(Cell *lattice, int index, int dir);
	bool isColorNeutral(unsigned char c1, unsigned char c2);
	void printLattice();
	unsigned int nextPowerOfTwo(unsigned int n);
	void normalize(double vec[3]);
	void cross_product(double result[3], const double a[3], const double b[3]);
	void compileNetCharges(Cell *mirror, Cell *draft);
	void relocate(Cell *draft, Cell *nei);
	void markPoles(unsigned p[3], int w);

	/// Cross variables ///

	extern COLORREF *voxels;
	extern unsigned long timer;
	extern bool stop;

	extern Cell *lattice_current;

	/// Cross constants ///

    extern const unsigned XYZ_DIFFUSION;
	extern const unsigned RELOC;
    extern const unsigned RMAX;
	extern const unsigned DIAG;
	extern const unsigned LIGHT;
	extern const unsigned RANGE;
	extern const unsigned UPDATE;
	extern const unsigned FRAME;
	extern const unsigned FMAX;
	extern const unsigned SIDE2;
	extern const unsigned SIDE3;
	extern const unsigned W_DIM;
	extern const unsigned BLOCK;
	extern const unsigned CONVOL;

}

#endif /* SIMULATION_H_ */
