/*
 * lorentz_mm.cpp — Michelson-Morley-style test of the active-shell speed.
 *
 * Interpretation (manuscript "maximum information speed"): the constant
 * wavefront expansion is v_max = the maximum speed of information
 * transmission, NOT the speed of light (which is emergent and
 * observer-dependent).  Lorentz symmetry is sought through the two
 * Mansouri-Sexl conditions: (1) two-way isotropy and (2) velocity
 * independence of the emitted light front.  This runner measures the
 * active-shell front advance (cells per frame) for a source displaced /
 * re-emitted by a momentum step V, against a rest control, along a chosen
 * axis.  A constant-speed front must advance exactly one cell per frame
 * with zero variance (mean = 1, std = 0), independent of V.
 *
 * Usage:
 *   lorentz_mm.exe [EL] [V] [AXIS] [FRAMES] [budget_seconds]
 *     EL     odd lattice side in [3,31]        (default 7)
 *     V      signed momentum step on the axis  (default +1; 0 = rest)
 *     AXIS   0 = x, 1 = y, 2 = z, 3 = x+y diagonal  (default 0)
 *     FRAMES number of light frames            (default 40)
 *     budget wall-clock budget in seconds      (default 15)
 */

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <array>
#include <vector>

#include "model/simulation.h"
#include "model/wavefront.h"
#include "config.h"

// ---------------------------------------------------------------------------
// Link stubs: globals normally defined in src/globals.cpp / config.cpp.
// ---------------------------------------------------------------------------
std::vector<unsigned int> voxels;          // used by tryAllocate() (initSim.cpp)
Config gConfig;                            // used by simulation.cpp delay gates

namespace automaton
{
  bool convol_delay  = false;              // declared in model/simulation.h
  bool diffuse_delay = false;
  bool reloc_delay   = false;

  // forward-declared: defined in simulation.cpp, not exposed in simulation.h
  void trackCenter(unsigned x, unsigned y, unsigned z, unsigned w);
}

// ---------------------------------------------------------------------------
// Reset layer w (all cells to "unreached"), then place its source centre at
// (x,y,z) with prescribed momentum m and impulse reloc.  curr/draft/mirror
// are all brought to the same state so the first tick sees a clean sphere.
// ---------------------------------------------------------------------------
static void placeSource(unsigned w, unsigned x, unsigned y, unsigned z,
                        int mx, int my, int mz)
{
  const int ELi = (int)automaton::EL;

  for (int a = 0; a < ELi; ++a)
    for (int b = 0; b < ELi; ++b)
      for (int c = 0; c < ELi; ++c)
      {
        automaton::Cell& cc = automaton::getCell(
            automaton::lattice_curr, a, b, c, (int)w);
        automaton::Cell& cd = automaton::getCell(
            automaton::lattice_draft, a, b, c, (int)w);
        automaton::Cell& cm = automaton::getCell(
            automaton::lattice_mirror, a, b, c, (int)w);
        for (automaton::Cell* p : { &cc, &cd, &cm })
        {
          p->r2  = INF_R2;
          p->r   = -1;
          p->u   = 0;
          p->v   = 0;
          p->active = 0;
          p->pol_u = p->pol_v = 0;
          p->bstamp = 0;
          p->m[0] = p->m[1] = p->m[2] = 0;
          p->reloc[0] = p->reloc[1] = p->reloc[2] = 0;
          p->kind = automaton::SourceKind::S;
          p->pair_idx   = automaton::NO_PAIR;
          p->pair_count = 0;
          p->parent     = automaton::NO_PARENT;
          p->leader_w   = automaton::NO_LEADER_W;
          p->a          = automaton::W_USED;   // orphan: no pre-existing island
          p->t = 0; p->f = 0;
          p->pB = p->sB = p->s2B = false;
          p->kB = p->bB = p->hB = p->cB = false;
          p->gB = false;
          p->c[0] = p->c[1] = p->c[2] = 0;
          p->g[0] = p->g[1] = p->g[2] = 0;
          p->spin_target = 0;
        }
      }

  // Source centre: r2 = 0, neutral charge word, prescribed momentum.
  for (automaton::Cell* p :
       { &automaton::getCell(automaton::lattice_curr, (int)x, (int)y, (int)z, (int)w),
         &automaton::getCell(automaton::lattice_draft, (int)x, (int)y, (int)z, (int)w),
         &automaton::getCell(automaton::lattice_mirror, (int)x, (int)y, (int)z, (int)w) })
  {
    p->r2   = 0;
    p->r    = 0;
    p->u    = 2048;                        // central wave source amplitude
    p->v    = 0;
    p->ch   = 0x00;                        // neutral (no pair with itself)
    p->m[0] = mx; p->m[1] = my; p->m[2] = mz;
    p->reloc[0] = mx; p->reloc[1] = my; p->reloc[2] = mz;
    p->t = 0; p->f = 0;
  }

  automaton::lcenters[w][0] = x;
  automaton::lcenters[w][1] = y;
  automaton::lcenters[w][2] = z;
  automaton::trackCenter(x, y, z, w);
}


int main(int argc, char** argv)
{
  unsigned EL_in  = (argc > 1) ? (unsigned)atoi(argv[1]) : 7u;
  int      V      = (argc > 2) ? atoi(argv[2]) : 1;
  unsigned AXIS   = (argc > 3) ? (unsigned)atoi(argv[3]) : 0u;
  unsigned FRAMES = (argc > 4) ? (unsigned)atoi(argv[4]) : 40u;
  double   budget = (argc > 5) ? atof(argv[5]) : 15.0;

  if (EL_in < 3 || EL_in > 31 || (EL_in % 2) == 0)
  {
    fprintf(stderr, "EL must be an odd value in [3,31]\n");
    return 1;
  }
  if (AXIS > 3) AXIS = 0;

  const unsigned W_in = 2u;   // layer 0 = moving source, layer 1 = rest control

  printf("=== Michelson-Morley: active-shell speed vs source motion ===\n");
  printf("EL=%u W_USED=2 V=%+d axis=%u RMAX=%u frames=%u budget=%.1fs\n",
         EL_in, V, AXIS, EL_in / 2u, FRAMES, budget);

  automaton::calculateParameters(EL_in, W_in);
  if (!automaton::tryAllocate((int)EL_in, (int)W_in))
  {
    fprintf(stderr, "allocation failed: %s\n",
            automaton::lastAllocationError.c_str());
    return 2;
  }

  // Canonical startup, then overwrite the two layers with the controlled
  // configuration (same pattern as tests/scatter_main.cpp).
  for (int step = 0; step <= 7; ++step)
    automaton::initSimulation(step);

  const unsigned C = automaton::CENTER;

  // Layer 0: moving source at the centre with momentum V along the axis.
  int mx = 0, my = 0, mz = 0;
  if      (AXIS == 0) mx = V;
  else if (AXIS == 1) my = V;
  else if (AXIS == 2) mz = V;
  else              { mx = V; my = V; }   // x+y diagonal

  placeSource(0, C, C, C, mx, my, mz);

  // Layer 1: rest control, far from the moving path.
  placeSource(1, C, C, (C + 3u) % EL_in, 0, 0, 0);

  automaton::wavefront::begin();

  const auto t0 = std::chrono::steady_clock::now();
  unsigned frame = 0;
  unsigned long long ticks = 0;

  printf("frame    src0 (x,y,z)          src1 (x,y,z)\n");
  while (frame < FRAMES)
  {
    const bool newFrame = automaton::simulation();
    ++ticks;
    if (newFrame)
    {
      ++frame;
      automaton::wavefront::sampleFrame(frame);

      const std::array<unsigned, 3>& p0 = automaton::lcenters[0];
      const std::array<unsigned, 3>& p1 = automaton::lcenters[1];
      if (frame <= 2 || (frame % 5) == 0)
        printf("%4u    (%u,%u,%u)          (%u,%u,%u)\n",
               frame, p0[0], p0[1], p0[2], p1[0], p1[1], p1[2]);

      const double secs = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - t0).count();
      if (secs >= budget)
      {
        fprintf(stderr, "[mm] budget hit at frame %u/%u\n", frame, FRAMES);
        break;
      }
    }
  }

  automaton::wavefront::report();
  printf("--------------------------------------------------------------\n");
  return 0;
}
