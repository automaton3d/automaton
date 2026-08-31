/*
 * scatter_main.cpp — headless controlled-scattering experiment.
 *
 * Usage:
 *   scatter.exe [EL] [SEP] [TOTAL_FRAMES] [budget_seconds]
 *
 * Two bubbles (source centres in layers 0 and 1) are placed a distance
 * `SEP` apart along the x axis with prescribed, opposite momenta (m and
 * reloc), then left to evolve.  The runner samples, once per light frame:
 *   - source-centre positions (lcenters), kind, m, reloc, pair state;
 * and finally reports:
 *   - mean speed (cells/frame) of each source;
 *   - total momentum before and after (sum of m);
 *   - closest approach distance;
 *   - deflection angle cos(theta) for each source (m_final . m_initial);
 *   - whether a pair formed (kind==P) or a capture happened (a changed).
 */

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <vector>
#include <array>

#include "model/simulation.h"
#include "config.h"                        // Config / gConfig stub below

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

  // Mark the source centre and give the bubble a complementary charge.
  const unsigned char chA = 0x00;          // layer 0: neutral word
  const unsigned char chB = 0x3F;          // layer 1: full complement (Rule 1 pair)
  for (automaton::Cell* p :
       { &automaton::getCell(automaton::lattice_curr, (int)x, (int)y, (int)z, (int)w),
         &automaton::getCell(automaton::lattice_draft, (int)x, (int)y, (int)z, (int)w),
         &automaton::getCell(automaton::lattice_mirror, (int)x, (int)y, (int)z, (int)w) })
  {
    p->r2   = 0;
    p->r    = 0;
    p->u    = 2048;                        // central wave source amplitude
    p->v    = 0;
    p->ch   = (w == 0) ? chA : chB;
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
  unsigned EL_in  = (argc > 1) ? static_cast<unsigned>(atoi(argv[1])) : 7u;
  unsigned SEP    = (argc > 2) ? static_cast<unsigned>(atoi(argv[2])) : 4u;
  unsigned FRAMES = (argc > 3) ? static_cast<unsigned>(atoi(argv[3])) : 24u;
  double   budget = (argc > 4) ? atof(argv[4]) : 15.0;

  if (EL_in < 3 || EL_in > 31 || (EL_in % 2) == 0)
  {
    fprintf(stderr, "EL must be an odd value in [3,31]\n");
    return 1;
  }
  if (SEP < 2) SEP = 2;

  const unsigned W_in = 2u;   // exactly two bubbles

  printf("=== controlled scattering experiment ===\n");
  printf("EL=%u W_USED=2 SEP=%u RMAX=%u target=%u frames budget=%.1fs\n",
         EL_in, SEP, EL_in / 2u, FRAMES, budget);

  automaton::calculateParameters(EL_in, W_in);
  if (!automaton::tryAllocate(static_cast<int>(EL_in), static_cast<int>(W_in)))
  {
    fprintf(stderr, "allocation failed: %s\n",
            automaton::lastAllocationError.c_str());
    return 2;
  }

  // Canonical startup (seed, params, replicate, sanity), then overwrite the
  // two layers with the controlled two-bubble configuration.
  for (int step = 0; step <= 7; ++step)
    automaton::initSimulation(step);

  const unsigned C = automaton::CENTER;
  const unsigned off = SEP / 2u;

  placeSource(0, C - off, C, C,  +1, 0, 0);   // left bubble, moving right
  placeSource(1, C + off, C, C,  -1, 0, 0);   // right bubble, moving left

  // Per-frame record.
  struct Snap { double x0, y0, z0, x1, y1, z1; int k0, k1; unsigned pc0, pc1; };
  std::vector<Snap> trail;

  const auto t0 = std::chrono::steady_clock::now();
  unsigned frame = 0;
  unsigned long long ticks = 0;

  while (frame < FRAMES)
  {
    const bool newFrame = automaton::simulation();
    ++ticks;
    if (newFrame)
    {
      ++frame;
      const std::array<unsigned, 3>& p0 = automaton::lcenters[0];
      const std::array<unsigned, 3>& p1 = automaton::lcenters[1];
      const automaton::Cell& s0 = automaton::getCell(automaton::lattice_curr,
                                          (int)p0[0], (int)p0[1], (int)p0[2], 0);
      const automaton::Cell& s1 = automaton::getCell(automaton::lattice_curr,
                                          (int)p1[0], (int)p1[1], (int)p1[2], 1);
      Snap sn;
      sn.x0 = (double)p0[0]; sn.y0 = (double)p0[1]; sn.z0 = (double)p0[2];
      sn.x1 = (double)p1[0]; sn.y1 = (double)p1[1]; sn.z1 = (double)p1[2];
      sn.k0 = (int)s0.kind;  sn.k1 = (int)s1.kind;
      sn.pc0 = s0.pair_count; sn.pc1 = s1.pair_count;
      trail.push_back(sn);

      const double secs = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - t0).count();
      if (secs >= budget)
      {
        fprintf(stderr, "[scatter] budget hit at frame %u/%u\n", frame, FRAMES);
        break;
      }
    }
  }

  if (trail.empty())
  {
    fprintf(stderr, "no frames sampled\n");
    return 3;
  }

  // ---- report -------------------------------------------------------------
  printf("\n==============================================================\n");
  printf("SCATTERING REPORT  (EL=%u RMAX=%u frames=%u)\n",
         EL_in, automaton::RMAX, (unsigned)trail.size());
  printf("==============================================================\n");

  printf("frame    x0  y0  z0     x1  y1  z1    kind0 kind1  pc0 pc1\n");
  const size_t step = (trail.size() > 24) ? trail.size() / 24 : 1;
  for (size_t i = 0; i < trail.size(); i += step)
  {
    const Snap& s = trail[i];
    printf("%4u  %3.0f %3.0f %3.0f   %3.0f %3.0f %3.0f    %5d %5d  %2u %2u\n",
           (unsigned)i, s.x0, s.y0, s.z0, s.x1, s.y1, s.z1, s.k0, s.k1,
           s.pc0, s.pc1);
  }
  const Snap& s_last = trail.back();
  printf("%4u  %3.0f %3.0f %3.0f   %3.0f %3.0f %3.0f    %5d %5d  %2u %2u\n",
         (unsigned)(trail.size() - 1), s_last.x0, s_last.y0, s_last.z0,
         s_last.x1, s_last.y1, s_last.z1, s_last.k0, s_last.k1,
         s_last.pc0, s_last.pc1);

  // Mean speed (shortest toroidal displacement per frame).
  const auto wrap = [&](int a, int b) {
    int d = b - a;
    if (d > (int)EL_in / 2) d -= (int)EL_in;
    else if (d < -(int)EL_in / 2) d += (int)EL_in;
    return d;
  };
  const Snap& f0 = trail.front();
  const Snap& f1 = trail.back();
  const double nf = (double)(trail.size() - 1);
  const int wx0 = wrap((int)f0.x0, (int)f1.x0);
  const int wy0 = wrap((int)f0.y0, (int)f1.y0);
  const int wz0 = wrap((int)f0.z0, (int)f1.z0);
  const int wx1 = wrap((int)f0.x1, (int)f1.x1);
  const int wy1 = wrap((int)f0.y1, (int)f1.y1);
  const int wz1 = wrap((int)f0.z1, (int)f1.z1);
  const double d0 = std::sqrt((double)(wx0*wx0 + wy0*wy0 + wz0*wz0));
  const double d1 = std::sqrt((double)(wx1*wx1 + wy1*wy1 + wz1*wz1));
  printf("\nmean speed:  bubble0 = %.3f cells/frame,  bubble1 = %.3f cells/frame\n",
         d0 / nf, d1 / nf);

  // Closest approach.
  double minD = 1e30;
  for (const Snap& s : trail)
  {
    const double dx = s.x0 - s.x1, dy = s.y0 - s.y1, dz = s.z0 - s.z1;
    const double dd = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dd < minD) minD = dd;
  }
  printf("closest approach: %.2f cells (initial separation %u)\n", minD, SEP);

  // Pair formation.
  bool anyP = false, anyPair = false;
  for (const Snap& s : trail)
  {
    if (s.k0 == (int)automaton::SourceKind::P ||
        s.k1 == (int)automaton::SourceKind::P) anyP = true;
    if (s.pc0 > 0 || s.pc1 > 0) anyPair = true;
  }
  printf("pair formed (kind==P): %s\n", anyP ? "yes" : "no");
  printf("pair stack (pair_count>0): %s\n", anyPair ? "yes" : "no");

  // Convolution diagnostics from interaction.cpp counters.
  printf("convolve: active-passes=%lld  s2B-passes=%lld  pairs-formed=%lld  self-guard=%lld\n",
         (long long)automaton::conv_calls, (long long)automaton::conv_s2b,
         (long long)automaton::conv_pair, (long long)automaton::conv_self);

  // Momentum and deflection at first vs last frame.
  {
    const automaton::Cell& s0i = automaton::getCell(automaton::lattice_curr,
        (int)f0.x0, (int)f0.y0, (int)f0.z0, 0);
    const automaton::Cell& s1i = automaton::getCell(automaton::lattice_curr,
        (int)f0.x1, (int)f0.y1, (int)f0.z1, 1);
    const automaton::Cell& s0f = automaton::getCell(automaton::lattice_curr,
        (int)f1.x0, (int)f1.y0, (int)f1.z0, 0);
    const automaton::Cell& s1f = automaton::getCell(automaton::lattice_curr,
        (int)f1.x1, (int)f1.y1, (int)f1.z1, 1);
    const int px_i = s0i.m[0] + s1i.m[0], py_i = s0i.m[1] + s1i.m[1], pz_i = s0i.m[2] + s1i.m[2];
    const int px_f = s0f.m[0] + s1f.m[0], py_f = s0f.m[1] + s1f.m[1], pz_f = s0f.m[2] + s1f.m[2];
    printf("momentum (sum of m):  initial=(%+d,%+d,%+d)  final=(%+d,%+d,%+d)\n",
           px_i, py_i, pz_i, px_f, py_f, pz_f);
    const double dot0 = (double)(s0i.m[0]*s0f.m[0] + s0i.m[1]*s0f.m[1] + s0i.m[2]*s0f.m[2]);
    const double dot1 = (double)(s1i.m[0]*s1f.m[0] + s1i.m[1]*s1f.m[1] + s1i.m[2]*s1f.m[2]);
    printf("deflection cos(theta):  bubble0 = %+.2f,  bubble1 = %+.2f\n",
           dot0, dot1);
  }

  printf("--------------------------------------------------------------\n");
  return 0;
}

