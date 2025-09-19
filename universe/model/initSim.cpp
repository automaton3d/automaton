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
  extern Cell lattice_curr[EL][EL][EL][W_DIM];
  extern Cell lattice_draft[EL][EL][EL][W_DIM];

  unsigned dirs[W_DIM][3];

  /**
   * Initialize the center cells.
   */
  void initCell0()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      // Access the central cell in the current layer (w-slice)
      Cell& cell = lattice_curr[CENTER][CENTER][CENTER][w];
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      // Set charge and affinity values
      cell.ch = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
    }
    puts("initCell0 ok.");
  }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      for (unsigned x = 0; x < EL; x++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned z = 0; z < EL; z++)
          {
            // Reference to the current cell
            Cell& cell = lattice_curr[x][y][z][w];
            // Initialize coordinates
            cell.x[0] = x;
            cell.x[1] = y;
            cell.x[2] = z;
            cell.x[3] = w;
            cell.a = w;
            // Reference to the central cell in the current layer
            Cell& cell0 = lattice_curr[CENTER][CENTER][CENTER][w];
            // Initialize properties of the cell
            cell.ch = cell0.ch;
            // Calculate distance from center
            double dx = static_cast<double>(x - (double)CENTER);
            double dy = static_cast<double>(y - (double)CENTER);
            double dz = static_cast<double>(z - (double)CENTER);
            double d = sqrt(dx * dx + dy * dy + dz * dz);
            // Set Euclidean distance
            cell.d = static_cast<unsigned>(d);
          }
        }
      }
    }
    puts("initGeneral ok.");
  }

  /*
   * Initializes charges and affinity in the central
   * cell of every layer.
   */
  void initCharges()
  {
    for (unsigned w = 0; w < W_DIM; w++)
    {
      // Access the central cell in the current layer (w-slice)
      Cell& cell = lattice_curr[CENTER][CENTER][CENTER][w];
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      // Set charge and affinity values
      cell.ch = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
    }
    puts("initCharges ok.");
  }

  void initMomentum()
  {
      struct Point { int x, y, z; };
      const double R = EL / 2.0;
      const int cx = EL / 2, cy = EL / 2, cz = EL / 2;
      std::vector<Point> points;
      points.reserve(EL * EL * 6); // rough upper bound for shell points
      // 1) Generate discrete spherical shell points
      for (int x = 0; x < EL; ++x)
        for (int y = 0; y < EL; ++y)
          for (int z = 0; z < EL; ++z)
          {
            const int dx = x - cx, dy = y - cy, dz = z - cz;
            const int r_int = static_cast<int>(std::round(std::sqrt(dx*dx + dy*dy + dz*dz)));
            if (r_int == static_cast<int>(R))
              points.push_back({x, y, z});
          }
      struct WeightedPoint { Point p; double weight; };
      std::vector<WeightedPoint> wpoints;
      wpoints.reserve(points.size());
      for (const auto& pt : points)
      {
        const double dz = pt.z - cz;
        const double theta = M_PI/2.0 + (dz / R) * M_PI; // [-R,R] -> [pi/2,3pi/2]
        const double w = std::sin(theta);
        wpoints.push_back({pt, w*w}); // sin^2
      }
      std::sort(wpoints.begin(), wpoints.end(),
           [](const WeightedPoint& a, const WeightedPoint& b){ return a.weight > b.weight; });
      // --- Clamp to the number we actually need/have ---
      const size_t target = std::min<size_t>(wpoints.size(), static_cast<size_t>(W_DIM));
      size_t N = (wpoints.size() > target) ? (wpoints.size() - target) : 0;
      // Safe erase range
      wpoints.erase(wpoints.begin(), wpoints.begin() + static_cast<std::ptrdiff_t>(N));
      std::cout << "W_DIM (requested): " << W_DIM << "\n";
      std::cout << "Shell points: " << points.size() << "\n";
      std::cout << "Remaining points (available): " << wpoints.size() << "\n";
      // Use the min to avoid out-of-bounds
      const size_t count = std::min<size_t>(W_DIM, wpoints.size());
      for (size_t w = 0; w < count; ++w)
      {
        dirs[w][0] = wpoints[w].p.x;
        dirs[w][1] = wpoints[w].p.y;
        dirs[w][2] = wpoints[w].p.z;
        unsigned p[3] =
        {
          static_cast<unsigned>(wpoints[w].p.x),
          static_cast<unsigned>(wpoints[w].p.y),
          static_cast<unsigned>(wpoints[w].p.z)
        };
        markPoints(p, static_cast<unsigned>(w));
      }
      puts("initMomentum ok");
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
    for (int w = 0; w < W_DIM; w++)
    {
      for (int i = 0; i < EL; ++i)
      {
        double dx = i - cx;
        for (int j = 0; j < EL; ++j)
        {
          double dy = j - cy;
          for (int k = 0; k < EL; ++k)
          {
            double dz = k - cz;
            double r = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (r > R + 1e-6) continue; // Small epsilon for floating-point precision
            double theta = PI * r / R;
            double prob = std::sin(theta) * std::sin(theta);
            if (dis(gen) < prob)
            {
              lattice_curr[i][j][k][w].phiB = true;
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

  // Rotate spiral around all dirs vectors and mark lattice
  void initSpirals()
  {
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
      double k[3] = { (double)dirs[w][0], (double)dirs[w][1], (double)dirs[w][2] };
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
        int x = (int)round(pr[0] + cx);
        int y = (int)round(pr[1] + cy);
        int z = (int)round(pr[2] + cz);
        if (x >= 0 && x < EL && y >= 0 && y < EL && z >= 0 && z < EL)
        {
          lattice_curr[x][y][z][w].sB = true;
        }
      }
    }
    printf("initSpiralAndRotate: Spiral mapped to %u directions.\n", W_DIM);
  }

  /**
   * Executes each initialization step.
   */
  bool initSimulation(int step)
  {
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
        //printLattice(0);
        break;
	  case 5:
        //printConstants();
        break;
      case 6:
        std::copy(&lattice_curr[0][0][0][0],
        &lattice_curr[0][0][0][0] + BLOCK,
        &lattice_draft[0][0][0][0]);
        puts("initDraft ok");
        break;
      case 7:
        // Check consistency
        //assert(sanityTest1());
        break;
      default:
    	return true;
	}
    return false;
  }

}
