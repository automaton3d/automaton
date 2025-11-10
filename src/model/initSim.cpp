/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "simulation.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <random>
#include <cassert>
#include <array>

namespace automaton
{
  using namespace std;

  constexpr double PI = std::acos(-1.0);

  // Global variables for lattice (assuming they're vectors, not functions)
  extern std::vector<Cell> lattice_curr;
  extern std::vector<Cell> lattice_draft;
  extern std::vector<Cell> lattice_mirror;

  // Global dimensions
  extern unsigned EL;
  extern unsigned W_DIM;
  extern unsigned W_USED;
  extern unsigned ORDER;
  extern unsigned CENTER;
  extern unsigned FCENTER;
  extern unsigned L2;

  vector<unsigned> dirs;
  vector<WPoint> wpoints;


  extern std::vector<std::array<unsigned, 3>> lcenters;

  /**
   * Function to initialize the lattice with general data.
   */
  void initGeneral()
  {
    for (unsigned w = 0; w < static_cast<unsigned>(W_USED); ++w)
    {
      for (unsigned x = 0; x < static_cast<unsigned>(EL); ++x)
      {
        for (unsigned y = 0; y < static_cast<unsigned>(EL); ++y)
        {
          for (unsigned z = 0; z < static_cast<unsigned>(EL); ++z)
          {
            // Reference to the current cell
            Cell& cell = getCell(lattice_curr, x, y, z, w);
            char w0 = w % 2;
            char w1 = (w >> 1) % 2;
            char q = w0 ^ w1;
            // Set charge
            cell.ch = (w % 8) | (q << 3) | (w0 << 4) | (w1 << 5);
            // Initialize coordinates
            cell.x[0] = x;
            cell.x[1] = y;
            cell.x[2] = z;
            cell.x[3] = w;
            cell.a = w;
            // Reference to the central cell in the current layer
            Cell& cell0 = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
            // Initialize properties of the cell
            cell.ch = cell0.ch;
            // Calculate distance from center
            double dx = static_cast<double>(x) - static_cast<double>(CENTER);
            double dy = static_cast<double>(y) - static_cast<double>(CENTER);
            double dz = static_cast<double>(z) - static_cast<double>(CENTER);
            double d  = std::sqrt(dx * dx + dy * dy + dz * dz);
            // Set Euclidean distance
            cell.d = static_cast<unsigned>(std::ceil(d));
          }
        }
      }
    }
    // Initialize lattice_mirror for safety
    lattice_mirror.assign(lattice_mirror.size(), Cell());
    puts("initGeneral ok.");
  }

  /*
   * initMomentum function fixed to use uniform sphere points generation.
   * Ensures isotropic distribution with slight z-bias.
   * Marks lines from center to peripheral points using markPoints.
   */
  void initMomentum()
  {
    const unsigned cx = CENTER;
    const unsigned cy = CENTER;
    const unsigned cz = CENTER;
    auto tuples = generateShell(static_cast<int>(EL));
    unsigned n = static_cast<unsigned>(std::floor(1 / 0.10483));
    unsigned w = 0;
    for (unsigned i = 0; i < n; i++)
    {
      for (auto& t : tuples)
      {
    	if (w >= W_USED)
    	  break;
    	int xx, yy, zz;
        std::tie(xx, yy, zz) = t;
        int dx = xx - static_cast<int>(cx);
        int dy = yy - static_cast<int>(cy);
        int dz = zz - static_cast<int>(cz);
        unsigned ux = static_cast<unsigned>(dx + CENTER);
        unsigned uy = static_cast<unsigned>(dy + CENTER);
        unsigned uz = static_cast<unsigned>(dz + CENTER);
        dirs.push_back(ux);
        dirs.push_back(uy);
        dirs.push_back(uz);
        unsigned p[3] = { ux, uy, uz };
        markPoints(p, static_cast<unsigned>(w));
        w++;
      }
    }
    std::vector<std::pair<double, size_t>> weighted_indices;
    if (w < W_DIM)
    {
      // Second loop: distribute remaining points with cos^2(theta) distribution (deterministic)
      weighted_indices.reserve(tuples.size());
      for (size_t i = 0; i < tuples.size(); ++i)
      {
        int xx, yy, zz;
        std::tie(xx, yy, zz) = tuples[i];
        int dx = xx - static_cast<int>(cx);
        int dy = yy - static_cast<int>(cy);
        int dz = zz - static_cast<int>(cz);
        // Calculate cos^2(theta) where theta is angle from z-axis
        double r = std::sqrt(dx*dx + dy*dy + dz*dz);
        double cos_theta = (r > 0) ? std::abs(dz) / r : 0.0;
        double weight = cos_theta * cos_theta;
        weighted_indices.push_back({weight, i});
      }
    }
    else
    {
      perror("Warning: W too small for this simulation.");
    }
    // Sort by weight (descending - highest weights first)
    std::sort(weighted_indices.begin(), weighted_indices.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });
    // Calculate how many times each point should appear based on its weight
    unsigned remaining = W_USED - w;  // FIX 3: use unsigned
    double total_weight = 0.0;
    for (const auto& wi : weighted_indices)
    {
      total_weight += wi.first;
    }
    // Distribute remaining slots proportionally to weights
    std::vector<unsigned> counts(tuples.size(), 0);  // FIX 4: use unsigned
    unsigned allocated = 0;
    for (size_t i = 0; i < weighted_indices.size() && allocated < remaining; ++i)
    {
      double proportion = weighted_indices[i].first / total_weight;
      unsigned count = static_cast<unsigned>(std::round(proportion * remaining));
      count = std::max(1u, count); // Each point appears at least once
      counts[weighted_indices[i].second] = count;
      allocated += count;
    }
    // Adjust if we over/under allocated due to rounding
    while (allocated > remaining)
    {
      // Remove from the point with lowest weight that still has count > 1
      for (int i = weighted_indices.size() - 1; i >= 0 && allocated > remaining; --i)
      {
        if (counts[weighted_indices[i].second] > 1)
        {
          counts[weighted_indices[i].second]--;
          allocated--;
        }
      }
    }
    while (allocated < remaining)
    {
      // Add to the point with highest weight
      for (size_t i = 0; i < weighted_indices.size() && allocated < remaining; ++i)
      {
        counts[weighted_indices[i].second]++;
        allocated++;
      }
    }
    // FIX 6: Iterate over weighted_indices, not tuples
    for (size_t i = 0; i < weighted_indices.size() && w < W_USED; ++i)
    {
      size_t idx = weighted_indices[i].second;
      for (unsigned c = 0; c < counts[idx] && w < W_USED; ++c)
      {
        int xx, yy, zz;
        std::tie(xx, yy, zz) = tuples[idx];
        int dx = xx - static_cast<int>(cx);
        int dy = yy - static_cast<int>(cy);
        int dz = zz - static_cast<int>(cz);
        unsigned ux = static_cast<unsigned>(dx + CENTER);
        unsigned uy = static_cast<unsigned>(dy + CENTER);
        unsigned uz = static_cast<unsigned>(dz + CENTER);
        dirs.push_back(ux);
        dirs.push_back(uy);
        dirs.push_back(uz);
        unsigned p[3] = { ux, uy, uz };
        markPoints(p, static_cast<unsigned>(w));
        w++;
      }
    }


    printf("initMomentum: requested W_USED=%u, actually initialized w=%u\n", W_USED, w);

        if (w < W_USED) {
            fprintf(stderr, "ERROR: Only %u/%u layers were initialized!\n", w, W_USED);
            fprintf(stderr, "Available shell points: %zu\n", tuples.size());
        }
        printf("initMomentum: W_USED=%u, dirs.size()=%zu, w=%u\n",
                   W_USED, dirs.size(), w);
            puts("initMomentum ok.");  // ← Add this to match other functions
            }

  void initSine2()
  {
    double cx = EL / 2.0;
    double cy = cx;
    double cz = cx;
    double R = EL / 2.0;
    // Fixed seed for deterministic behavior
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    for (unsigned w = 0; w < W_USED; w++)
    {
      for (unsigned i = 0; i < EL; ++i)
      {
        double dx = i - cx;
        for (unsigned j = 0; j < EL; ++j)
        {
          double dy = j - cy;
          for (unsigned k = 0; k < EL; ++k)
          {
            double dz = k - cz;
            double r = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (r > R + 1e-6) continue; // Small epsilon for floating-point precision
            double theta = PI * r / R;
            double prob = std::sin(theta) * std::sin(theta);
            if (dis(gen) < prob)
            {
              getCell(lattice_curr, i, j, k, w).phiB = true;
            }
          }
        }
      }
    }
    puts("initSine2 ok.");
  }

  // Helper: rotate vector p around axis k by angle theta (Rodrigues' formula)
  void rotateAroundAxis(const double p[3], const double k[3], double theta, double result[3])
  {
    double cosT = cos(theta);
    double sinT = sin(theta);
    // k × p
    double cross[3] =
    {
      k[1]*p[2] - k[2]*p[1],
      k[2]*p[0] - k[0]*p[2],
      k[0]*p[1] - k[1]*p[0]
    };
    // k · p
    double dot = k[0]*p[0] + k[1]*p[1] + k[2]*p[2];
    // Rodrigues rotation
    for (int i = 0; i < 3; ++i)
    {
      result[i] = p[i]*cosT + cross[i]*sinT + k[i]*dot*(1 - cosT);
    }
  }

  void initSpirals()
  {
	   if (dirs.size() < 3 * W_USED) {
	        fprintf(stderr, "FATAL: dirs[] has only %zu elements, need %u\n",
	                dirs.size(), 3 * W_USED);
	        fprintf(stderr, "Some layers will have uninitialized spiral directions!\n");
	        return;  // Don't crash with out-of-bounds access
	    }    const int num_points = 10 * EL;
    std::vector<double> theta(num_points);
    std::vector<double> r(num_points);
    std::vector<double> x_curve(num_points);
    std::vector<double> y_curve(num_points);
    std::vector<double> z_curve(num_points);
    // Construir espiral base ao redor do eixo Z
    for (int i = 0; i < num_points; ++i)
    {
       theta[i] = 2 * M_PI * i / num_points;
       r[i] = (double)EL / (4 * M_PI) * theta[i];
       x_curve[i] = r[i] * cos(theta[i]);
       y_curve[i] = r[i] * sin(theta[i]);
       z_curve[i] = r[i];
    }
    const double cx = EL / 2.0;
    const double cy = EL / 2.0;
    const double cz = EL / 2.0;
    // Para cada direção em dirs[], rotacionar a espiral
    for (unsigned w = 0; w < W_USED; ++w)
    {
      // Usar dirs[] diretamente (já preenchido por initMomentum)
      size_t base = w * 3;
      double k[3] =
      {
        static_cast<double>(dirs[base]) - cx,
        static_cast<double>(dirs[base + 1]) - cy,
        static_cast<double>(dirs[base + 2]) - cz
      };
      normalize(k);
      // Eixo de rotação: de Z para k
      double z_axis[3] = { 0.0, 0.0, 1.0 };
      // Produto vetorial z × k
      double axis[3] =
      {
        z_axis[1]*k[2] - z_axis[2]*k[1],
        z_axis[2]*k[0] - z_axis[0]*k[2],
        z_axis[0]*k[1] - z_axis[1]*k[0]
      };
      double axis_len = sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
      if (axis_len < 1e-12)
      {
        // Paralelo a Z, sem rotação
        axis[0] = 1.0; axis[1] = 0.0; axis[2] = 0.0;
        axis_len = 1.0;
      }
      axis[0] /= axis_len;
      axis[1] /= axis_len;
      axis[2] /= axis_len;
      // Ângulo de rotação
      double dot = z_axis[0]*k[0] + z_axis[1]*k[1] + z_axis[2]*k[2];
      dot = std::max(-1.0, std::min(1.0, dot));
      double angle = acos(dot);
      // Rotacionar e mapear pontos da espiral
      for (int i = 0; i < num_points; ++i)
      {
        double p[3] = { x_curve[i], y_curve[i], z_curve[i] };
        double pr[3];
        rotateAroundAxis(p, axis, angle, pr);
        unsigned x = static_cast<unsigned>(round(pr[0] + cx));
        unsigned y = static_cast<unsigned>(round(pr[1] + cy));
        unsigned z = static_cast<unsigned>(round(pr[2] + cz));
        if (x < EL && y < EL && z < EL)
        {
          getCell(lattice_curr, x, y, z, w).sB = true;
        }
      }
    }
    printf("initSpirals ok - %u espirais mapeadas\n", W_USED);
  }

  /*
   * Replicate data.
   */
  void replicate()
  {
    // Deep copy
    std::copy(lattice_curr.begin(),
            lattice_curr.begin() + BLOCK,
            lattice_draft.begin());
    //
    puts("initDraft ok");
  }

  /**
   * Executes each initialization step.
   *
   * @step the selected phase
   */
  bool initSimulation(int step)
  {
    switch(step)
    {
      case 0:
        initGeneral();
        break;
      case 1:
        initMomentum();
        break;
      case 2:
        initSpirals();
        break;
      case 3:
        initSine2();
        break;
      case 4:
        printParams();
        break;
      case 5:
        replicate();
        std::copy(lattice_curr.begin(),
                  lattice_curr.begin() + BLOCK,
                  lattice_mirror.begin());
        break;
      case 6:
        assert(sanityTest());
        break;
      default:
      return true;
    }
    return false;
  }

  /**
   * The tentative allocation of memory for the simulation.
   *
   * @EL grid size
   * @W number of layers
   */
  bool tryAllocate(int EL, int W)
  {
    try
    {
      size_t total = static_cast<size_t>(EL) * EL * EL * W;
      lattice_curr.resize(total);
      lattice_draft.resize(total);
      lattice_mirror.resize(total);
      return true;
    }
    catch (const std::bad_alloc& e)
    {
      lastAllocationError = "Memory allocation failed: " + std::string(e.what());
      std::cerr << lastAllocationError << std::endl;
      return false;
    }
    catch (...)
    {
      lastAllocationError = "Unknown error during memory allocation";
      std::cerr << lastAllocationError << std::endl;
      return false;
    }
  }

  void initCenters(unsigned wDim)
  {
    lcenters.resize(wDim);
    for (unsigned w = 0; w < wDim; ++w)
    {
      lcenters[w][0] = CENTER;
      lcenters[w][1] = CENTER;
      lcenters[w][2] = CENTER;
    }
  }

  /**
   * Calculates the simulation parameters.
   */
  void calculateParameters(unsigned L, unsigned W)
  {
    EL        = L;
    L2        = (EL*EL);
    W_DIM     = (3 * L2 + 1);
    W_USED    = W;
    L3        = L2 * EL;
    ORDER     = ((int)round(log2(EL)));   // Number of bits
    CENTER    = ((EL - 1) / 2);
    FCENTER   = (EL/2.0);
    BLOCK     = L3 * W_USED;
    DIAG      = (unsigned) EL* sqrt(3);
    RMAX      = DIAG / 2;
    CONTRACT  = static_cast<int>(floor(sqrt(3.0) * CENTER));
    // Time slots
    CONVOL    = W_USED;
    SLOT1     = CONVOL + RMAX;
    SLOT2     = SLOT1 + 3 * (EL - 1);
    SLOT3     = SLOT2 + 3 * (EL - 1);
    SLOT4     = SLOT3 + 2 * W_USED;
    SLOT5     = SLOT4 + 3 * (L - 1);
    DIFFUSION = SLOT5;
    SLOT6     = DIFFUSION + (EL - 1);
    SLOT7     = SLOT6 + (EL - 1);
    SLOT8     = SLOT7 + (EL - 1);
    RELOC     = SLOT8;
    REISSUE   = RELOC + 1;
    FLOOD     = REISSUE + 3 * (L - 1);
    FRAME     = FLOOD;

    // Centers will be used by the frame recorder
    initCenters(W_USED);
  }
}
