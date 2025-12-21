/*
 * simulation.h
 * CUDA GPU Version - Consolidated & Production-Ready
 * Optimized for automaton_compute_cuda.cu
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <tuple>
#include <fstream>
#include <array>
#include <cmath>
#include <algorithm>

// ===================================================================
// SIMULATION CONSTANTS & MACROS
// ===================================================================

// Direction symbols (von Neumann neighborhood in 4D)
#define NORTH     0
#define EAST      1
#define SOUTH     2
#define WEST      3
#define UP        4
#define DOWN      5
#define FORWARD   6
#define BACKWARD  7

// Feature flags
#define GRAPH         // Enable visualization
#ifndef USE_CUDA
    #define USE_CUDA
#endif
// Vector macros
#define ZERO(v)   (!(v[0] | v[1] | v[2]))

// Charge bit masks (6-bit charge encoding)
#define C0_MASK     0x01  // Color bit 0
#define C1_MASK     0x02  // Color bit 1
#define C2_MASK     0x04  // Color bit 2
#define Q_MASK      0x08  // Quark flag
#define W0_MASK     0x10  // Weak force bit 0
#define W1_MASK     0x20  // Weak force bit 1

// Composite masks
#define COLOR_MASK  (C0_MASK | C1_MASK | C2_MASK)
#define WEAK_MASK   (W0_MASK | W1_MASK)
#define CHARGE_MASK (W0_MASK | W1_MASK | C0_MASK | C1_MASK | C2_MASK | Q_MASK)

// ===================================================================
// FRAMEWORK INTERFACE
// ===================================================================

namespace framework
{
  extern void sound(bool loop);
}

// ===================================================================
// AUTOMATON NAMESPACE - Core CA Engine
// ===================================================================

namespace automaton
{
  // -------------------------------------------------------------------
  // Grid Configuration (Runtime Parameters)
  // -------------------------------------------------------------------
  
  // Note: Grid dimension variables are defined in .cu implementation files
  // These extern declarations allow code to reference automaton::EL, etc.
  extern unsigned EL;              // Grid edge length (spatial dimensions)
  extern unsigned W_USED;          // Number of W-layers (4th dimension)
  extern unsigned CENTER;          // Grid center coordinate
  extern unsigned L2;              // EL² (calculated)
  extern unsigned L3;              // EL³ (calculated)
  extern unsigned long BLOCK;      // L3 × W_USED (total cells)
  
  // Simulation control flags (defined in .cu files)
  extern bool convol_delay;        // Convolution phase delay flag
  extern bool diffuse_delay;       // Diffusion phase delay flag
  extern bool reloc_delay;         // Relocation phase delay flag
  
  // Lattice center tracking (one center per W-layer, defined in .cu files)
  extern std::vector<std::array<unsigned, 3>> lcenters;
  
  // Allocation error reporting
  extern std::string lastAllocationError;

  // -------------------------------------------------------------------
  // Core Data Structures
  // -------------------------------------------------------------------
  
  /**
   * Point - 3D spatial coordinate
   */
  struct Point
  {
    unsigned x, y, z;
    
    bool operator==(const Point& other) const
    {
      return x == other.x && y == other.y && z == other.z;
    }
  };

  /**
   * WPoint - Point with W-dimension context
   */
  struct WPoint
  {
    Point p;
  };

  /**
   * Voxel - Visualization primitive (rendered cell data)
   */
  struct Voxel
  {
    unsigned char r, g, b, a;  // RGBA color channels
    unsigned char charge;       // Encoded charge state
    unsigned distance;          // Wavefront distance field
    
    Voxel() : r(0), g(0), b(0), a(0), charge(0), distance(0) {}
  };

  /**
   * Cell - Fundamental automaton unit
   * 
   * Represents a single cell in the 4D cellular automaton lattice.
   * Contains physical properties, wavefront data, and interaction flags.
   */
  class Cell
  {
    public:
      // ------- Physical Properties -------
      unsigned char ch;   // Charge encoding (6 bits: q, w1, w0, c2, c1, c0)
      bool pB;            // Linear motion polarity bit
      bool sB;            // Rotation spiral direction bit
      unsigned a;         // Affinity identifier (particle type)
      unsigned x[4];      // 4D position (x, y, z, w)
      
      // ------- Wavefront Propagation -------
      unsigned d;         // Euclidean distance from origin
      bool phiB;          // Phase lock bit
      unsigned t;         // Temporal frame counter
      unsigned f;         // Phase parameter (for oscillations)
      
      // ------- Operational State -------
      unsigned c[3];      // Relocation offset vector (dx, dy, dz)
      unsigned k;         // Local tick counter (simulation phase)
      bool s2B;           // Sieve test result (filtering)
      
      // ------- Interaction Flags -------
      bool kB;            // Collapse flag (particle interaction)
      bool bB;            // Blob flag (clustering)
      bool hB;            // Hunt flag (search mode)
      bool cB;            // Contraction flag (attraction)
      
      // ------- Constructor -------
      Cell()
        : ch(0), pB(false), sB(false), a(0),
          d(0), phiB(false), t(0), f(0),
          k(0), s2B(false), kB(false), bB(false), hB(false), cB(false)
      {
        std::fill(std::begin(x), std::end(x), 0);
        std::fill(std::begin(c), std::end(c), 0);
      }
      
      // ------- Persistence -------
      void serialize(std::ofstream& out) const;
      void deserialize(std::ifstream& in);

      // ------- Charge Bit Accessors -------
      inline bool Q()  { return ch & Q_MASK; }
      inline bool W1() { return ch & W1_MASK; }
      inline bool W0() { return ch & W0_MASK; }
      inline bool C2() { return ch & C2_MASK; }
      inline bool C1() { return ch & C1_MASK; }
      inline bool C0() { return ch & C0_MASK; }
      inline unsigned char COLOR() { return ch & COLOR_MASK; }
      inline unsigned char ANTICOLOR() { return ~ch & COLOR_MASK; }
      
      // ------- Neighbor Access (CPU version) -------
      Cell& getNeighbor(int i);
  };

  // -------------------------------------------------------------------
  // 4D Lattice Indexing (GPU-Optimized Memory Layout)
  // -------------------------------------------------------------------
  
  /**
   * getCell - Access cell in 4D lattice
   * 
   * Memory layout: w * L3 + z * L2 + y * EL + x
   * This layout optimizes GPU coalesced memory access patterns.
   * 
   * @param lattice - 1D vector representing 4D grid
   * @param x, y, z - 3D spatial coordinates
   * @param w - 4th dimension coordinate (W-layer)
   * @return Reference to Cell at (x, y, z, w)
   */
  inline Cell& getCell(std::vector<Cell>& lattice, int x, int y, int z, int w)
  {
    unsigned L2 = EL * EL;
    unsigned L3 = L2 * EL;
    return lattice[w * L3 + z * L2 + y * EL + x];
  }

  inline const Cell& getCell(const std::vector<Cell>& lattice, int x, int y, int z, int w)
  {
    unsigned L2 = EL * EL;
    unsigned L3 = L2 * EL;
    return lattice[w * L3 + z * L2 + y * EL + x];
  }

  // -------------------------------------------------------------------
  // Timing Constants (Simulation Phase Boundaries)
  // -------------------------------------------------------------------
  
  extern unsigned DIAG;         // Grid diagonal length
  extern unsigned RMAX;         // Maximum propagation radius
  extern unsigned CONTRACT;     // Contraction threshold
  
  // Phase timing slots
  extern unsigned CONVOL;       // Convolution phase end
  extern unsigned SLOT1;        // Time slot 1 boundary
  extern unsigned SLOT2;        // Time slot 2 boundary
  extern unsigned SLOT3;        // Time slot 3 boundary
  extern unsigned SLOT4;        // Time slot 4 boundary
  extern unsigned DIFFUSION;    // Diffusion phase end
  extern unsigned SLOT5;        // Time slot 5 boundary
  extern unsigned SLOT6;        // Time slot 6 boundary
  extern unsigned SLOT7;        // Time slot 7 boundary
  extern unsigned SLOT8;        // Time slot 8 boundary
  extern unsigned RELOC;        // Relocation phase end
  extern unsigned REISSUE;      // Reissue phase end
  extern unsigned FLOOD;        // Flood fill phase end
  extern unsigned FRAME;        // Complete frame duration

  // -------------------------------------------------------------------
  // Utility Functions
  // -------------------------------------------------------------------
  
  /**
   * EQUAL - Test vector equality
   */
  inline bool EQUAL(unsigned v1[3], unsigned v2[3])
  {
    return (v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]);
  }

  /**
   * neutralColor - Test color charge neutralization
   * Returns true if two cells' color charges combine to white/neutral
   */
  bool neutralColor(Cell &a, Cell &b);
  
  /**
   * neutralWeak - Test weak force neutralization
   */
  bool neutralWeak(Cell &a, Cell &b);

  // -------------------------------------------------------------------
  // Initialization & Configuration
  // -------------------------------------------------------------------
  
  void calculateParameters(unsigned L, unsigned W);
  bool initSimulation(int step);
  void initSpirals();
  void replicate();
  void markPoints(unsigned p[3], int w);
  bool tryAllocate(int EL, int W);
  
  // -------------------------------------------------------------------
  // Utilities
  // -------------------------------------------------------------------
  
  void updateBuffer();
  std::vector<std::tuple<int, int, int>> generateShell(int L);
  void normalize(double vec[3]);
  void cross_product(double result[3], const double a[3], const double b[3]);
  void printLattice(int w);
  void printParams();
  unsigned int getRandomUnsigned(unsigned int modulus);
  void relocateGlobal(unsigned dx, unsigned dy, unsigned dz);
  
  // -------------------------------------------------------------------
  // CA Rules & Interaction Functions
  // -------------------------------------------------------------------
  
  // Convolution rules (scenario-specific)
  bool convolute(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute0(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute1(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute2(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute3(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute4(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute5(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute6(Cell& curr, Cell &draft, Cell &mirror);
  bool convolute7(Cell& curr, Cell &draft, Cell &mirror);
  
  // Time slot rules
  void diffuse(Cell& curr, Cell &draft, Cell &forward, 
               Cell &north, Cell &west, Cell &down, 
               Cell &south, Cell &east, Cell &up);
  void relocate(Cell& curr, Cell &draft, 
                Cell &north, Cell &west, Cell &down);
  void reissue(Cell& curr, Cell &draft, Cell &forward,
               Cell &north, Cell &west, Cell &down,
               Cell &south, Cell &east, Cell &up);
  void flood(Cell& curr, Cell &draft, Cell &forward,
             Cell &north, Cell &west, Cell &down,
             Cell &south, Cell &east, Cell &up);
  
  // Mirror management
  void shiftMirror();
  
  // -------------------------------------------------------------------
  // Simulation Control & Update
  // -------------------------------------------------------------------
  
  void* SimulationLoop();
  void DeleteAutomaton();
  bool swap_lattices();
  void update();
  bool simulation();
  
  // -------------------------------------------------------------------
  // Testing & Validation
  // -------------------------------------------------------------------
  
  bool sanityTest();
  bool sanityTest2();
  bool sanityTest3();

  // ===================================================================
  // CUDA GPU INTERFACE
  // ===================================================================
  
  #ifdef USE_CUDA
  
  /**
   * initGPU - Allocate GPU memory and initialize CUDA context
   * 
   * @param EL - Grid edge length
   * @param W_USED - Number of W-layers
   */
  void initGPU(unsigned EL, unsigned W_USED);
  
  /**
   * setSimulationParameters - Configure simulation timing constants
   * 
   * Calculates and uploads all phase boundaries to GPU constant memory.
   * Must be called after initGPU() and before simulation starts.
   * 
   * @param EL - Grid edge length
   * @param W_USED - Number of W-layers
   * @param scenario - Scenario ID (selects convolution ruleset)
   */
  void setSimulationParameters(unsigned EL, unsigned W_USED, int scenario);
  
  /**
   * initGPULatticeState - Initialize lattice state on GPU
   * 
   * Sets up initial particle configuration and copies data to device.
   * Must be called after setSimulationParameters().
   */
  void initGPULatticeState();
  
  /**
   * runSimulationSteps - Execute CA update steps on GPU
   * 
   * Runs the complete update cycle (convolution → diffusion → 
   * relocation → reissue → flood) for the specified number of steps.
   * 
   * @param numSteps - Number of update cycles to execute
   */
  void runSimulationSteps(int numSteps);
  
  /**
   * getLayerData - Copy single W-layer from GPU to host
   * 
   * Retrieves a 3D slice of the 4D lattice for visualization.
   * 
   * @param selectedW - W-layer index to retrieve
   * @param host_layer_data - Output vector (resized automatically)
   */
  void getLayerData(unsigned selectedW, std::vector<Cell>& host_layer_data);
  
  /**
   * getLayerDataCPU - CPU fallback for layer data retrieval
   */
  void getLayerDataCPU(unsigned selectedW, std::vector<Cell>& host_layer_data);
  
  /**
   * setupHostBuffers - Initialize host-side buffers (debugging)
   */
  void setupHostBuffers();
  
  /**
   * cleanupGPU - Free all GPU resources
   * 
   * Must be called before program exit to prevent memory leaks.
   */
  void cleanupGPU();
  
  #endif // USE_CUDA

} // namespace automaton

// ===================================================================
// GLOBAL LATTICE STORAGE
// ===================================================================

/**
 * Primary lattices (double-buffered for GPU computation)
 * 
 * - lattice_curr: Current state (read by kernels)
 * - lattice_mirror: Mirror/history state (for convolution phase)
 */
extern std::vector<automaton::Cell> lattice_curr;
extern std::vector<automaton::Cell> lattice_draft;
extern std::vector<automaton::Cell> lattice_mirror;

namespace automaton
{
  // Namespace-level references to global lattices for compatibility
  using ::lattice_curr;
  using ::lattice_draft;
  using ::lattice_mirror;
}

#endif /* SIMULATION_H_ */