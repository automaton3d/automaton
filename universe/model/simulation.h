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
#include <iostream>
#include <assert.h>

#define SIDE	11			// Odd multiple of 3 (9, 15, 21, ...)
#define SIDE2	(SIDE*SIDE)
#define SIDE3	(SIDE2*SIDE)
#define SIDE4	(SIDE2*SIDE2)
#define DEPTH	(13*SIDE2/4)

#define NORTH   0
#define EAST    1
#define SOUTH   2
#define WEST    3
#define UP      4
#define DOWN    5

#define ZERO(v)      (!(v[0]|v[1]|v[2]))
#define MAG(v)       ((v)[0]*(v)[0]+(v)[1]*(v)[1]+(v)[2]*(v)[2])
#define CP(u,v)      {u[0]=v[0];u[1]=v[1];u[2]=v[2];}

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

	/// Cell structure ///

	typedef struct
	{
		// Wavefront

	    unsigned n, m;			// Tick counters
	    bool ctrl;				// Cycle control bit
	    bool wv;				// Wavefront
	    unsigned d;				// Euclidean distance code
	    bool reloc;				// Relocation flag
	    unsigned x, y, z;		// Relative position
	    unsigned cx, cy, cz;	// Relocation offset
	    unsigned dx, dy, dz;	// Parallel transport offset

	    // Sine phase

	    unsigned amplitude;		// Combined phase
	    unsigned sin;			// Basic sine half cycle
	    unsigned angle;			// To find the phase
	    unsigned freq;			// Frequency (angle increment)

	    // Physical properties

	    unsigned char charge;	// Charge bits w1,w0,q,c2,c1,c0
	    int p[3];				// Motion driver (momentum)
	    int s[3];				// Rotation driver (spin)
	    unsigned aff;			// Affinity
	    int o[3];

	    bool seed;				// Marks where p punches the sphere

	    // Interference

	    bool e;					// Empodion role

	    // Interaction control

	    bool boson;				// boson x non boson
	    unsigned net_c0;		// Net color 0 charge
	    unsigned net_c1;		// Net color 1 charge
	    unsigned net_c2;		// Net color 2 charge
	    unsigned net_q;			// Net electric charge
	    unsigned net_w0;		// Net weak 0 charge
	    unsigned net_w1;		// Net weak 1 charge
	    bool fxf;				// fermion x fermion flag
	    bool bxb;
	    bool fxb;
	    bool collapse;

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
	Cell* get_neighbor(int index, int dir);
	bool isColorNeutral(unsigned char c1, unsigned char c2);
	bool isWeakNeutral(unsigned char c1, unsigned char c2);
	void printLattice();

	/// Cross variables ///

	extern COLORREF *voxels;
	extern unsigned long timer;
	extern bool stop;

	extern Cell *lattice_main;

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

}

#endif /* SIMULATION_H_ */
