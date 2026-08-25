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
      unsigned nBuckets_ = 0;
      unsigned frames_   = 0;

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
      pop_.clear(); cap_.clear(); esc_.clear();
      frames_ = 0;

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

      std::fill(pop_.begin() + off, pop_.end(), 0ull);
      std::fill(cap_.begin() + off, cap_.end(), 0ull);
      std::fill(esc_.begin() + off, esc_.end(), 0ull);

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
        if (pv == ORPHAN && now != ORPHAN)
          ++cap_[off + now];
        else if (pv != ORPHAN && now == ORPHAN)
          ++esc_[off + pv];
        else if (pv != ORPHAN && now != ORPHAN && pv != now)
        {
          ++esc_[off + pv];   // island switch = escape + capture
          ++cap_[off + now];
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
      uint32_t hdr[2] = { frames_, nBuckets_ };
      fwrite(hdr, sizeof(hdr), 1, f);
      if (frames_)
      {
        fwrite(pop_.data(), sizeof(uint64_t), pop_.size(), f);
        fwrite(cap_.data(), sizeof(uint64_t), cap_.size(), f);
        fwrite(esc_.data(), sizeof(uint64_t), esc_.size(), f);
      }
      fclose(f);
      return true;
    }

    bool loadSeries(const std::string& path)
    {
      FILE* f = fopen(path.c_str(), "rb");
      if (!f) return false;
      uint32_t hdr[2] = { 0, 0 };
      if (fread(hdr, sizeof(hdr), 1, f) != 1 ||
          hdr[1] != nBuckets_)
      {
        fclose(f);
        return false;
      }
      frames_ = hdr[0];
      const size_t n = static_cast<size_t>(frames_) * nBuckets_;
      pop_.resize(n); cap_.resize(n); esc_.resize(n);
      bool ok = true;
      if (n)
        ok = fread(pop_.data(), sizeof(uint64_t), n, f) == n &&
             fread(cap_.data(), sizeof(uint64_t), n, f) == n &&
             fread(esc_.data(), sizeof(uint64_t), n, f) == n;
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

} } // namespace automaton::attractor

