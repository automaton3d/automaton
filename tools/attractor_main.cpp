/*
 * attractor_main.cpp — headless console runner for the island-population
 * attractor experiment ("Dynamic charge quantization", manuscript).
 *
 * Usage:
 *   attractor.exe [EL] [W_USED] [TOTAL_FRAMES] [csv] [ckpt] [budget_seconds]
 *
 * Chunked execution: each invocation advances at most `budget_seconds` of
 * wall time (checked at frame boundaries), writes a checkpoint and exits;
 * re-running resumes until TOTAL_FRAMES, when report/CSV are emitted.
 *
 * Checkpoint (binary): frame(u32), pulse_tick(u32), lcenters,
 * lattice_curr[BLOCK*sizeof(Cell)]. draft/mirror are rebuilt on resume
 * (boundary mirror = curr with f := t; draft is regenerated every tick).
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <string>
#include <fstream>
#include <algorithm>

#include "model/simulation.h"
#include "model/attractor.h"
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
}

static bool saveCheckpoint(const std::string& path, unsigned frame)
{
  std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
  if (!out) return false;

  const uint32_t hdr[2] = { frame, automaton::pulse_tick };
  out.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));

  for (unsigned w = 0; w < automaton::W_USED; ++w)
    out.write(reinterpret_cast<const char*>(automaton::lcenters[w].data()),
              static_cast<std::streamsize>(sizeof(unsigned) * 3));

  out.write(reinterpret_cast<const char*>(automaton::lattice_curr.data()),
            static_cast<std::streamsize>(automaton::BLOCK * sizeof(automaton::Cell)));
  return static_cast<bool>(out);
}

static bool loadCheckpoint(const std::string& path, unsigned& frame)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) return false;

  uint32_t hdr[2] = { 0, 0 };
  in.read(reinterpret_cast<char*>(hdr), sizeof(hdr));
  frame                 = hdr[0];
  automaton::pulse_tick = hdr[1];

  for (unsigned w = 0; w < automaton::W_USED; ++w)
    in.read(reinterpret_cast<char*>(automaton::lcenters[w].data()),
            static_cast<std::streamsize>(sizeof(unsigned) * 3));

  in.read(reinterpret_cast<char*>(automaton::lattice_curr.data()),
          static_cast<std::streamsize>(automaton::BLOCK * sizeof(automaton::Cell)));
  if (!in) return false;

  // Boundary mirror exactly as swap_lattices_cpu() builds it when k==0:
  for (unsigned w = 0; w < automaton::W_USED; ++w)
    for (unsigned x = 0; x < automaton::EL; ++x)
      for (unsigned y = 0; y < automaton::EL; ++y)
        for (unsigned z = 0; z < automaton::EL; ++z)
        {
          automaton::Cell& c = automaton::getCell(
              automaton::lattice_curr, static_cast<int>(x),
              static_cast<int>(y), static_cast<int>(z), static_cast<int>(w));
          automaton::Cell& m = automaton::getCell(
              automaton::lattice_mirror, static_cast<int>(x),
              static_cast<int>(y), static_cast<int>(z), static_cast<int>(w));
          m = c;
          m.f = c.t;
        }
  // draft must hold valid k values before the next tick's FSM dispatch.
  std::copy(automaton::lattice_curr.begin(),
            automaton::lattice_curr.begin() + automaton::BLOCK,
            automaton::lattice_draft.begin());
  return true;
}

int main(int argc, char** argv)
{
  unsigned    EL_in  = (argc > 1) ? static_cast<unsigned>(atoi(argv[1])) : 7u;
  unsigned    W_in   = (argc > 2) ? static_cast<unsigned>(atoi(argv[2])) : 0u;
  unsigned    FRAMES = (argc > 3) ? static_cast<unsigned>(atoi(argv[3])) : 48u;
  std::string csv    = (argc > 4) ? argv[4] : "build/attractor_ts.csv";
  std::string ckpt   = (argc > 5) ? argv[5] : "build/attractor.ckpt";
  double      budget = (argc > 6) ? atof(argv[6]) : 22.0;

  // Fatia 2 knobs — M/Mbar turnaround hook: eps / pbase / xorshift seed.
  gConfig.simulation.mm_eps   = (argc > 7) ? atof(argv[7]) : 0.0;
  gConfig.simulation.mm_pbase = (argc > 8) ? atof(argv[8]) : 1.0;
  gConfig.simulation.mm_seed  = (argc > 9) ? (unsigned)atoi(argv[9]) : 1u;

  if (EL_in < 3 || EL_in > 31 || (EL_in % 2) == 0)
  {
    fprintf(stderr, "EL must be an odd value in [3,31]\n");
    return 1;
  }
  if (W_in == 0)
    W_in = 3u * EL_in * EL_in;

  printf("=== island-population attractor experiment ===\n");
  printf("EL=%u W_USED=%u target=%u frames csv=%s ckpt=%s budget=%.1fs\n",
         EL_in, W_in, FRAMES, csv.c_str(), ckpt.c_str(), budget);

  automaton::calculateParameters(EL_in, W_in);
  if (!automaton::tryAllocate(static_cast<int>(EL_in), static_cast<int>(W_in)))
  {
    fprintf(stderr, "allocation failed: %s\n",
            automaton::lastAllocationError.c_str());
    return 2;
  }

  unsigned frame = 0;
  const bool resumed = loadCheckpoint(ckpt, frame);   // sets pulse_tick too

  if (resumed)
  {
    automaton::attractor::begin();
    if (!automaton::attractor::loadSeries(ckpt + ".series"))
    {
      fprintf(stderr, "checkpoint series missing/corrupt\n");
      return 3;
    }
    automaton::attractor::resyncPrev();
    printf("resumed at frame %u\n", frame);
    if (frame >= FRAMES)
      frame = FRAMES;                     // report-only pass-through below
  }
  else
  {
    // Canonical startup sequence (steps 0..7): seed, params, replicate, sanity.
    for (int step = 0; step <= 7; ++step)
      automaton::initSimulation(step);
    automaton::attractor::begin();
  }

  const auto t0 = std::chrono::steady_clock::now();
  long long ticks = 0;
  bool checkpointed = false;

  while (frame < FRAMES)
  {
    // simulation() = update_lattice() + swap_lattices(); true once per frame.
    const bool newFrame = automaton::simulation();
    ++ticks;

    if (newFrame)
    {
      ++frame;
      automaton::attractor::sampleFrame(frame);

      const double secs = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - t0).count();

      if ((frame % 5) == 0 || secs >= budget || frame == FRAMES)
        fprintf(stderr, "[chunk] frame %u/%u (%.1fs)\n", frame, FRAMES, secs);

      if (secs >= budget && frame < FRAMES)
      {
        if (saveCheckpoint(ckpt, frame) &&
            automaton::attractor::saveSeries(ckpt + ".series"))
          printf("CHECKPOINT frame=%u ticks_this_chunk=%lld\n", frame, ticks);
        else
        {
          fprintf(stderr, "checkpoint write failed\n");
          return 4;
        }
        checkpointed = true;
        break;
      }
    }
  }

  if (!checkpointed)
  {
    // Target reached: emit final results and clean the checkpoint artifacts.
    const automaton::attractor::Report rep =
        automaton::attractor::summarize();
    automaton::attractor::printReport(rep);

    if (automaton::attractor::writeCSV(csv, rep))
      printf("time series written to %s\n", csv.c_str());
    else
      fprintf(stderr, "warning: could not write CSV to %s\n", csv.c_str());

    remove(ckpt.c_str());
    remove((ckpt + ".series").c_str());
    printf("DONE frames=%u\n", frame);
  }

  return 0;
}
