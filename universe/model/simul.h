/*
 * simul.h
 *
 *  Created on: 11 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef MODEL_SIMUL_H_
#define MODEL_SIMUL_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <vector>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <mmsystem.h>
#include <algorithm>
#include <assert.h>
#include "entropy.h"

//#define ORDER        4
//#define SIDE        9   //((1<<ORDER)+1)
#define SIDE2		(SIDE*SIDE)
#define W_DIM		(3*SIDE2)
#define ORDER       ((int)round(log2(SIDE)))
//#define FCENTER     (SIDE/2.0)

#define NORTH       0
#define EAST        1
#define SOUTH       2
#define WEST        3
#define UP          4
#define DOWN        5
#define FORWARD     6
#define BACKWARD    7

// Simulation IDE
#define GRAPH

// Macros
#define ZERO(v)     (!(v[0] | v[1] | v[2]))

// Charge masks
#define C0_MASK     0x01
#define C1_MASK     0x02
#define C2_MASK     0x04
#define Q_MASK      0x08
#define W0_MASK     0x10
#define W1_MASK     0x20
#define COLOR_MASK  (C0_MASK | C1_MASK | C2_MASK)
#define WEAK_MASK   (W0_MASK | W1_MASK)

namespace framework
{
    extern unsigned long long timer;
    extern void sound();
}

namespace automaton_back
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
      //      friend void serializeSN(const SN& sn, ofstream& out);
        //    friend void deserializeSN(SN& sn, ifstream& in);
    };

    // Declare serialize and deserialize functions
   // void serializeSN(const SN& sn, ofstream& out);
   // void deserializeSN(SN& sn, ifstream& in);

    // Cell Class
    class Cell
    {
        public:
            // Attributes
            bool pole;				// Active spot of momentum
            unsigned char charge;   // Charge bits
            unsigned s[3] = { 0, 0, 0 };	// Rotation driver (spin)
            unsigned aff;           // Affinity
            unsigned pos[3] = { 0, 0, 0 }; // Relative position

            // Wavefront
            bool wv;				// Wavefront flag
            unsigned d;				// Euclidean distance
            unsigned sin;			// Half-wave sine pattern
            SN A;                   // Amplitude of frequency
            unsigned freq;			// Number of superposed bubbles
            unsigned angle;			// Sine phase parameter

            // Superluminal
            unsigned c[3] = { 0, 0, 0 };	// Relocation offset
            unsigned k, t;					// Tick counters

            // Interference
            unsigned m[3] = { 0, 0, 0 };	// Sought for direction
            bool e;					// Empodion flag
            SN A_bar;               // Boson average amplitude
            SN ph;                  // Cumulative phase

            // Interaction control
            bool collapse;			// Collapse flag
            // Charge counters
            unsigned net_c0, net_c1, net_c2, net_q, net_w0, net_w1;
            // Interaction flags
            bool fxf, bxb, fxb, wxw, boson;

            // Constructor
            Cell()
                : pole(false), charge(0), aff(0), wv(false), d(0), sin(0),
				  freq(0), angle(0), k(0), t(0), e(false), collapse(false),
                  net_c0(0), net_c1(0), net_c2(0), net_q(0), net_w0(0), net_w1(0),
                  fxf(false), bxb(false), fxb(false), wxw(false), boson(false) {}

            Cell(const Cell& other)
                : pole(other.pole), charge(other.charge), aff(other.aff),
                  wv(other.wv), d(other.d), sin(other.sin),
                  A(other.A), freq(other.freq), angle(other.angle),
                  k(other.k), t(other.t), e(other.e), A_bar(other.A_bar), ph(other.ph),
                  collapse(other.collapse),
                  net_c0(other.net_c0), net_c1(other.net_c1), net_c2(other.net_c2),
                  net_q(other.net_q), net_w0(other.net_w0), net_w1(other.net_w1),
                  fxf(other.fxf), bxb(other.bxb), fxb(other.fxb),
                  wxw(other.wxw), boson(other.boson)
            {
                std::copy(std::begin(other.s), std::end(other.s), std::begin(s));
                std::copy(std::begin(other.pos), std::end(other.pos), std::begin(pos));
                std::copy(std::begin(other.c), std::end(other.c), std::begin(c));
                std::copy(std::begin(other.m), std::end(other.m), std::begin(m));
            }
    };

    /// Function prototypes ///
    bool checkPoincare();
    bool isColorNeutral(unsigned char c1, unsigned char c2);
	uint32_t cellState(unsigned x, unsigned y, unsigned z, Cell *cell);
    void* SimulationLoop();
    void DeleteAutomaton();
    void update();
    bool initSimulation(int step);
    void initScreen();
    void simulation();
    void updateBuffer();
    void displayLattice();
    void printLattice();
    void normalize(double vec[3]);
    void cross_product(double result[3], const double a[3], const double b[3]);
    void compileNetCharges(Cell &mirror, Cell &draft);
	void relocate(Cell& draft, const Cell& nei);
    void markPoles(unsigned p[3], int w);
    void saveState0();
    void updateEntropy();
    void detectPoincare();
    void shiftW();
    void executeInteraction(Cell &curr, Cell &draft);
    bool convolute(Cell& curr, Cell &draft, Cell &mirror, Cell &wleft, Cell &wright);
    void seggregation(Cell& curr, Cell &draft, Cell &mirror);

	// Tests

    void printConstants();
    bool sanityTest1();
    bool sanityTest2();
    bool sanityTest3();
    bool sanityTest4();

    /// Cross variables ///
    extern COLORREF* voxels;
    extern Cell lattice_curr[SIDE][SIDE][SIDE][W_DIM];

    /// Cross constants ///
    extern const unsigned SIDE3;
    extern const unsigned BLOCK;
    extern const unsigned DIAG;
    extern const unsigned RMAX;
    extern const unsigned FMAX;
    extern const unsigned CONVOL;
    extern const unsigned COLLISION;
    extern const unsigned W_DIFFUSION;
    extern const unsigned X_DIFFUSION;
    extern const unsigned Y_DIFFUSION;
    extern const unsigned Z_DIFFUSION;
    extern const unsigned RELOCATION;
    extern const unsigned TRANSPORT;
    extern const unsigned UPDATE;
    extern const unsigned LIGHT;
    extern const unsigned RANGE;
    extern const unsigned FRAME;

}


#endif /* MODEL_SIMUL_H_ */
