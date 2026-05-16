/*
 * simulation.cpp
 * Implements the main functionality of the FSM.
 */

#include <thread>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <array>
#include "model/simulation.h"
#include "config.h"

#ifdef USE_CUDA
extern void cudaSimulationStepWrapper();
extern bool isCudaEnabled();
extern void updateMirrorOnGPU();
#endif

namespace automaton
{
  using namespace std;

  // Grid constants
  unsigned EL;
  unsigned W_DIM;
  unsigned W_USED;
  unsigned L2;
  unsigned L3 = 0;
  unsigned long BLOCK = 0;
  unsigned DIAG = 0;
  unsigned RMAX = 0;
  unsigned CONTRACT = 0;
  unsigned UPDATE = 0;
  unsigned CONVOL = 0;
  unsigned GSLOT_X = 0, GSLOT_Y = 0, GSLOT_Z = 0;
  unsigned SLOT1 = 0, SLOT2 = 0, SLOT3 = 0, SLOT4 = 0, SLOT5 = 0;
  unsigned SLOT6 = 0, SLOT7 = 0, SLOT8 = 0;
  unsigned DIFFUSION = 0;
  unsigned RELOC = 0;
  unsigned REISSUE = 0;
  unsigned FLOOD = 0;
  unsigned FRAME = 0;
  unsigned ORDER;
  unsigned CENTER;
  unsigned FCENTER;

  // Lattices
  std::vector<Cell> lattice_curr;
  std::vector<Cell> lattice_draft;
  std::vector<Cell> lattice_mirror;

  string lastAllocationError;
  std::vector<std::array<unsigned, 3>> lcenters;

  // ============================================================
  // WRAPPING FUNCTIONS
  // ============================================================

  // Spherical (antipodal) wrapping – used for wave propagation
  inline void spherical_wrap(int& x, int& y, int& z, int& w)
  {
    if (x < 0 || x >= (int)EL ||
        y < 0 || y >= (int)EL ||
        z < 0 || z >= (int)EL ||
        w < 0 || w >= (int)W_USED)
    {
        x = EL - 1 - x;
        y = EL - 1 - y;
        z = EL - 1 - z;
        w = W_USED - 1 - w;

        x = (x % (int)EL + EL) % EL;
        y = (y % (int)EL + EL) % EL;
        z = (z % (int)EL + EL) % EL;
        w = (w % (int)W_USED + W_USED) % W_USED;
    }
  }

  // Periodic (toroidal) wrapping – used for bubble relocation
  inline void periodic_wrap(int& x, int& y, int& z, int& w)
  {
    if (x < 0) x += EL; else if (x >= (int)EL) x -= EL;
    if (y < 0) y += EL; else if (y >= (int)EL) y -= EL;
    if (z < 0) z += EL; else if (z >= (int)EL) z -= EL;
    if (w < 0) w += W_USED; else if (w >= (int)W_USED) w -= W_USED;
  }

  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w)
  {
    lcenters[w][0] = x;
    lcenters[w][1] = y;
    lcenters[w][2] = z;
  }

  // ============================================================
  // CPU UPDATE
  // ============================================================

  void update_lattice_cpu()
{
    // ============================================================
    // RELOCAÇÃO BRUTA PARA O CENÁRIO 1
    // ============================================================
    static unsigned pending_vec[3] = {0,0,0};
    static bool relocation_done = false;
    
    if (gConfig.simulation.scenario == 1 && !relocation_done) {
        // Procura o vetor de deslocamento na primeira célula da camada 0
        Cell& first = getCell(lattice_curr, 0, 0, 0, 0);
        if (first.c[0] != 0 || first.c[1] != 0 || first.c[2] != 0) {
            pending_vec[0] = first.c[0];
            pending_vec[1] = first.c[1];
            pending_vec[2] = first.c[2];
            
            // Aplica o deslocamento a TODAS as células da camada 0
            for (unsigned x = 0; x < EL; ++x) {
                for (unsigned y = 0; y < EL; ++y) {
                    for (unsigned z = 0; z < EL; ++z) {
                        Cell& src = getCell(lattice_curr, x, y, z, 0);
                        Cell& dst = getCell(lattice_draft, x, y, z, 0);
                        
                        int nx = (int)x + pending_vec[0];
                        int ny = (int)y + pending_vec[1];
                        int nz = (int)z + pending_vec[2];
                        int nw = 0;
                        periodic_wrap(nx, ny, nz, nw);
                        
                        dst = src;
                        dst.x[0] = nx;
                        dst.x[1] = ny;
                        dst.x[2] = nz;
                        dst.x[3] = nw;
                    }
                }
            }
            relocation_done = true;
        }
    }
    
    // ============================================================
    // LOOP PRINCIPAL (todas as camadas)
    // ============================================================
    for (unsigned w = 0; w < W_USED; ++w)
    {
        if (w == 0)
        {
            const Cell& first = lattice_curr.front();
            if (gConfig.delays.convol && first.k < CONVOL)
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
            else if (diffuse_delay && first.k >= CONVOL && first.k < DIFFUSION)
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            else if (reloc_delay && first.k >= DIFFUSION && first.k < RELOC)
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }

        for (unsigned x = 0; x < EL; ++x)
        for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
        {
            Cell &curr   = getCell(lattice_curr, x, y, z, w);
            Cell &draft  = getCell(lattice_draft, x, y, z, w);
            Cell &mirror = getCell(lattice_mirror, x, y, z, w);

            // Se o cenário for 1 e a realocação já foi aplicada, não processa relocate novamente
            // (as células já foram movidas no draft)
            if (gConfig.simulation.scenario == 1 && relocation_done) {
                // Apenas copia as coordenadas que já foram ajustadas no draft
                // Não precisa fazer nada aqui, pois draft já está correto
            } else {
                draft = curr;
            }

            // Ensure correct coordinates
            curr.x[0] = x;
            curr.x[1] = y;
            curr.x[2] = z;
            curr.x[3] = w;

            Cell &forward = curr.getNeighbor(FORWARD);
            Cell &north   = curr.getNeighbor(NORTH);
            Cell &west    = curr.getNeighbor(WEST);
            Cell &down    = curr.getNeighbor(DOWN);
            Cell &south   = curr.getNeighbor(SOUTH);
            Cell &east    = curr.getNeighbor(EAST);
            Cell &up      = curr.getNeighbor(UP);

            if (curr.k < CONVOL) {
                convolute(curr, draft, mirror);
            } else if (curr.k < GSLOT_Z) {
                // glider slots
            } else if (curr.k < DIFFUSION) {
                diffuse(curr, draft, forward, north, west, down, south, east, up);
            } else if (curr.k < RELOC) {
                // Para o cenário 1, a realocação já foi feita no início
                if (gConfig.simulation.scenario != 1) {
                    relocate(curr, draft, north, west, down);
                }
            } else if (curr.k < REISSUE) {
                reissue(curr, draft, forward, north, west, down, south, east, up);
            } else if (curr.k < FLOOD) {
                flood(curr, draft, forward, north, west, down, south, east, up);
            }

            if (curr.d == 0) {
                trackCenter(x, y, z, w);
            }

            draft.k = (curr.k + 1) % FRAME;

            if (draft.k == 0) {
                if (curr.a == W_USED && curr.t <= RMAX)
                    draft.t++;
                else
                    draft.t = (curr.t + 1) % (2 * RMAX);
            }
        }
    }
}

  void update_lattice()
  {
#ifdef USE_CUDA
    if (isCudaEnabled()) {
        cudaSimulationStepWrapper();
        return;
    }
#endif
    update_lattice_cpu();
  }

  bool swap_lattices_cpu()
  {
    if (BLOCK == 0 || lattice_curr.empty())
        return false;

    bool newLightFrame = false;

    std::copy(
        lattice_draft.begin(),
        lattice_draft.begin() + BLOCK,
        lattice_curr.begin());

    Cell &repr = getCell(lattice_curr, 0, 0, 0, 0);

    if (repr.k == 0)
    {
      for (unsigned w = 0; w < W_USED; ++w)
      for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
      for (unsigned z = 0; z < EL; ++z)
      {
          Cell &curr = getCell(lattice_curr, x, y, z, w);
          Cell &mirror = getCell(lattice_mirror, x, y, z, w);
          mirror = curr;
          mirror.f = mirror.t;
      }
      newLightFrame = true;
    }

    if (repr.k < CONVOL)
        shiftMirror();

    return newLightFrame;
  }

  bool swap_lattices()
  {
#ifdef USE_CUDA
    if (isCudaEnabled()) {
        Cell &repr = getCell(lattice_curr, 0, 0, 0, 0);
        return (repr.k == 0);
    }
#endif
    return swap_lattices_cpu();
  }

  bool simulation()
  {
    update_lattice();
    return swap_lattices();
  }

  // ============================================================
  // Cell neighbor access – uses spherical wrap (antipodal)
  // ============================================================

  Cell &Cell::getNeighbor(int i)
  {
    static int disp[8][4] =
    {
        {+1,0,0,0},{-1,0,0,0},
        {0,+1,0,0},{0,-1,0,0},
        {0,0,+1,0},{0,0,-1,0},
        {0,0,0,+1},{0,0,0,-1}
    };

    int nx = x[0] + disp[i][0];
    int ny = x[1] + disp[i][1];
    int nz = x[2] + disp[i][2];
    int nw = x[3] + disp[i][3];

    spherical_wrap(nx, ny, nz, nw);

    return getCell(lattice_curr, nx, ny, nz, nw);
  }
} // namespace automaton