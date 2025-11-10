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
    int w = framework::list->getSelected();

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
  void updateBuffer()
  {
	int w = framework::list->getSelected();
    unsigned index3D = 0;
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          bool visible = true;
          // Apply tomography filter if enabled
          if (framework::tomo && framework::tomo->getState())
          {
            if (framework::tomoDirs[0].isSelected())      // XY → fix Z
              visible = (z == framework::tomo_z);
            else if (framework::tomoDirs[1].isSelected()) // YZ → fix X
              visible = (x == framework::tomo_x);
            else if (framework::tomoDirs[2].isSelected()) // ZX → fix Y
              visible = (y == framework::tomo_y);
          }
          if (!visible)
          {
            voxels[index3D++] = RGB(0, 0, 0);  // Black for hidden voxels
            continue;
          }
          Cell &cell = getCell(lattice_curr, x, y, z, w);
          if (cell.t == cell.d)
          {
            // Select wavefront cells
            if (cell.a == W_USED)
              voxels[index3D] = RGB(255, 0, 0);     // Red (orphan)
      	    else if (cell.t == 0)
      	      voxels[index3D] = RGB(0, 255, 0);     // Green for fresh wavefront seed
            else
              voxels[index3D] = RGB(255, 255, 255); // White (wavefront match)
          }
          else
          {
            voxels[index3D] = RGB(0, 0, 0); // Black
          }
          /*
          else if (cell.cB)
          else if (cell.t < RMAX / 2)
          {
            voxels[index3D] = RGB(100, 100, 255);   // Blue (contraction/reissue)
          }
          else
          {
            voxels[index3D] = RGB(0, 0, 0); // Black
          }
          */
          index3D++;
        }
      }
    }
  }
#endif
}
