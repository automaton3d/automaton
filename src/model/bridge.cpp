/*
 * bridge.cpp
 *
 * Bridges the automaton logic to the GUI.
 */
#include <GUI.h>
#include <vector>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <set>
#include <random>
#include <chrono>
#include <thread>
#include "cuda/cuda_sim.h" // declare wrappers
#include "simulation.h"

namespace framework
{
  extern Tickbox* tomo;
  extern std::vector<Radio> tomoDirs;
  extern unsigned tomo_x, tomo_y, tomo_z;
}

// Near the top of bridge.cpp
namespace tomo_util
{
  inline bool isTomogramMatch(unsigned x, unsigned y, unsigned z) {
    using namespace framework;
    if (!tomo || !tomo->getState()) return true;
    if (tomoDirs.empty()) return true;

    // XY → fix Z
    if (tomoDirs[0].isSelected()) return z == tomo_z;
    // YZ → fix X
    if (tomoDirs[1].isSelected()) return x == tomo_x;
    // ZX → fix Y
    if (tomoDirs[2].isSelected()) return y == tomo_y;

    return true;
  }
}

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  extern std::vector<Cell> lattice_curr;

  COLORREF *voxels;

#ifdef CUDA

#include "cuda_sim.h" // declare wrappers

void updateBuffer()
{
    int w = framework::layerList->getSelected();

    // Ensure CUDA layer pointer exists
    // Launch device kernel to fill mapped voxels for given layer
    cudaUpdateVoxelsLayer((unsigned)w);

    // Get mapped voxels pointer (COLORREF layout is 0x00BBGGRR on Windows)
    uint32_t* mapped = getMappedVoxels();
    size_t n = getVoxelsSize();
    // Copy mapped (device wrote host-mapped memory) into GUI's voxels pointer
    // If your GUI expects 'voxels' pointer directly, you can set it to mapped.
    // Example: overwrite automaton::voxels pointer
    automaton::voxels = (COLORREF*) mapped;
    // If your GUI requires a separate buffer, memcpy mapped -> GUI buffer here.
}

#else

  /**
   * This is a bridge between the model and the graphical framework.
   */
// bridge.cpp (non-CUDA updateBuffer)
void updateBuffer()
{
  int w = framework::layerList->getSelected();
  unsigned index3D = 0;
  for (unsigned x = 0; x < EL; x++)
  {
    for (unsigned y = 0; y < EL; y++)
    {
      for (unsigned z = 0; z < EL; z++)
      {
        if (!tomo_util::isTomogramMatch(x, y, z))
        {
          voxels[index3D++] = RGB(0, 0, 0);
          continue;
        }

        Cell &cell = getCell(lattice_curr, x, y, z, w);
        if (cell.t == cell.d)
        {
          if (cell.a == W_USED)      voxels[index3D] = RGB(255, 0, 0);
          else if (cell.t == 0)      voxels[index3D] = RGB(0, 255, 0);
          else                        voxels[index3D] = RGB(255, 255, 255);
        }
        else
        {
          voxels[index3D] = RGB(0, 0, 0);
        }
        index3D++;
      }
    }
  }
}
#endif
}
