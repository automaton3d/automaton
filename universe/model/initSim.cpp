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
  extern unsigned CENTER;

  vector<unsigned> dirs;
  vector<WPoint> wpoints;

  /**
   * Initialize the center cells.
   */
  void initCell0()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      // Access the central cell in the current layer (w-slice)
      Cell& cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      // Set charge and affinity values
      cell.ch = (w % 8) | (q << 3) | (w0 << 4) | (w1 << 5);
    }
    puts("initCell0 ok.");
  }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
    for (unsigned w = 0; w < static_cast<unsigned>(W_DIM); ++w)
    {
      for (unsigned x = 0; x < static_cast<unsigned>(EL); ++x)
      {
        for (unsigned y = 0; y < static_cast<unsigned>(EL); ++y)
        {
          for (unsigned z = 0; z < static_cast<unsigned>(EL); ++z)
          {
            // Reference to the current cell
            Cell& cell = getCell(lattice_curr, x, y, z, w);
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

  /**
   * Ensures wpoints is correctly sized and populated with data.
   * NOTE: Replace the loop content with your actual spiral point generation logic.
   */
  void populateWPoints()
  {
    // Resize the vector to the required number of layers
    wpoints.resize(W_DIM);
    // Placeholder Logic (MUST BE REPLACED)
    // The actual coordinates should come from your model's geometry.
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Assign simple, safe, non-zero coordinates to prevent memory read failure
      wpoints[w].p.x = 1;
      wpoints[w].p.y = 0;
      wpoints[w].p.z = 0;
    }
  }

  void initMomentum()
  {
    if (wpoints.empty() || wpoints.size() != W_DIM)
    {
      populateWPoints();
    }
    if (dirs.empty() || dirs.size() != W_DIM * 3)
    {
      dirs.resize(W_DIM * 3);
    }
    const double R = EL / 2.0;
    const int cx = EL / 2, cy = EL / 2, cz = EL / 2;
    std::vector<Point> points;
    points.reserve(EL * EL * 6); // rough upper bound for shell points
    // 1) Generate discrete spherical shell points
    for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
        {
          const unsigned dx = x - cx, dy = y - cy, dz = z - cz;
          const unsigned r_int = static_cast<unsigned>(std::round(std::sqrt(dx*dx + dy*dy + dz*dz)));
          if (r_int == static_cast<unsigned>(R))
            points.push_back({static_cast<unsigned>(x), static_cast<unsigned>(y), static_cast<unsigned>(z)});
        }
    struct WeightedPoint { Point p; double weight; };
    std::vector<WeightedPoint> wpoints;
    wpoints.reserve(points.size());
    // Adjustable bias parameter (0.3 = mild, 0.5 = moderate, 1.0 = strong)
    const double BIAS_STRENGTH = 0.3;  // Small bias for monopoles
    for (const auto& pt : points)
    {
      const double dz = pt.z - cz;
      // Polar angle from z-axis: 0 at north pole, π at south pole
      const double cos_polar = dz / R;  // Ranges from -1 (south) to +1 (north)
      // Weight favors poles: high weight at |cos_polar| ≈ 1, low at equator
      // Use cos²(polar_angle) raised to BIAS_STRENGTH
      const double w = std::pow(cos_polar * cos_polar, BIAS_STRENGTH);
      wpoints.push_back({pt, w});
    }
    // Sort by weight (descending - highest weight first = polar points)
    std::sort(wpoints.begin(), wpoints.end(),
         [](const WeightedPoint& a, const WeightedPoint& b){ return a.weight > b.weight; });
    // Keep the highest-weighted points (polar bias for monopoles)
    const size_t target = std::min<size_t>(wpoints.size(), static_cast<size_t>(W_DIM));
    // Keep high-weight (polar) points by removing from END
    if (wpoints.size() > target)
    {
      wpoints.erase(wpoints.begin() + target, wpoints.end());
    }
    std::cout << "W_DIM (requested): " << W_DIM << "\n";
    std::cout << "Shell points: " << points.size() << "\n";
    std::cout << "Selected points: " << wpoints.size() << "\n";
    std::cout << "Polar bias strength: " << BIAS_STRENGTH
             << " (0.3=mild, 0.5=moderate, 1.0=strong)\n";
    std::cout << "Distribution: favors north/south poles for monopole simulation\n";
    // 5) Assign to dirs[] and mark lattice
    const size_t count = std::min<size_t>(W_DIM, wpoints.size());
    for (size_t w = 0; w < count; ++w)
    {
      dirs[w * 3 + 0] = wpoints[w].p.x;
      dirs[w * 3 + 1] = wpoints[w].p.y;
      dirs[w * 3 + 2] = wpoints[w].p.z;
      if (wpoints[w].p.x >= 0 && wpoints[w].p.x < EL && wpoints[w].p.y >= 0 && wpoints[w].p.y < EL && wpoints[w].p.z >= 0 && wpoints[w].p.z < EL)
      {
        unsigned p[3] =
        {
          static_cast<unsigned>(wpoints[w].p.x),
          static_cast<unsigned>(wpoints[w].p.y),
          static_cast<unsigned>(wpoints[w].p.z)
        };
        markPoints(p, static_cast<unsigned>(w));
      }
    }
    puts("initMomentum ok (polar bias for monopole simulation)");
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
    for (unsigned w = 0; w < W_DIM; w++)
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

  // In initSim.cpp (after globals, before initSpirals)

  // Rotate spiral around all dirs vectors and mark lattice
  void initSpirals()
  {
    // FIX 1: Ensure wpoints is populated BEFORE access
    if (wpoints.empty() || wpoints.size() != W_DIM)
    {
      populateWPoints();
    }
    // FIX 2: Resize dirs (if not done elsewhere)
    if (dirs.empty() || dirs.size() != W_DIM * 3)
    {
      dirs.resize(W_DIM * 3);
    }
    const int num_points = 10 * EL;
    double theta[num_points];
    double r[num_points], x_curve[num_points], y_curve[num_points], z_curve[num_points];
    // Build base spiral around Z-axis
    for (int i = 0; i < num_points; ++i)
    {
      theta[i] = 2 * M_PI * i / num_points;
      r[i] = (double)EL / (4 * M_PI) * theta[i];
      x_curve[i] = r[i] * cos(theta[i]);
      y_curve[i] = r[i] * sin(theta[i]);
      z_curve[i] = r[i];
    }
    // Centering offset
    double cx = EL / 2.0;
    double cy = EL / 2.0;
    double cz = EL / 2.0;
    // For each direction vector, rotate entire spiral
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Patch
       dirs[w * 3 + 0] = wpoints[w].p.x; // Now this index is valid
      dirs[w * 3 + 1] = wpoints[w].p.y;
      dirs[w * 3 + 2] = wpoints[w].p.z;
      //
       size_t base = w * 3;
      double k[3] = { (double)dirs[base], (double)dirs[base + 1], (double)dirs[base + 2] };
      normalize(k);
      // Determine rotation axis: we need to rotate from Z-axis to dirs[w]
      double z_axis[3] = { 0.0, 0.0, 1.0 };
      // Compute cross product z × k (rotation axis)
      double axis[3] =
      {
        z_axis[1]*k[2] - z_axis[2]*k[1],
        z_axis[2]*k[0] - z_axis[0]*k[2],
        z_axis[0]*k[1] - z_axis[1]*k[0]
      };
      double axis_len = sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
      if (axis_len < 1e-12)
      {
        // If dirs[w] is parallel to z, no rotation is needed
        axis[0] = 1.0; axis[1] = 0.0; axis[2] = 0.0;
        axis_len = 1.0;
      }
      axis[0] /= axis_len; axis[1] /= axis_len; axis[2] /= axis_len;
      // Rotation angle
      double dot = z_axis[0]*k[0] + z_axis[1]*k[1] + z_axis[2]*k[2];
      if (dot > 1.0) dot = 1.0;
      if (dot < -1.0) dot = -1.0;
      double angle = acos(dot);
      // Rotate and map points
      for (int i = 0; i < num_points; ++i)
      {
        double p[3] = { x_curve[i], y_curve[i], z_curve[i] };
        double pr[3];
        rotateAroundAxis(p, axis, angle, pr);
        // Translate to lattice coordinates
        unsigned x = (unsigned)round(pr[0] + cx);
        unsigned y = (unsigned)round(pr[1] + cy);
        unsigned z = (unsigned)round(pr[2] + cz);
        if (x >= 0 && x < EL && y >= 0 && y < EL && z >= 0 && z < EL)
        {
          getCell(lattice_curr, x, y, z, w).sB = true;
        }
      }
    }
    printf("initSpiralAndRotate: Spiral mapped to %u directions.\n", W_DIM);
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
    // Check lattice allocation
    if (lattice_curr.empty() || lattice_draft.empty() || lattice_mirror.empty())
    {
      std::cerr << "Error: Lattices not allocated. Ensure calculateParameters and allocate_lattices are called." << std::endl;
      return false;
    }
    std::cout << "Starting initSimulation step " << step << std::endl;
    switch(step)
    {
      case 0:
        initCell0();
        break;
      case 1:
        initGeneral();
        break;
      case 2:
        initMomentum();
        break;
      case 3:
        initSpirals();
        break;
      case 4:
        initSine2();
        break;
      case 5:
        printConstants();
        break;
      case 6:
        replicate();
        std::copy(lattice_curr.begin(),
                  lattice_curr.begin() + BLOCK,
                  lattice_mirror.begin());
        break;
      case 7:
        assert(sanityTest());
        break;
      default:
      return true;
    }
    std::cout << "Completed initSimulation step " << step << std::endl;
    return false;
  }

  void allocate_lattices(int EL, int W_DIM)
  {
    size_t total = static_cast<size_t>(EL) * EL * EL * W_DIM;
    lattice_curr.resize(total);
    lattice_draft.resize(total);
    lattice_mirror.resize(total);
  }
}
