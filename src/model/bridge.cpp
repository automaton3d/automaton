// bridge.cpp
// Candidate 2 Updated

#include "GUI.h"
#include "simulation.h"
#include "layers.h"
#include "automaton_compute.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include "cuda.h"  // Add for CUDA if needed

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  // REMOVIDO: extern std::vector<Cell> lattice_curr; 
  // lattice_curr não é mais acessado aqui, o acesso é via a cópia da GPU.
  
  // Variável para armazenar a cópia da camada W da GPU
  std::vector<Cell> host_layer_curr;

  void setupHostBuffers()
  {
	  unsigned long long gridSize = (unsigned long long)EL * EL * EL;
        // Garante que o vetor tenha o tamanho correto ANTES de ser acessado.
	  host_layer_curr.resize(gridSize);
  }

  // ===================================================================
  // ADIÇÃO: Definição da função de acesso 3D para o vetor da CPU (host_layer_curr)
  // ===================================================================
  inline Cell& getCell(std::vector<Cell>& lattice, unsigned x, unsigned y, unsigned z)
  {
      // A indexação 3D é baseada no padrão de indexação 4D anterior, removendo 'w' e 'W_USED'.
      // Assume a ordem de indexação: ((x * EL + y) * EL + z)
      return lattice[((x * EL + y) * EL + z)];
  }

  void getLayerDataCPU(unsigned selectedW, std::vector<Cell>& lattice)
  {
      for (unsigned z = 0; z < EL; ++z) {
          for (unsigned y = 0; y < EL; ++y) {
              for (unsigned x = 0; x < EL; ++x) {
                  size_t idx_3d = (size_t)z * L2 + (size_t)y * EL + (size_t)x;
                  size_t idx_4d = (size_t)selectedW * L3 + idx_3d;

                  lattice[idx_3d] = lattice_curr[idx_4d];

                  // Optional: Ensure coordinates are correct
                  lattice[idx_3d].x[0] = x;
                  lattice[idx_3d].x[1] = y;
                  lattice[idx_3d].x[2] = z;
                  lattice[idx_3d].x[3] = selectedW;
              }
          }
      }
  }
}

extern std::vector<unsigned int> voxels; // Seu buffer de visualização

// ===================================================================
// Tomografia: verifica se o voxel deve ser desenhado (Lógica CPU Original)
// ===================================================================
static bool isVisibleInTomogram(unsigned x, unsigned y, unsigned z)
{
  // Acesso direto às globais da GUI/camada
  if (!tomoEnable || !tomoEnable->getState())
    return true;

  if (tomoDirs.empty())
    return true;

  // Assume que tomo_x, tomo_y, tomo_z, tomoDirs[i] estão acessíveis
  if (tomoDirs[0].isSelected()) return z == tomo_z; // XY plane
  if (tomoDirs[1].isSelected()) return x == tomo_x; // YZ plane
  if (tomoDirs[2].isSelected()) return y == tomo_y; // ZX plane

  return true;
}

// ===================================================================
// updateBuffer() - Versão Híbrida CPU/GPU
// ===================================================================
void automaton::updateBuffer()
{
    // 1. Determina a camada W (W-layer)
    unsigned selectedW = (framework::layerList && framework::layerList.get())
                         ? framework::layerList->getSelected()
                         : 0u;
    
    // 2. Traz a camada W selecionada da GPU para o vetor host_layer_curr na CPU
#ifdef USE_CUDA
    getLayerData(selectedW, host_layer_curr);
#else
    getLayerDataCPU(selectedW, host_layer_curr);  // Use host_layer_curr even for CPU
#endif

    // Redimensiona o buffer de voxels se necessário (from Cand1)
    if (voxels.size() != EL * EL * EL) {
        voxels.resize(EL * EL * EL);
    }

    // 3. Executa a lógica de visualização na CPU (como era originalmente)
    size_t idx = 0;
    unsigned L3 = EL * EL * EL; // Tamanho do volume 3D (from Cand1)

    for (unsigned x = 0; x < EL; ++x)
    for (unsigned y = 0; y < EL; ++y)
    for (unsigned z = 0; z < EL; ++z)
    {
        // 3a. Tomografia (Lógica CPU) - Keep but comment if issues
        if (!isVisibleInTomogram(x, y, z))
        {
            voxels[idx++] = 0x00000000u;  // Totalmente transparente
            continue;
        }

        const Cell& cell = getCell(host_layer_curr, x, y, z); 

        uint32_t color = 0x00000000u;  // Padrão = invisível

        // 3b. Mapeamento de Cores (Lógica CPU)
        // A cor só é definida se a célula estiver no *wavefront* (cell.t == cell.d)
        if (cell.t == cell.d)
        {
            // Mapeamento de cor da lógica original do bridge.cpp
            if (cell.a == W_USED) // Affinity == W_USED
                color = 0xFFFF5050u;      // Vermelho (0xAARRGGBB - Orphan)
            else if (cell.t == 0)
                color = 0xFF50FF50u;      // Verde (Source)
            else
                color = 0xFFC8C8FFu;    // Azul (Propagating Wave)
        }

        voxels[idx++] = color;
    }
} // Fim automaton::updateBuffer