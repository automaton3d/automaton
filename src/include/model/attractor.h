#ifndef ATTRACTOR_H_
#define ATTRACTOR_H_

/*
 * attractor.h — Population instrumentation for W-islands.
 *
 * Measures the per-island bubble population N(t) plus gross capture/escape
 * event counts between consecutive light frames, in order to test the
 * dynamic charge quantization attractor N* described in the manuscript
 * section "Dynamic charge quantization":
 *
 *     Gamma_cap(N) > Gamma_esc(N) for small N,
 *     Gamma_cap(N) < Gamma_esc(N) for large N,
 *     stable attractor near Gamma_cap(N*) ~= Gamma_esc(N*).
 *
 * Operational definitions (affinity level):
 *   - A cell belongs to island g when its affinity a != W_USED and
 *     g = islandOf(a) (= a / ISLAND_SIZE).
 *   - CAPTURE : orphan -> member        (a: W_USED -> chiefW)
 *   - ESCAPE  : member -> orphan        (a: chiefW -> W_USED)
 *   - SWITCH  : member g1 -> member g2  (counted as escape(g1)+capture(g2))
 *
 * Sampling happens once per completed light frame (k wrapped to zero),
 * reading lattice_curr only.
 */

#include <string>
#include <vector>
#include <cstdint>

namespace automaton
{
  namespace attractor
  {
    struct IslandStats
    {
      unsigned island  = 0;
      double   meanN   = 0.0;   // mean population
      double   stdN    = 0.0;   // population std deviation
      double   minN    = 0.0;
      double   maxN    = 0.0;
      long     samples = 0;     // number of frames sampled for this island

      // OLS fit of dN(t) = N(t+1) - N(t) against N(t):
      double   slope     = 0.0;  // restoring coefficient (< 0 => attractor)
      double   slopeSE   = 0.0;  // standard error of slope
      double   intercept = 0.0;
      double   r2        = 0.0;
      bool     hasFit    = false;
      double   nStar     = 0.0;  // -intercept/slope when slope < 0

      double   capRate   = 0.0;  // gross captures per frame
      double   escRate   = 0.0;  // gross escapes per frame
    };

    struct Report
    {
      unsigned frames    = 0;
      unsigned nIslands  = 0;
      double   pooledSlope = 0.0, pooledSlopeSE = 0.0;
      double   pooledIntercept = 0.0, pooledR2 = 0.0;
      bool     pooledHasFit = false;
      double   pooledNStar  = 0.0;
      long     pooledPoints = 0;
      double   meanTotalPop = 0.0;   // mean sum of all island populations
      double   meanCaptures = 0.0;   // gross captures per frame (all islands)
      double   meanEscapes  = 0.0;   // gross escapes  per frame (all islands)
      std::vector<IslandStats> islands;
    };

    /// Allocate the previous-state snapshot. Call once after tryAllocate().
    void begin();

    /// Recompute the previous-state snapshot from lattice_curr WITHOUT
    /// counting events (used when resuming from a checkpoint).
    void resyncPrev();

    /// Persist the sampled time series (frames_, nBuckets_, pop_, cap_, esc_).
    bool saveSeries(const std::string& path);

    /// Restore the time series previously written by saveSeries().
    /// Returns false on I/O error or topology mismatch.
    bool loadSeries(const std::string& path);


    /// Scan lattice_curr after a completed light frame (frame = 1-based id).
    void sampleFrame(unsigned frame);

    /// Number of completed frames sampled so far.
    unsigned framesSampled();

    /// Per-island statistics + pooled regression across all islands.
    Report summarize();

    /// Console report (stdout).
    void printReport(const Report& rep);

    /// Time-series CSV: frame,island,population,captures,escapes
    bool writeCSV(const std::string& path, const Report& rep);

    /// Per-frame sector-flux CSV (8 columns: cap/esc split by Orbis/Umbra
    /// and matter/anti).  Parallel to writeCSV.
    bool writeSectorCSV(const std::string& path);
  }
}

#endif /* ATTRACTOR_H_ */
