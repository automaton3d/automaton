// charges.cpp
//
// Fatia 1 — charge census & virgin-wrap ledger (instrumentation only).
//
// Measures the two quantities the bariogenesis study (virada.c) needs
// before any dynamics change:
//   (1) the emergent matter/antimatter composition of the live lattice;
//   (2) the fraction of source turnarounds (t == RMAX crossings) that
//       happen WITHOUT any interaction in between ("virgin wraps") —
//       the pbase(lap) the toy model had to prescribe by hand.
//
// READ-ONLY over the lattice: nothing here writes to cells, so the
// automaton dynamics are bit-identical with or without this module.
// CPU path only (update_lattice_cpu); the CUDA bridge is untouched.
//
// Charge encoding (initSim.cpp:76):  ch = color | q<<3 | w0<<4 | w1<<5
//   color = ch & 0x07 = (c2 c1 c0)
//   Matter colors  (SIG < 2 set bits): N(000), R(001), G(010), B(100)
//   Antimatter     (SIG >= 2):         B~(011), G~(101), R~(110), N~(111)
// — the same classification used by the virada.c / combine.c toy studies.

#include "model/simulation.h"

#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>

namespace automaton
{
  // Globals owned by the lattice/simulation translation units.
  extern unsigned EL;
  extern unsigned W_USED;
  extern std::vector<std::array<unsigned, 3>> lcenters;

  namespace
  {
    constexpr unsigned REPORT_EVERY = 256;   // census cadence (ticks)

    std::vector<uint64_t> matCount;          // per layer, last census
    std::vector<uint64_t> antiCount;
    std::vector<uint64_t> orphanMat;         // cells outside any island (a == W_USED)
    std::vector<uint64_t> orphanAnti;
    std::vector<unsigned> prevT;             // last seen source-center clock
    std::vector<uint8_t>  dirty;             // island interacted since last turnaround
    uint64_t turnVirgin = 0;
    uint64_t turnDirty  = 0;

    inline unsigned popcount3(unsigned color)
    {
      return (color & 1u) + ((color >> 1) & 1u) + ((color >> 2) & 1u);
    }

    inline bool ledgerReady()
    {
      return (!dirty.empty() && dirty.size() == W_USED &&
              lcenters.size() >= W_USED);
    }
  }

  void chargesReset()
  {
    matCount.assign(W_USED, 0);
    antiCount.assign(W_USED, 0);
    orphanMat.assign(W_USED, 0);
    orphanAnti.assign(W_USED, 0);
    prevT.assign(W_USED, 0);
    dirty.assign(W_USED, 0);
    turnVirgin = 0;
    turnDirty  = 0;
  }

  void chargesMarkInteraction(unsigned w)
  {
    if (w < dirty.size())
      dirty[w] = 1;
  }

  void chargesSampleTurnarounds()
  {
    if (!ledgerReady() || lattice_curr.empty())
      return;

    for (unsigned w = 0; w < W_USED; ++w)
    {
      const std::array<unsigned, 3>& c3 = lcenters[w];
      const Cell& c = getCell(lattice_curr, (int)c3[0], (int)c3[1], (int)c3[2], (int)w);
      const unsigned t    = c.t;
      const unsigned prev = prevT[w];
      prevT[w] = t;

      // The breathing clock crossed into the expansion limit this window:
      // a turnaround of island w.  Virgin iff no reemission happened since
      // the previous crossing (reemitSourceAt resets t to 0, which also
      // cannot produce a false crossing: prev >= t then).
      if (prev < RMAX && t >= RMAX)
      {
        if (dirty[w] != 0)
        {
          ++turnDirty;
          dirty[w] = 0;
        }
        else
        {
          ++turnVirgin;
          printf("[charges] VIRGIN turnaround w=%u tick=%u t=%u\n",
                 w, pulse_tick, t);
          fflush(stdout);
        }
      }
    }
  }

  void chargesReport(unsigned tick)
  {
    if (tick % REPORT_EVERY != 1)   // baseline at tick 1, then every 256
      return;
    if (!ledgerReady() || lattice_curr.empty())
      return;

    for (unsigned w = 0; w < W_USED; ++w)
    {
      matCount[w]  = 0;  antiCount[w]  = 0;
      orphanMat[w] = 0;  orphanAnti[w] = 0;
    }

    uint64_t totMat = 0, totAnti = 0, totOrphanMat = 0, totOrphanAnti = 0;

    for (unsigned w = 0; w < W_USED; ++w)
    {
      for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
      for (unsigned z = 0; z < EL; ++z)
      {
        const Cell& c = getCell(lattice_curr, (int)x, (int)y, (int)z, (int)w);
        const bool isMatter = popcount3(c.ch & 0x07u) < 2u;
        const bool orphan   = (c.a == W_USED);

        if (isMatter) { ++matCount[w];  ++totMat;  if (orphan) { ++orphanMat[w];  ++totOrphanMat; } }
        else          { ++antiCount[w]; ++totAnti; if (orphan) { ++orphanAnti[w]; ++totOrphanAnti; } }
      }
    }

    const double total = (double)totMat + (double)totAnti;
    const double ratio = (total > 0.0) ? (double)totMat / total : 0.0;

    printf("[charges] tick=%u MAT=%llu ANTI=%llu mat/total=%.6f | orphanM=%llu orphanA=%llu | turnarounds V=%llu D=%llu\n",
           tick,
           (unsigned long long)totMat, (unsigned long long)totAnti,
           ratio,
           (unsigned long long)totOrphanMat, (unsigned long long)totOrphanAnti,
           (unsigned long long)turnVirgin, (unsigned long long)turnDirty);
    for (unsigned w = 0; w < W_USED; ++w)
      printf("[charges]   w=%u mat=%llu anti=%llu\n",
             w, (unsigned long long)matCount[w], (unsigned long long)antiCount[w]);
    fflush(stdout);
  }
}
