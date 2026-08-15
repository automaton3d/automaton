/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "model/simulation.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cassert>
#include "globals.h"
#include "layers.h"

namespace automaton
{
  using namespace std;

  // Global variables for lattice
  extern std::vector<Cell> lattice_curr;
  extern std::vector<Cell> lattice_draft;
  extern std::vector<Cell> lattice_mirror;

  extern std::vector<std::array<unsigned, 3>> lcenters;

  inline size_t index(unsigned x, unsigned y, unsigned z, unsigned w)
  {
    return (((size_t)w * EL + x) * EL + y) * EL + z;
  }

  /**
   * Function to initialize the lattice with general data.
   */
void initGeneral()
{
    const double R = EL / 2.0;
    
    printf("initGeneral: EL=%u, RMAX=%u\n", EL, RMAX);

    // Reset pulsating sphere tick counter
    pulse_tick = 0;
    
    for (unsigned w = 0; w < W_USED; ++w)
    {
        // Layer-specific center
        unsigned cx = lcenters[w][0];
        unsigned cy = lcenters[w][1];
        unsigned cz = lcenters[w][2];
        
        printf("  Layer %u: center=(%u,%u,%u)\n", w, cx, cy, cz);
        
        for (unsigned x = 0; x < EL; ++x)
        {
            for (unsigned y = 0; y < EL; ++y)
            {
                for (unsigned z = 0; z < EL; ++z)
                {
                    Cell& cell = getCell(lattice_curr, x, y, z, w);
                    
                    // Basic configuration
                    cell.w = static_cast<WIndex>(w);
                    cell.leader_w = NO_LEADER_W;
                    cell.is_core = false;

                    char w0 = w % 2;
                    char w1 = (w >> 1) % 2;
                    char q = w0 ^ w1;
                    
                    cell.ch = (w % 8) | (q << 3) | (w0 << 4) | (w1 << 5);
                    cell.x[0] = x;
                    cell.x[1] = y;
                    cell.x[2] = z;
                    cell.x[3] = w;
                    
                    // Calculate squared distance from layer center
                    int dx = (int)x - (int)cx;
                    int dy = (int)y - (int)cy;
                    int dz = (int)z - (int)cz;
                    unsigned int dist_r2 = dx*dx + dy*dy + dz*dz;
                    unsigned int R2 = RMAX * RMAX;
                    
                    if (dist_r2 <= R2) {
                        cell.a = w;
                    } else {
                        cell.a = W_USED;  // Orphan outside sphere
                    }
                    
                    // Initialize r2 (squared distance from center, integer only)
                    cell.r2 = dist_r2;
                    cell.r  = isqrt((int)dist_r2);
                    cell.u = 0;
                    cell.v = 0;
                    if (cell.r == 0)
                        cell.u = 2048;  // seed the central wave source
                    cell.active = 0;

                    // Initialize flags
                    cell.pB = false;
                    cell.sB = false;
                    cell.phiB = false;
                    cell.t = 0;
                    cell.f = 0;
                    cell.s2B = false;
                    cell.kB = false;
                    cell.bB = false;
                    cell.hB = false;
                    cell.cB = false;
                    cell.c[0] = 0;
                    cell.c[1] = 0;
                    cell.c[2] = 0;

                    // Spin-rev source model: default to singleton,
                    // layer 0 is the initial K (chief) source.
                    cell.kind       = (w == 0 ? SourceKind::K : SourceKind::S);
                    cell.parent     = NO_PARENT;
                    cell.spin_target= 0;
                    cell.pair_idx   = NO_PAIR;

                    // Every hosted bubble has an immutable momentum vector M.
                    if (cell.r == 0) {
                        int axis  = (int)(w % 3u);
                        int sign  = ((w / 3u) % 2u == 0u) ? +1 : -1;
                        cell.m[0] = (axis == 0) ? sign : 0;
                        cell.m[1] = (axis == 1) ? sign : 0;
                        cell.m[2] = (axis == 2) ? sign : 0;
                    } else {
                        cell.m[0] = cell.m[1] = cell.m[2] = 0;
                    }
                }
            }
        }
    }
    
    puts("initGeneral ok.");
}

  /*
   * Replicate data to draft and mirror
   */
  void replicate()
  {
    std::copy(lattice_curr.begin(), lattice_curr.begin() + BLOCK, lattice_draft.begin());
    std::copy(lattice_curr.begin(), lattice_curr.begin() + BLOCK, lattice_mirror.begin());
    puts("replicate ok.");
  }

  /**
   * Executes initialization steps
   */
  bool initSimulation(int step)
  {
    switch(step)
    {
      case 0:
        initGeneral();
        break;
        
      case 1:
      case 2:
      case 3:
        // Deprecated: static momentum/spiral/sine initialisation removed;
        // polarisation and active wavefront now emerge from phase_step().
        break;
        
      case 4:
        printParams();
        break;
        
      case 5:
        // Previously used for debug topological relocation; removed.
        break;
        
      case 6:
        replicate();
        break;
        
      case 7:
        assert(sanityTest());
        break;
        
      default:
        return true;
    }
    
    return false;
  }

  /**
   * Tentative allocation
   */
  bool tryAllocate(int EL, int W)
  {
    try
    {
      size_t total = static_cast<size_t>(EL) * EL * EL * W;
      
      lattice_curr.resize(total);
      lattice_draft.resize(total);
      lattice_mirror.resize(total);
      
      const size_t totalVoxels = static_cast<size_t>(EL) * EL * EL;
      
      if (voxels.size() != totalVoxels)
        voxels.resize(totalVoxels);
      
      printf("tryAllocate: allocated %zu cells (%u^3 * %u)\n", total, EL, W);
      
      return true;
    }
    catch (const std::bad_alloc& e)
    {
      lastAllocationError = "Memory allocation failed: " + std::string(e.what());
      std::cerr << lastAllocationError << std::endl;
      return false;
    }
    catch (...)
    {
      lastAllocationError = "Unknown error during memory allocation";
      std::cerr << lastAllocationError << std::endl;
      return false;
    }
  }

void initCenters(unsigned wDim)
{
    lcenters.resize(wDim);

    // All bubbles centered at lattice center in all layers
    for (unsigned w = 0; w < wDim; ++w)
    {
        lcenters[w][0] = CENTER;
        lcenters[w][1] = CENTER;
        lcenters[w][2] = CENTER;

        printf("initCenters: w=%u, center=(%u,%u,%u)\n", w, CENTER, CENTER, CENTER);
    }
}

  /**
   * Calculates parameters
   */
  void calculateParameters(unsigned L, unsigned W)
  {
    EL        = L;
    L2        = (EL * EL);
    W_DIM     = (3 * L2 + 1);
    W_USED    = W;
    L3        = L2 * EL;
    ORDER     = ((int)round(log2(EL)));
    CENTER    = ((EL - 1) / 2);
    FCENTER   = (EL / 2.0);
    BLOCK     = L3 * W_USED;
    DIAG      = (unsigned)EL * (unsigned)sqrt(3);
    
    RMAX      = L / 2;
    
    CONTRACT  = static_cast<int>(floor(sqrt(3.0) * CENTER));
    CONVOL    = W_USED;
    
    GSLOT_X   = CONVOL + 2 * RMAX;
    GSLOT_Y   = GSLOT_X + 2 * RMAX;
    GSLOT_Z   = GSLOT_Y + 2 * RMAX;
    
    SLOT1     = GSLOT_Z + RMAX;
    SLOT2     = SLOT1 + 3 * (EL - 1);
    SLOT3     = SLOT2 + 3 * (EL - 1);
    SLOT4     = SLOT3 + 2 * W_USED;
    SLOT5     = SLOT4 + 3 * (L - 1);
    
    DIFFUSION = SLOT5;
    
    SLOT6     = DIFFUSION + (EL - 1);
    SLOT7     = SLOT6 + (EL - 1);
    SLOT8     = SLOT7 + (EL - 1);
    
    RELOC     = SLOT8;
    REISSUE   = RELOC + 1;
    FLOOD     = REISSUE + 3 * (L - 1);
    
    FRAME     = FLOOD;
    
    printf("calculateParameters: EL=%u, W_USED=%u, RMAX=%u, CENTER=%u, FRAME=%u\n", 
           EL, W_USED, RMAX, CENTER, FRAME);
    
    initCenters(W_USED);
  }

} // namespace automaton