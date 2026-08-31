/*
 * voxel_dump.cpp — headless two-bubble experiment that dumps the active
 * voxels (wavefront cells) to stdout as plain text (CSV), for later plotting.
 *
 * Usage:
 *   voxel_dump.exe [EL] [SEP] [TOTAL_FRAMES] [DUMP_EVERY] [budget_seconds]
 *
 * Two source centres (layers 0 and 1) are placed a distance SEP apart along
 * the x axis with opposite momenta.  Every DUMP_EVERY frames the runner
 * scans the whole lattice and prints, for each cell that is part of the
 * wave field (active or u != 0), one CSV line:
 *
 *   frame,x,y,z,w,active,r,u,v,ch
 *
 * Coordinates are absolute lattice coordinates (0..EL-1), w is the layer.
 * ch is the charge word in hex.  Rows with active=1 are the front shell;
 * rows with active=0 and u!=0 are the interior of the wave.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cmath>
#include <vector>
#include <array>
#include <io.h>
#include <fcntl.h>

#include "model/simulation.h"
#include "config.h"                        // Config / gConfig stub below

// ---------------------------------------------------------------------------
// Suppress chatter written by the init routines directly to stdout (the
// lattice setup prints banners/parameters).  The CSV goes to stdout, so we
// briefly redirect descriptor 1 to NUL, run the init, then restore.
// ---------------------------------------------------------------------------
static int suppressStdout()
{
  fflush(stdout);
  const int saved = _dup(_fileno(stdout));
  const int nul = _open("NUL", _O_WRONLY);
  if (saved >= 0 && nul >= 0)
    _dup2(nul, _fileno(stdout));
  else
  {
    if (saved >= 0) _close(saved);
    if (nul >= 0)   _close(nul);
    return -1;
  }
  return saved;
}

static void restoreStdout(int saved)
{
  fflush(stdout);
  _dup2(saved, _fileno(stdout));
  _close(saved);
}

static void quietInit()
{
  const int saved = suppressStdout();
  for (int step = 0; step <= 7; ++step)
    automaton::initSimulation(step);
  if (saved >= 0)
    restoreStdout(saved);
}

// ---------------------------------------------------------------------------
// CSV stream: a duplicate of the original stdout descriptor.  The simulation
// keeps printing periodic [charges] census lines to stdout, so we redirect
// descriptor 1 to NUL for the whole run and emit the CSV through g_csv,
// which still points at the caller's stdout (file or pipe).
// ---------------------------------------------------------------------------
static FILE* g_csv = nullptr;

static void openCsvStream()
{
  fflush(stdout);
  const int dup = _dup(_fileno(stdout));
  g_csv = (dup >= 0) ? _fdopen(dup, "w") : nullptr;
  const int nul = _open("NUL", _O_WRONLY);
  if (nul >= 0)
  {
    _dup2(nul, _fileno(stdout));   // all stray printf() now goes to NUL
    _close(nul);
  }
}

static void closeCsvStream()
{
  if (g_csv)
  {
    fflush(g_csv);
    fclose(g_csv);
    g_csv = nullptr;
  }
}

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

// ---------------------------------------------------------------------------
// Dump every voxel that belongs to the wave field of layer w.
// ---------------------------------------------------------------------------
static void dumpLayerVoxels(unsigned frame, unsigned w)
{
  const int ELi = (int)automaton::EL;
  for (int x = 0; x < ELi; ++x)
    for (int y = 0; y < ELi; ++y)
      for (int z = 0; z < ELi; ++z)
      {
        const automaton::Cell& c = automaton::getCell(
            automaton::lattice_curr, x, y, z, (int)w);
        if (!c.active && c.u == 0)
          continue;                        // not part of the wave field
        // CSV: frame,x,y,z,w,active,r,u,v,ch
        std::fprintf(g_csv, "%u,%d,%d,%d,%u,%u,%d,%d,%d,0x%02X\n",
                     frame, x, y, z, w, c.active, c.r, c.u, c.v,
                     (unsigned)c.ch);
      }
}

// ---------------------------------------------------------------------------
// Dump all wave-field voxels of both layers.
// ---------------------------------------------------------------------------
static void dumpVoxels(unsigned frame)
{
  dumpLayerVoxels(frame, 0);
  dumpLayerVoxels(frame, 1);
}

int main(int argc, char** argv)
{
  unsigned EL_in  = (argc > 1) ? static_cast<unsigned>(atoi(argv[1])) : 7u;
  unsigned SEP    = (argc > 2) ? static_cast<unsigned>(atoi(argv[2])) : 4u;
  unsigned FRAMES = (argc > 3) ? static_cast<unsigned>(atoi(argv[3])) : 24u;
  unsigned EVERY  = (argc > 4) ? static_cast<unsigned>(atoi(argv[4])) : 1u;
  double   budget = (argc > 5) ? atof(argv[5]) : 15.0;

  if (EL_in < 3 || EL_in > 31 || (EL_in % 2) == 0)
  {
    fprintf(stderr, "EL must be an odd value in [3,31]\n");
    return 1;
  }
  if (SEP < 2) SEP = 2;
  if (EVERY == 0) EVERY = 1;

  const unsigned W_in = 2u;   // exactly two bubbles

  fprintf(stderr, "=== voxel dump: two-bubble experiment ===\n");
  fprintf(stderr, "EL=%u W_USED=2 SEP=%u RMAX=%u frames=%u dump_every=%u budget=%.1fs\n",
          EL_in, SEP, EL_in / 2u, FRAMES, EVERY, budget);

  // Setup phase: everything below prints banners to stdout, which would
  // pollute the CSV stream.  Redirect descriptor 1 to NUL for the whole
  // initialization, then restore before the first data line.
  const int saved = suppressStdout();
  automaton::calculateParameters(EL_in, W_in);
  const bool okAlloc = automaton::tryAllocate(
      static_cast<int>(EL_in), static_cast<int>(W_in));
  if (saved >= 0)
    restoreStdout(saved);
  if (!okAlloc)
  {
    fprintf(stderr, "allocation failed: %s\n",
            automaton::lastAllocationError.c_str());
    return 2;
  }

  // Canonical startup (seed, params, replicate, sanity), then overwrite the
  // two layers with the controlled two-bubble configuration.
  quietInit();

  const unsigned C = automaton::CENTER;
  const unsigned off = SEP / 2u;

  placeSource(0, C - off, C, C,  +1, 0, 0);   // left bubble, moving right
  placeSource(1, C + off, C, C,  -1, 0, 0);   // right bubble, moving left

  // CSV header on stdout (through the dedicated stream).
  openCsvStream();
  std::fprintf(g_csv, "frame,x,y,z,w,active,r,u,v,ch\n");

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
      if ((frame % EVERY) == 0)
        dumpVoxels(frame);

      const double secs = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - t0).count();
      if (secs >= budget)
      {
        fprintf(stderr, "[voxel] budget hit at frame %u/%u\n", frame, FRAMES);
        break;
      }
    }
  }

  // Final source-centre positions (stderr so it does not pollute the CSV).
  const auto& p0 = automaton::lcenters[0];
  const auto& p1 = automaton::lcenters[1];
  fprintf(stderr, "final centres: bubble0=(%u,%u,%u) bubble1=(%u,%u,%u)\n",
          p0[0], p0[1], p0[2], p1[0], p1[1], p1[2]);

  closeCsvStream();
  return 0;
}

