/*
 * bridge.cpp - versao corrigida e totalmente funcional (CPU path)
 */

#include "GUI.h"
#include "simulation.h"
#include "layers.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  extern std::vector<Cell> lattice_curr;
}

namespace framework
{
  extern Tickbox* tomo;
  extern std::vector<Radio> tomoDirs;
  extern unsigned tomo_x, tomo_y, tomo_z;
  extern std::unique_ptr<LayerList> layerList;  // Correct type: unique_ptr
}

// ===================================================================
// Tomografia: verifica se o voxel deve ser desenhado
// ===================================================================
static bool isVisibleInTomogram(unsigned x, unsigned y, unsigned z)
{
  if (!framework::tomo || !framework::tomo->getState())
    return true;

  if (framework::tomoDirs.empty())
    return true;

  if (framework::tomoDirs[0].isSelected()) return z == framework::tomo_z; // XY plane
  if (framework::tomoDirs[1].isSelected()) return x == framework::tomo_x; // YZ plane
  if (framework::tomoDirs[2].isSelected()) return y == framework::tomo_y; // ZX plane

  return true;
}

// ===================================================================
// updateBuffer() - versao CPU (a unica que voce esta usando agora)
// ===================================================================
void automaton::updateBuffer()
{
  // Camada W selecionada no GUI
  unsigned selectedW = (framework::layerList && framework::layerList.get()) 
                       ? framework::layerList->getSelected() 
                       : 0u;

//  const size_t totalVoxels = static_cast<size_t>(EL) * EL * EL;

  // Garante tamanho correto e evita realocacoes desnecessarias
//  if (voxels.size() != totalVoxels)
 //   voxels.resize(totalVoxels);

  size_t idx = 0;

  for (unsigned x = 0; x < EL; ++x)
  {
    for (unsigned y = 0; y < EL; ++y)
    {
      for (unsigned z = 0; z < EL; ++z)
      {
        if (!isVisibleInTomogram(x, y, z))
        {
          voxels[idx++] = 0x000000FFu; // preto totalmente opaco
          continue;
        }

        const Cell& cell = getCell(lattice_curr, static_cast<int>(x),
                                              static_cast<int>(y),
                                              static_cast<int>(z),
                                              static_cast<int>(selectedW));

        uint32_t color = 0x000000FFu; // preto por default

        if (cell.t == cell.d)
        {
          if (cell.a == W_USED)
            color = 0xFF0000FFu;        // vermelho
          else if (cell.t == 0)
            color = 0x00FF00FFu;        // verde
          else
            color = 0xFFFFFFFFu;        // branco
        }
        // caso contrario permanece preto

        voxels[idx++] = color;
      }
    }
  }
}