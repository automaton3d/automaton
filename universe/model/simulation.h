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
#include <vector>
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <mmsystem.h>
#include <algorithm>
#include <assert.h>
#include "nvector.h"

#define EL        11                       // An odd number
#define L2        (EL*EL)
#define W_DIM     (3*L2+1)              // An even number
#define ORDER     ((int)round(log2(EL))) // Number of bits
#define CENTER    ((EL-1)/2)
#define FCENTER   (EL/2.0)

#define NORTH     0
#define EAST      1
#define SOUTH     2
#define WEST      3
#define UP        4
#define DOWN      5
#define FORWARD   6
#define BACKWARD  7

// Enable simulation IDE
#define GRAPH

// Macros
#define ZERO(v)   (!(v[0] | v[1] | v[2]))

// Charge masks
#define C0_MASK     0x01
#define C1_MASK     0x02
#define C2_MASK     0x04
#define Q_MASK      0x08
#define W0_MASK     0x10
#define W1_MASK     0x20
#define COLOR_MASK  (C0_MASK | C1_MASK | C2_MASK)
#define WEAK_MASK   (W0_MASK | W1_MASK)
#define CHARGE_MASK (W0_MASK | W1_MASK | C0_MASK | C1_MASK | C2_MASK | Q_MASK)

namespace framework
{
  extern unsigned long long timer;
  extern void sound(bool loop);
}

namespace automaton
{
  extern const unsigned LIGHT;
  extern const unsigned COLLISION;
  extern const unsigned UPDATE;

  using namespace std;

  // Cell Class
  class Cell
  {
    public:
      // Physical properties
      unsigned char ch; 	// Charge bits q, w1, w0, c2, c1, c0
      bool pB;            	// Linear motion direction bit
      bool sB;        		// Rotation spiral bit
      unsigned a;           // Affinity
      unsigned x[4] = { 0, 0, 0 }; // Relative position
      // Wavefront
      unsigned d;        	// Euclidean distance
      bool phiB;			// Fixed period mask bit
      unsigned t;         	// Light frame counter
      unsigned f;      		// Sine phase parameter
      // Operational variables
      unsigned c[3] = { 0, 0, 0 }; // Relocation offset
      unsigned k;         	// Tick counter
      bool hB;
      // Interaction control
      bool kB;      		// Collapse flag
      // Default constructor
      Cell()
        : ch(0), pB(false), sB(false), a(0),
          d(0), phiB(false), t(0), f(0),
          k(0), hB(false), kB(false)
      {
        std::fill(std::begin(x), std::end(x), 0);
        std::fill(std::begin(c), std::end(c), 0);
      }
      // Copy constructor
      Cell(const Cell& other)
        : ch(other.ch), pB(other.pB), sB(other.sB), a(other.a),
          d(other.d), phiB(other.phiB), t(other.t), f(other.f),
          k(other.k), hB(other.hB), kB(other.kB)
      {
        std::copy(std::begin(other.x), std::end(other.x), std::begin(x));
        std::copy(std::begin(other.c), std::end(other.c), std::begin(c));
     }      // Serialization functions
      void serialize(ofstream& out) const;
      void deserialize(ifstream& in);

      bool Q() { return ch & Q_MASK; }
      bool W1() { return ch & W1_MASK; }
      bool W0() { return ch & W0_MASK; }
      bool C2() { return ch & C2_MASK; }
      bool C1() { return ch & C1_MASK; }
      bool C0() { return ch & C0_MASK; }
      unsigned char COLOR() { return ch & COLOR_MASK; }
      unsigned char ANTICOLOR() { return ~ch & COLOR_MASK; }

      Cell &getNeighbor(int i);
  };

  /// Function prototypes ///
  void swap_lattices();
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
  void markMetaPoles(unsigned p[3], int w);
  void saveState0();
  void updateEntropy();
  void shiftX(unsigned w);
  void shiftY(unsigned w);
  void shiftZ(unsigned w);
  void shiftW();
  void executeInteraction(Cell &curr, Cell &draft);
  bool convolute(Cell& curr, Cell &draft, Cell &mirror);
  void diffuse(Cell& curr, Cell &draft, Cell &mirror);
  void relocate(Cell& curr, Cell &draft, Cell &mirror);
  void transport(Cell& curr, Cell &draft, Cell &mirror);
  void seggregation(Cell& curr, Cell &draft, Cell &mirror);
  void testReloc(Cell& curr, Cell &draft, Cell &mirror, unsigned w);
  //  vector<tuple<int, int, int>> generateUniformSpherePoints(int R, int u, int L);

  // Tests

  void printConstants();
  bool sanityTest1();

  /// Cross variables ///
  extern COLORREF* voxels;
  extern Cell lattice_curr[EL][EL][EL][W_DIM];

  /// Cross constants ///
  extern const unsigned L3;
  extern const unsigned long BLOCK;
  extern const unsigned DIAG;
  extern const unsigned RMAX;
  extern const unsigned FMAX;
  extern const unsigned CONVOL;
  extern const unsigned COLLISION;
  extern const unsigned DIFFUSION;
  extern const unsigned RELOC;
  extern const unsigned TRANSP;
  extern const unsigned FRAME;

  inline bool EQUAL(unsigned v1[3], unsigned v2[3])
  {
    // Compare elements
    for (size_t i = 0; i < 3; ++i)
    {
      if (v1[i] != v2[i])
        return false;
    }
    return true;
  }

}

#endif /* SIMULATION_H_ */
