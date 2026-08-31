/*
 * attractor.cpp — W-island population instrumentation.
 * See attractor.h for the operational definitions.
 */

#include "model/attractor.h"
#include "model/simulation.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace automaton
{
  namespace attractor
  {
    namespace
    {
      constexpr uint16_t ORPHAN = 0xFFFF;

      std::vector<uint16_t> prev_;   // per-cell island bucket snapshot
      std::vector<uint64_t> pop_;    // [frame * nBuckets + g]
      std::vector<uint64_t> cap_;
      std::vector<uint64_t> esc_;
      std::vector<uint64_t> flux_;   // per-frame sector flux, 8 slots/frame:
                                     //   idx = (esc?4:0) + sec*2 + (mat?1:0)
                                     //   capM_Orb capA_Orb capM_Umb capA_Umb
                                     //   escM_Orb escA_Orb escM_Umb escA_Umb
      unsigned nBuckets_ = 0;
      unsigned frames_   = 0;

      // Sector of a cell from its charge word (bit5 = w1).  Invariant under
      // the M/Mbar hook (ch ^= 0x1F preserves w1), so a bubble's sector is
      // a stable label throughout the run.
      inline unsigned secOf(const Cell& c)
      {
        return (c.ch >> 5) & 1u;
      }

      // Matter/anti from the color weight (SIG < 2 = matter), same rule as
      // the charge census and the virada/combine studies.
      inline bool matOf(const Cell& c)
      {
        return ((c.ch & 1u) + ((c.ch >> 1) & 1u) + ((c.ch >> 2) & 1u)) < 2u;
      }

      // Series-start island charge balance per sector (series v3): the
      // baseline the sector-flux telescoping integrates from.  While
      // (sector, sign) per cell is invariant (eps=0 hook), the island
      // balance obeys
      //   D_isl(sec, t) = D0(sec) + sum_frames (capM-capA-escM+escA)(sec)
      // so the SECTOR FLUX report can reconcile directly against the
      // [charges] closure census (DslOrb/DslUmb).
      long long dIslOrb0_ = 0;
      long long dIslUmb0_ = 0;

      void computeIslandD(long long& orb, long long& umb)
      {
        long long o = 0, u = 0;
        for (size_t i = 0; i < static_cast<size_t>(BLOCK); ++i)
        {
          const Cell& c = lattice_curr[i];
          if (c.a == W_USED || ISLAND_SIZE == 0) continue;
          const long long d = matOf(c) ? 1 : -1;
          if (secOf(c) == 0) o += d; else u += d;
        }
        orb = o;
        umb = u;
      }

      // Ordinary least squares of y ~ x over x[begin..end).
      inline void ols(const std::vector<double>& x,
                      const std::vector<double>& y,
                      size_t begin, size_t end,
                      double& slope, double& intercept,
                      double& r2, double& slopeSE)
      {
        slope = intercept = r2 = slopeSE = 0.0;
        const size_t n = end - begin;
        if (n < 8) return;

        double sx = 0, sy = 0;
        for (size_t i = begin; i < end; ++i) { sx += x[i]; sy += y[i]; }
        const double mx = sx / n, my = sy / n;

        double sxx = 0, sxy = 0, syy = 0;
        for (size_t i = begin; i < end; ++i)
        {
          const double dx = x[i] - mx, dy = y[i] - my;
          sxx += dx * dx; sxy += dx * dy; syy += dy * dy;
        }
        if (sxx <= 0.0) return;

        slope     = sxy / sxx;
        intercept = my - slope * mx;

        // Residual sum of squares around the fitted line.
        const double sse = syy - slope * sxy;
        const double sst = syy;
        r2 = (sst > 0.0) ? std::max(0.0, 1.0 - sse / sst) : 0.0;

        const double dof = static_cast<double>(n) - 2.0;
        if (dof > 0.0 && sse > 0.0)
          slopeSE = std::sqrt((sse / dof) / sxx);
      }
    }

    void begin()
    {
      nBuckets_ = ISLAND_SIZE ? ((W_USED + ISLAND_SIZE - 1) / ISLAND_SIZE) : W_USED;
      if (nBuckets_ == 0) nBuckets_ = 1;

      prev_.assign(static_cast<size_t>(BLOCK), ORPHAN);
      pop_.clear(); cap_.clear(); esc_.clear(); flux_.clear();
      frames_ = 0;
      // Fresh start: D0 = island balance of the seed.  On resume this gets
      // overwritten by loadSeries (v3) with the run's original baseline.
      computeIslandD(dIslOrb0_, dIslUmb0_);

      printf("attractor: %u island buckets (ISLAND_SIZE=%u, W_USED=%u)\n",
             nBuckets_, ISLAND_SIZE, W_USED);
    }

    void sampleFrame(unsigned frame)
    {
      if (frame == 0) frame = frames_ + 1;   // defensive
      const size_t off = static_cast<size_t>(frame - 1) * nBuckets_;

      pop_.resize(off + nBuckets_, 0);
      cap_.resize(off + nBuckets_, 0);
      esc_.resize(off + nBuckets_, 0);

      const size_t foff = static_cast<size_t>(frame - 1) * 8;
      flux_.resize(foff + 8, 0);

      std::fill(pop_.begin() + off, pop_.end(), 0ull);
      std::fill(cap_.begin() + off, cap_.end(), 0ull);
      std::fill(esc_.begin() + off, esc_.end(), 0ull);
      std::fill(flux_.begin() + foff, flux_.end(), 0ull);

      for (size_t i = 0; i < static_cast<size_t>(BLOCK); ++i)
      {
        const unsigned a = lattice_curr[i].a;

        uint16_t now = ORPHAN;
        if (a != W_USED && ISLAND_SIZE != 0)
        {
          size_t b = static_cast<size_t>(a) / ISLAND_SIZE;
          if (b >= nBuckets_) b = nBuckets_ - 1;
          now = static_cast<uint16_t>(b);
        }

        if (now != ORPHAN)
          ++pop_[off + now];

        const uint16_t pv = prev_[i];
        if (lattice_curr[i].t >= RMAX && pv != now)
        {
          // Turnaround flicker: at the expansion limit (t == RMAX) the active
          // shell reads a := W_USED for exactly one tick and is re-affiliated
          // on the next tick with an unchanged charge word.  Hold the
          // pre-turnaround label so the frame series measures settled
          // membership instead of this D-neutral 1-tick flicker.
          continue;
        }

        if (pv == ORPHAN && now != ORPHAN)
        {
          ++cap_[off + now];
          ++flux_[foff + 0u + secOf(lattice_curr[i]) * 2u + (matOf(lattice_curr[i]) ? 1u : 0u)];
        }
        else if (pv != ORPHAN && now == ORPHAN)
        {
          ++esc_[off + pv];
          ++flux_[foff + 4u + secOf(lattice_curr[i]) * 2u + (matOf(lattice_curr[i]) ? 1u : 0u)];
        }
        else if (pv != ORPHAN && now != ORPHAN && pv != now)
        {
          ++esc_[off + pv];   // island switch = escape + capture
          ++cap_[off + now];
          ++flux_[foff + 0u + secOf(lattice_curr[i]) * 2u + (matOf(lattice_curr[i]) ? 1u : 0u)];
          ++flux_[foff + 4u + secOf(lattice_curr[i]) * 2u + (matOf(lattice_curr[i]) ? 1u : 0u)];
        }

        prev_[i] = now;
      }

      frames_ = frame;
    }

    unsigned framesSampled() { return frames_; }

    void resyncPrev()
    {
      if (prev_.size() != static_cast<size_t>(BLOCK))
        prev_.assign(static_cast<size_t>(BLOCK), ORPHAN);

      for (size_t i = 0; i < static_cast<size_t>(BLOCK); ++i)
      {
        const unsigned a = lattice_curr[i].a;
        uint16_t now = ORPHAN;
        if (a != W_USED && ISLAND_SIZE != 0)
        {
          size_t b = static_cast<size_t>(a) / ISLAND_SIZE;
          if (b >= nBuckets_) b = nBuckets_ - 1;
          now = static_cast<uint16_t>(b);
        }
        prev_[i] = now;
      }
    }

    bool saveSeries(const std::string& path)
    {
      FILE* f = fopen(path.c_str(), "wb");
      if (!f) return false;
      uint32_t hdr[5] = { frames_, nBuckets_, 3,
                          (uint32_t)(int32_t)dIslOrb0_,
                          (uint32_t)(int32_t)dIslUmb0_ };  // v3 adds D0 per sector
      fwrite(hdr, sizeof(hdr), 1, f);
      if (frames_)
      {
        fwrite(pop_.data(), sizeof(uint64_t), pop_.size(), f);
        fwrite(cap_.data(), sizeof(uint64_t), cap_.size(), f);
        fwrite(esc_.data(), sizeof(uint64_t), esc_.size(), f);
        fwrite(flux_.data(), sizeof(uint64_t), flux_.size(), f);
      }
      fclose(f);
      return true;
    }

    bool loadSeries(const std::string& path)
    {
      FILE* f = fopen(path.c_str(), "rb");
      if (!f) return false;
      uint32_t hdr[3] = { 0, 0, 0 };
      size_t got = fread(hdr, sizeof(uint32_t), 3, f);
      if (got != 3)
      {
        // Fallback: pre-v2 header (frames_, nBuckets_ only).
        uint32_t hdr2[2] = { hdr[0], hdr[1] };
        rewind(f);
        if (fread(hdr2, sizeof(uint32_t), 2, f) != 2 ||
            hdr2[1] != nBuckets_)
        {
          fclose(f);
          return false;
        }
        hdr[0] = hdr2[0]; hdr[1] = hdr2[1]; hdr[2] = 0;
      }
      if (hdr[1] != nBuckets_)
      {
        fclose(f);
        return false;
      }
      if (hdr[2] >= 3)
      {
        // v3: series-start island D per sector (telescoping baseline).
        uint32_t d0[2] = { 0, 0 };
        if (fread(d0, sizeof(uint32_t), 2, f) != 2)
        {
          fclose(f);
          return false;
        }
        dIslOrb0_ = (long long)(int32_t)d0[0];
        dIslUmb0_ = (long long)(int32_t)d0[1];
      }
      else
      {
        dIslOrb0_ = 0;
        dIslUmb0_ = 0;
      }
      frames_ = hdr[0];
      const size_t n = static_cast<size_t>(frames_) * nBuckets_;
      pop_.resize(n); cap_.resize(n); esc_.resize(n);
      flux_.assign(static_cast<size_t>(frames_) * 8, 0);
      bool ok = true;
      if (n)
        ok = fread(pop_.data(), sizeof(uint64_t), n, f) == n &&
             fread(cap_.data(), sizeof(uint64_t), n, f) == n &&
             fread(esc_.data(), sizeof(uint64_t), n, f) == n;
      if (ok && hdr[2] >= 2 && frames_)
        ok = fread(flux_.data(), sizeof(uint64_t), frames_ * 8, f) == frames_ * 8;
      fclose(f);
      return ok;
    }


    // ==== summary / report / csv ====

    Report summarize()
    {
      Report rep;
      rep.frames   = frames_;
      rep.nIslands = nBuckets_;
      rep.islands.resize(nBuckets_);

      std::vector<double> xs, ys;   // pooled (N_t, dN_t) points

      double totPop = 0.0;
      unsigned totFrames = 0;

      for (unsigned g = 0; g < nBuckets_; ++g)
      {
        IslandStats& st = rep.islands[g];
        st.island = g;

        const size_t F = frames_;
        if (F < 8) continue;

        std::vector<double> N(F), dN(F - 1);
        double s = 0, s2 = 0;
        double mn = 1e300, mx = -1e300;
        double caps = 0, escs = 0;

        for (size_t t = 0; t < F; ++t)
        {
          const double v = static_cast<double>(pop_[t * nBuckets_ + g]);
          N[t] = v;
          s += v; s2 += v * v;
          mn = std::min(mn, v); mx = std::max(mx, v);
          caps += static_cast<double>(cap_[t * nBuckets_ + g]);
          escs += static_cast<double>(esc_[t * nBuckets_ + g]);
        }

        st.meanN   = s / F;
        st.stdN    = std::sqrt(std::max(0.0, s2 / F - st.meanN * st.meanN));
        st.minN    = mn;
        st.maxN    = mx;
        st.capRate = caps / F;
        st.escRate = escs / F;
        st.samples = static_cast<long>(F);

        for (size_t t = 0; t + 1 < F; ++t)
        {
          dN[t] = N[t + 1] - N[t];
          xs.push_back(N[t]);
          ys.push_back(dN[t]);
        }

        ols(N, dN, 0, dN.size(), st.slope, st.intercept, st.r2, st.slopeSE);
        st.hasFit = (st.slopeSE > 0.0) || (st.r2 > 0.0);
        if (st.hasFit && st.slope < 0.0)
          st.nStar = -st.intercept / st.slope;

        totPop += s;
        totFrames = static_cast<unsigned>(F);
      }

      rep.meanTotalPop = (totFrames > 0) ? totPop / totFrames : 0.0;

      if (frames_ > 0)
      {
        double c = 0, e = 0;
        for (size_t t = 0; t < frames_; ++t)
          for (unsigned g = 0; g < nBuckets_; ++g)
          {
            c += static_cast<double>(cap_[t * nBuckets_ + g]);
            e += static_cast<double>(esc_[t * nBuckets_ + g]);
          }
        rep.meanCaptures = c / frames_;
        rep.meanEscapes  = e / frames_;
      }

      // Pooled regression across all islands: strongest attractor test.
      rep.pooledPoints = static_cast<long>(xs.size());
      ols(xs, ys, 0, xs.size(),
          rep.pooledSlope, rep.pooledIntercept, rep.pooledR2, rep.pooledSlopeSE);
      rep.pooledHasFit = rep.pooledPoints >= 8;
      if (rep.pooledHasFit && rep.pooledSlope < 0.0)
        rep.pooledNStar = -rep.pooledIntercept / rep.pooledSlope;

      return rep;
    }

    // Accumulated sector flux over all sampled frames.
    struct SectorTotals
    {
      uint64_t capM_Orb = 0, capA_Orb = 0, capM_Umb = 0, capA_Umb = 0;
      uint64_t escM_Orb = 0, escA_Orb = 0, escM_Umb = 0, escA_Umb = 0;
    };

    SectorTotals fluxTotals()
    {
      SectorTotals t;
      for (unsigned fr = 0; fr < frames_; ++fr)
      {
        const size_t foff = static_cast<size_t>(fr) * 8;
        t.capM_Orb += flux_[foff + 0];
        t.capA_Orb += flux_[foff + 1];
        t.capM_Umb += flux_[foff + 2];
        t.capA_Umb += flux_[foff + 3];
        t.escM_Orb += flux_[foff + 4];
        t.escA_Orb += flux_[foff + 5];
        t.escM_Umb += flux_[foff + 6];
        t.escA_Umb += flux_[foff + 7];
      }
      return t;
    }

    void printSectorFlux(const SectorTotals& t, unsigned frames)
    {
      const double f = (frames > 0) ? (double)frames : 1.0;
      // Net balance of D = mat - anti flowing INTO islands per frame, by sector.
      //   netD_orb = (capM-capA)_orb - (escM-esA)_orb
      //   netD_umb = (capM-capA)_umb - (escM-esA)_umb
      const double netD_orb =
          ((double)t.capM_Orb - (double)t.capA_Orb) - ((double)t.escM_Orb - (double)t.escA_Orb);
      const double netD_umb =
          ((double)t.capM_Umb - (double)t.capA_Umb) - ((double)t.escM_Umb - (double)t.escA_Umb);

      printf("----------------------------------------------------------------\n");
      printf("SECTOR FLUX  (aggregate %u frames, per-frame rates)\n", frames);
      // Telescoping prediction: with (sector, sign) per cell invariant
      // (eps=0), D_isl(sec, now) = D0(sec) + cumulative net flux.  Compare
      // against the [charges] closure DslOrb/DslUmb at the census ticks.
      const long long cumOrb = (long long)t.capM_Orb - (long long)t.capA_Orb -
                               (long long)t.escM_Orb + (long long)t.escA_Orb;
      const long long cumUmb = (long long)t.capM_Umb - (long long)t.capA_Umb -
                               (long long)t.escM_Umb + (long long)t.escA_Umb;
      printf("                capM   capA   |   escM   escA   |  netD/fr  D_isl(pred)\n");
      printf("  Orbis (w1=0) %5.1f %6.1f | %6.1f %6.1f | %+8.2f  %+9lld\n",
             (double)t.capM_Orb / f, (double)t.capA_Orb / f,
             (double)t.escM_Orb / f, (double)t.escA_Orb / f, netD_orb,
             dIslOrb0_ + cumOrb);
      printf("  Umbra (w1=1) %5.1f %6.1f | %6.1f %6.1f | %+8.2f  %+9lld\n",
             (double)t.capM_Umb / f, (double)t.capA_Umb / f,
             (double)t.escM_Umb / f, (double)t.escA_Umb / f, netD_umb,
             dIslUmb0_ + cumUmb);

      // The headline question: does Umbra shed anti faster than Orbis sheds
      // matter while Orbis keeps netting matter?
      const double escA_Umb = (double)t.escA_Umb / f;
      const double escM_Orb = (double)t.escM_Orb / f;
      const double netM_Orb = ((double)t.capM_Orb - (double)t.escM_Orb) / f;
      const double netA_Umb = ((double)t.capA_Umb - (double)t.escA_Umb) / f;
      printf("  Umbra anti escape rate    %8.2f cells/frame\n", escA_Umb);
      printf("  Orbis matter escape rate  %8.2f cells/frame\n", escM_Orb);
      printf("  Umbra net anti flux to islands %+8.2f cells/frame (neg = shedding)\n", netA_Umb);
      printf("  Orbis net matter flux to islands %+8.2f cells/frame (pos = gaining)\n", netM_Orb);
      printf("----------------------------------------------------------------\n");
    }

    void printReport(const Report& rep)
    {
      printf("\n==============================================================\n");
      printf("ISLAND POPULATION ATTRACTOR REPORT\n");
      printf("==============================================================\n");
      printf("lattice: EL=%u W_USED=%u RMAX=%u ISLAND_SIZE=%u buckets=%u\n",
             EL, W_USED, RMAX, ISLAND_SIZE, rep.nIslands);
      printf("frames sampled: %u\n", rep.frames);
      printf("mean total affiliated population: %.1f cells\n", rep.meanTotalPop);
      printf("gross turnover: captures/frame=%.2f  escapes/frame=%.2f\n",
             rep.meanCaptures, rep.meanEscapes);
      printSectorFlux(fluxTotals(), frames_);

      printf("\nPooled dN ~ N regression (%ld points):\n", rep.pooledPoints);
      if (rep.pooledHasFit)
      {
        printf("  slope     = %+.6g  (+- %.2g)\n", rep.pooledSlope, rep.pooledSlopeSE);
        printf("  intercept = %+.6g\n", rep.pooledIntercept);
        printf("  r2        = %.4f\n", rep.pooledR2);
        if (rep.pooledSlope < 0.0)
          printf("  ==> restoring dynamics detected; N* ~= %.2f\n", rep.pooledNStar);
        else
          printf("  ==> no restoring dynamics detected (slope >= 0)\n");
      }
      else
        printf("  insufficient data\n");

      // Rank islands by |t-stat| of their individual slope.
      std::vector<const IslandStats*> ranked;
      for (const auto& st : rep.islands)
        if (st.samples > 0)
          ranked.push_back(&st);

      std::sort(ranked.begin(), ranked.end(),
                [](const IslandStats* a, const IslandStats* b)
      {
        const double ta = (a->slopeSE > 0) ? std::fabs(a->slope / a->slopeSE) : 0.0;
        const double tb = (b->slopeSE > 0) ? std::fabs(b->slope / b->slopeSE) : 0.0;
        return ta > tb;
      });

      printf("\ntop islands by |slope| significance:\n");
      printf("%6s %10s %9s %9s %9s %+11s %8s %9s %8s\n",
             "island", "meanN", "std", "min", "max", "slope", "r2", "N*", "cap/fr");
      int shown = 0;
      for (const IslandStats* st : ranked)
      {
        if (shown++ >= 12) break;
        printf("%6u %10.2f %9.2f %9.0f %9.0f %+11.3g %8.3f ",
               st->island, st->meanN, st->stdN, st->minN, st->maxN,
               st->slope, st->r2);
        if (st->hasFit && st->slope < 0.0)
          printf("%9.2f ", st->nStar);
        else
          printf("%9s ", "--");
        printf("%8.2f\n", st->capRate);
      }
      printf("==============================================================\n");
    }

    bool sectorCSVToFile(const std::string& path)
    {
      FILE* f = fopen(path.c_str(), "w");
      if (!f) return false;
      fprintf(f, "frame,capM_Orb,capA_Orb,capM_Umb,capA_Umb,escM_Orb,escA_Orb,escM_Umb,escA_Umb\n");
      for (unsigned fr = 0; fr < frames_; ++fr)
      {
        const size_t foff = static_cast<size_t>(fr) * 8;
        fprintf(f, "%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                fr + 1,
                (unsigned long long)flux_[foff + 0],
                (unsigned long long)flux_[foff + 1],
                (unsigned long long)flux_[foff + 2],
                (unsigned long long)flux_[foff + 3],
                (unsigned long long)flux_[foff + 4],
                (unsigned long long)flux_[foff + 5],
                (unsigned long long)flux_[foff + 6],
                (unsigned long long)flux_[foff + 7]);
      }
      fclose(f);
      return true;
    }

    bool writeCSV(const std::string& path, const Report& rep)
    {
      (void)rep;
      FILE* f = fopen(path.c_str(), "w");
      if (!f) return false;
      fprintf(f, "frame,island,population,captures,escapes\n");
      for (unsigned t = 0; t < frames_; ++t)
        for (unsigned g = 0; g < nBuckets_; ++g)
          fprintf(f, "%u,%u,%llu,%llu,%llu\n",
                  t + 1, g,
                  (unsigned long long)pop_[t * nBuckets_ + g],
                  (unsigned long long)cap_[t * nBuckets_ + g],
                  (unsigned long long)esc_[t * nBuckets_ + g]);
      fclose(f);
      return true;
    }

    // Public wrapper: persist the 8-sector-flux series to its own CSV.
    bool writeSectorCSV(const std::string& path)
    {
      return sectorCSVToFile(path);
    }

} } // namespace automaton::attractor

