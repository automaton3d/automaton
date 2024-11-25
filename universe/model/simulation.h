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
#include <fstream>
#include <mmsystem.h>
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

#define POINCARE	48000
#define WIDTH		480		// graph width
#define ERA			(POINCARE/WIDTH)

// Simulation IDE

#define GRAPH

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
    extern void sound();
}

namespace automaton
{
	// Signed Natural (SN) Class
	class SN
	{
		public:
			unsigned a; // Absolute value
			bool s;     // Sign

			// Default Constructor
			SN() : a(0), s(false) {}

			// Parameterized Constructor
			SN(unsigned abs_val, bool sign) : a(abs_val), s(sign) {}

			// Getters
			unsigned getAbsolute() const { return a; }
			bool getSign() const { return s; }

			// Setters
			void setAbsolute(unsigned abs_val) { a = abs_val; }
			void setSign(bool sign) { s = sign; }

			// Friend functions for serialization
			friend void serializeSN(const SN& sn, std::ofstream& out);
			friend void deserializeSN(SN& sn, std::ifstream& in);
	};

	// Declare serialize and deserialize functions
	void serializeSN(const SN& sn, std::ofstream& out);
	void deserializeSN(SN& sn, std::ifstream& in);

	// Cell Class
	class Cell
	{
		public:
		// Attributes
		bool pole;
		unsigned char charge;   // Charge bits
		unsigned s[3];          // Rotation driver (spin)
		unsigned aff;           // Affinity
		unsigned pos[3];        // Relative position

		// Wavefront
	    bool wv;
	    unsigned d;
	    unsigned sin;
	    SN A;                   // Amplitude of frequency
	    unsigned freq;
	    unsigned angle;

	    // Superluminal
	    unsigned c[3];
	    unsigned k, t;
	    bool ctrl;
	    bool reloc;

	    // Interference
	    unsigned m[3];
	    bool e;
	    SN A_bar;               // Boson average amplitude
	    SN ph;                  // Cumulative phase

	    // Interaction control
	    bool collapse;
	    unsigned net_c0, net_c1, net_c2, net_q, net_w0, net_w1;
	    bool fxf, bxb, fxb, wxw, boson;

		public:
	    // Constructor
	    Cell()
        	: pole(false), charge(0), aff(0), wv(false), d(0), sin(0), freq(0), angle(0),
          k(0), t(0), ctrl(false), reloc(false), e(false), collapse(false),
          net_c0(0), net_c1(0), net_c2(0), net_q(0), net_w0(0), net_w1(0),
          fxf(false), bxb(false), fxb(false), wxw(false), boson(false) {
        std::fill(std::begin(s), std::end(s), 0);
        std::fill(std::begin(pos), std::end(pos), 0);
        std::fill(std::begin(c), std::end(c), 0);
        std::fill(std::begin(m), std::end(m), 0);
    }
	void serialize(std::ofstream& out) const;
	void deserialize(std::ifstream& in);
	};


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
	void printLattice();
	unsigned int nextPowerOfTwo(unsigned int n);
	void normalize(double vec[3]);
	void cross_product(double result[3], const double a[3], const double b[3]);
	void compileNetCharges(Cell *mirror, Cell *draft);
	void relocate(Cell *draft, Cell *nei);
	void markPoles(unsigned p[3], int w);
	int get_neighbor_index(int index, int dir);
	void saveState0();
	void checkPoincare();
    void detectPoincare();

	/// Cross variables ///

	extern COLORREF *voxels;
	extern unsigned long timer;
	extern bool stop;

    extern double H0;

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
	extern std::vector<Cell> lattice_current;

}

#endif /* SIMULATION_H_ */
