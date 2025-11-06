/*
 * simulation.h
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <windows.h>
#include <vector>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simulation symbols
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
  using namespace std;

  extern unsigned EL;
  extern unsigned W_USED;

  struct Point
  {
	unsigned x, y, z;
	bool operator==(const Point& other) const
	{
	  return x == other.x && y == other.y && z == other.z;
	}
  };

  // Define the outer structure
  struct WPoint
  {
    Point p;
  };

  extern string lastAllocationError;

  // Cell Class
  class Cell
  {
    public:
      // Physical properties
      unsigned char ch;   // Charge bits q, w1, w0, c2, c1, c0
      bool pB;            // Linear motion direction bit
      bool sB;            // Rotation spiral bit
      unsigned a;         // Affinity
      unsigned x[4];      // Relative position
      // Wavefront
      unsigned d;         // Euclidean distance
      bool phiB;          // Fixed period mask bit
      unsigned t;         // Light frame counter
      unsigned f;         // Sine phase parameter
      // Operational variables
      unsigned c[3] = { 0, 0, 0 }; // Relocation offset
      unsigned k;         // Tick counter
      bool s2B;           // Sieve test result
      // Interaction control
      bool kB;            // Collapse flag
      bool bB;            // Blob flag
      bool hB;            // Hunt flag
      bool cB;            // Contraction flag
      // Default constructor
      Cell()
        : ch(0), pB(false), sB(false), a(0),
          d(0), phiB(false), t(0), f(0),
          k(0), s2B(false), kB(false), bB(false), hB(false), cB(false)
      {
        std::fill(std::begin(x), std::end(x), 0);
        std::fill(std::begin(c), std::end(c), 0);
      }
      // Serialization functions
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


  // Inline accessor for 4D indexing
  inline Cell& getCell(std::vector<Cell>& lattice, int x, int y, int z, int w)
  {
    return lattice[((x * EL + y) * EL + z) * W_USED + w];
  }

  /// Function prototypes ///
  void calculateParameters(unsigned L, unsigned W);
  void* SimulationLoop();
  void DeleteAutomaton();
  void swap_lattices();
  void update();
  bool initSimulation(int step);
  void initSpirals();
  void replicate();
  void markPoints(unsigned p[3], int w);
  void simulation();
  bool convolute(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute0(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute1(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute2(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute3(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute4(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute5(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute6(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute7(Cell& curr, Cell &draft, Cell &mirror);
  void diffuse(Cell& curr, Cell &draft, Cell &forward, Cell &north, Cell &west, Cell &down, Cell &south, Cell &east, Cell &up);
  void relocate(Cell& curr, Cell &draft, Cell &north, Cell &west, Cell &down);
  void reissue(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up);
  void flood(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up);
  void updateBuffer();
  std::vector<std::tuple<int, int, int>> generateShell(int L);
  void normalize(double vec[3]);
  void cross_product(double result[3], const double a[3], const double b[3]);
  void printLattice(int w);
  bool neutralColor(Cell &a, Cell &b);
  bool neutralWeak(Cell &a, Cell &b);
  void shiftMirror();
  bool sanityTest3();
  bool tryAllocate(int EL, int W);
  unsigned int getRandomUnsigned(unsigned int modulus);

  // Tests

  void printParams();
  bool sanityTest();
  bool sanityTest2();

  /// Cross variables ///
  extern COLORREF* voxels;
  extern std::vector<Cell> lattice_curr;

  /// Cross constants ///
  extern unsigned L3;
  extern unsigned long BLOCK;
  extern unsigned DIAG;
  extern unsigned RMAX;
  extern unsigned CONTRACT;
  extern unsigned CONVOL;
  extern unsigned SLOT1;
  extern unsigned SLOT2;
  extern unsigned SLOT3;
  extern unsigned SLOT4;
  extern unsigned DIFFUSION;
  extern unsigned SLOT5;
  extern unsigned SLOT6;
  extern unsigned SLOT7;
  extern unsigned SLOT8;
  extern unsigned RELOC;
  extern unsigned REISSUE;
  extern unsigned FLOOD;
  extern unsigned FRAME;

  /**
   * Tests if two vectors are equal.
   */
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
