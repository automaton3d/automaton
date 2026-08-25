/*
 * simulation.h
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <vector>
#include <array>
#include <iostream>
#include <cstdint>
#include <limits>

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

/// Integer square root (binary method, table-free).
inline int isqrt(int n)
{
    if (n <= 0) return 0;
    int result = 0;
    int bit = 1 << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

// Platform-independent color type (RGBA)
struct Color {
  uint8_t r, g, b, a;
  
  Color() : r(0), g(0), b(0), a(255) {}
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    : r(red), g(green), b(blue), a(alpha) {}
  
  // Convert to 32-bit integer (RGBA)
  uint32_t toUInt32() const {
    return (static_cast<uint32_t>(r) << 24) |
           (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) |
           static_cast<uint32_t>(a);
  }
  
  // Create from 32-bit integer (RGBA)
  static Color fromUInt32(uint32_t color) {
    return Color(
      (color >> 24) & 0xFF,
      (color >> 16) & 0xFF,
      (color >> 8) & 0xFF,
      color & 0xFF
    );
  }
};

namespace framework
{
  extern void sound(bool loop);
}

namespace automaton
{
  using namespace std;

  using WIndex = uint32_t;
  inline constexpr WIndex NO_LEADER_W = std::numeric_limits<WIndex>::max();

  // Source kinds for the spin-rev source model (K/S/D/P)
  enum class SourceKind : uint8_t { K = 0, S = 1, D = 2, P = 3 };
  inline constexpr uint32_t NO_PARENT = std::numeric_limits<uint32_t>::max();
  inline constexpr uint32_t NO_PAIR   = std::numeric_limits<uint32_t>::max();

  extern unsigned EL;
  extern unsigned W_USED;
  extern bool convol_delay;
  extern bool diffuse_delay;
  extern bool reloc_delay;
  extern std::vector<std::array<unsigned, 3>> lcenters;


struct NeighborResult
{
    int x, y, z, w;

    bool wrapped;
    bool antipodal;
};

  extern string lastAllocationError;

  // Cell Class
  class Cell
  {
    public:
      // Physical properties
      WIndex w;           // Intrinsic W identity
      WIndex leader_w;    // Auxiliary W identity copied from the core
      bool is_core;       // Winding core flag
      unsigned char ch;   // Charge bits q, w1, w0, c2, c1, c0
      bool pB;            // local in-phase wave sign (pB = (u>0)); electric channel trigger
      bool sB;            // emergent transverse polarisation (sB = (v>0)); magnetic channel trigger
      unsigned a;         // Affinity
      unsigned x[4];      // Relative position
      // Wavefront
      unsigned d;         // Euclidean distance
      bool phiB;          // Active wavefront marker (phiB = active)
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
      // Glider (antipodal transport)
      bool gB;            // Glider active flag
      int  g[3] = {0,0,0}; // Signed displacement to antipodal
      // Pulsating sphere
      unsigned int r2;    // Squared distance from center (BFS-propagated)
      int r;              // Integer radius propagated/corrected from r2
      int u, v;           // Radial polarisation pair (u: in-phase, v: quadrature)
      unsigned int active; // 1 when the cell is on the pulsating wavefront
      // Emergent polarisation broadcast (manuscript Sect. "Emergent
      // polarization pair"): b(x) arrival stamp + reconstructed pair.
      unsigned int bstamp; // Arrival tick of the elected-momentum news (0 = never reached)
      int pol_u, pol_v;    // Reconstructed transverse pair (pol_u^2+pol_v^2 = R^4)
      // Spin-rev source model
      SourceKind kind;      // K (chief), S (singleton), D (delegate), P (pair)
      uint32_t parent;      // Parent source index (for D/P)
      int8_t spin_target;   // +1 outward / -1 inward / 0 neutral
      uint32_t pair_idx;    // Pair partner index for P sources
      uint8_t pair_count;   // Number of overlapping pairs in a P source (frequency = 2 * pair_count)
      int m[3];             // Momentum direction vector (long-term stable)
      int reloc[3];         // Consumable relocation offset / impulse
      // Default constructor
      Cell()
        : w(0), leader_w(NO_LEADER_W), is_core(false),
          ch(0), pB(false), sB(false), a(0),
          d(0), phiB(false), t(0), f(0),
          k(0), s2B(false), kB(false), bB(false), hB(false), cB(false),
          gB(false), r2(0xFFFFFFFFu), r(-1), u(0), v(0), active(0),
          bstamp(0), pol_u(0), pol_v(0),
          kind(SourceKind::S), parent(NO_PARENT), spin_target(0), pair_idx(NO_PAIR), pair_count(0)
      {
        fill(begin(x), end(x), 0);
        fill(begin(c), end(c), 0);
        fill(begin(g), end(g), 0);
        fill(begin(m), end(m), 0);
        fill(begin(reloc), end(reloc), 0);
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
  inline Cell& getCell(vector<Cell>& lattice, int x, int y, int z, int w)
  {
    return lattice[((x * EL + y) * EL + z) * W_USED + w];
  }

  
  inline const Cell& getCell(const vector<Cell>& lattice, int x, int y, int z, int w)
  {
    return lattice[((x * EL + y) * EL + z) * W_USED + w];
  }

  /// Function prototypes ///
  void calculateParameters(unsigned L, unsigned W);
  void* SimulationLoop();
  void DeleteAutomaton();
  bool swap_lattices();
  void update();
  bool initSimulation(int step);
  void replicate();
  bool simulation();
  bool convolute(Cell& curr, Cell &draft, Cell &mirror);
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
  void printLattice(int w);
  bool neutralColor(Cell &a, Cell &b);
  bool neutralWeak(Cell &a, Cell &b);
  void shiftMirror();
  bool sanityTest3();
  bool tryAllocate(int EL, int W);
  unsigned int getRandomUnsigned(unsigned int modulus);
  void relocateGlobal(unsigned dx, unsigned dy, unsigned dz);

  // Tests

  void printParams();
  bool sanityTest();
  bool sanityTest2();

  /// Cross variables ///
  extern vector<Cell> lattice_curr;

  /// Cross constants ///
  extern unsigned ORDER;
  extern unsigned EL;
  extern unsigned L2;
  extern unsigned L3;
  extern unsigned W_DIM;
  extern unsigned W_USED;
  extern unsigned long BLOCK;
  extern unsigned CENTER;
  extern unsigned FCENTER;
  extern unsigned UPDATE;
  extern unsigned DIAG;
  extern unsigned RMAX;
  extern unsigned CONTRACT;
  extern unsigned CONVOL;
  extern unsigned GSLOT_X;
  extern unsigned GSLOT_Y;
  extern unsigned GSLOT_Z;
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
  extern unsigned int pulse_tick;

  // W-island topology (W = 3L^2 = (9L) * (L/3))
  extern unsigned ISLAND_SIZE;
  extern unsigned ISLAND_COUNT;

  inline unsigned islandOf(WIndex w)       { return (ISLAND_SIZE > 0) ? (unsigned)(w / ISLAND_SIZE) : 0; }
  inline WIndex firstWOfIsland(unsigned i) { return (WIndex)(i * ISLAND_SIZE); }
  inline bool   isIslandChief(WIndex w)    { return (ISLAND_SIZE > 0) && ((w % ISLAND_SIZE) == 0); }

  #define INF_R2 0xFFFFFFFFu

  void update_pulsating_wavefront();

  // Effective wavefront radius (triangle wave: expands 0→RMAX, contracts RMAX→0).
  // Period = 2*RMAX (= L in physics terms), amplitude = RMAX.
  // This is the local, constant-speed light-clock: a cell is on the active
  // shell exactly when its propagated integer radius r equals this value.
  inline unsigned effective_t(unsigned t)
  {
      unsigned cycle = 2 * RMAX;
      unsigned phase = t % cycle;
      if (phase <= RMAX)
          return phase;
      else
          return cycle - phase;
  }


/// Cross variables ///
extern std::vector<Cell> lattice_curr;
extern std::vector<Cell> lattice_draft; // Add or verify
extern std::vector<Cell> lattice_mirror; // Add or verify

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

// ===================================================================
  // CUDA ACCELERATION FUNCTIONS
  // ===================================================================
  
  // These functions are always declared, regardless of USE_CUDA
  // When USE_CUDA is defined: implemented in cuda_automaton.cu
  // When USE_CUDA is NOT defined: implemented in bridge.cpp as stubs
  
  bool tryEnableCuda();
  void disableCuda();
  bool isCudaEnabled();
  
#ifdef USE_CUDA
  // Internal GPU wrapper functions - only declared when CUDA is enabled
  // Implementations are in cuda_automaton.cu
  bool swap_lattices_gpu();

  // Pointers for Device (GPU) memory
  extern Cell* d_lattice_curr;
  extern Cell* d_lattice_draft;
  extern Cell* d_lattice_mirror;
  
#endif // USE_CUDA
  
}

#endif /* SIMULATION_H_ */
  