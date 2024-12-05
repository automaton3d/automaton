/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "simulation.h"
#include "poincare.h"

namespace automaton
{
  using namespace std;

  const double epsilon = 1e-9;

  // Global variables for lattice (assuming they're vectors, not functions)
  extern Cell lattice_curr[SIDE][SIDE][SIDE][W_DIM];
  extern Cell lattice_draft[SIDE][SIDE][SIDE][W_DIM];
  extern EntropyCalculator entropyCalc;


  /*
   * Generate spin vectors.
   */
  void initSpin()
  {
      const int r_max = CENTER;
      const int n_steps = CENTER;
      const double step_size = 1.0 / n_steps;

      // Get the center cell in the first layer of the lattice
      Cell& center_cell = lattice_curr[CENTER][CENTER][CENTER][0];
      unsigned w = 0; // Counter for the number of spins initialized
      // Iterate through the lattice to initialize spin vectors
      for (int x = 0; x < SIDE; ++x)
      {
          for (int y = 0; y < SIDE; ++y)
          {
              for (int z = 0; z < SIDE; ++z)
              {
                  int dx = x - CENTER;
                  int dy = y - CENTER;
                  int dz = z - CENTER;
                  int distance_squared = dx * dx + dy * dy + dz * dz;

                  // Assign spin vectors based on distance thresholds
                  for (int i = 0; i < n_steps; ++i)
                  {
                      double lower_bound = r_max - 1 + i * step_size;
                      double upper_bound = r_max - 1 + (i + 1) * step_size;
                      double lower_bound_sq = lower_bound * lower_bound;
                      double upper_bound_sq = upper_bound * upper_bound;

                      if (distance_squared >= lower_bound_sq && distance_squared < upper_bound_sq)
                      {
                          center_cell.s[0] = x;
                          center_cell.s[1] = y;
                          center_cell.s[2] = z;
                          ++w;

                          // Move to the next cell in a valid manner
                          if (center_cell.pos[0] >= 0 && center_cell.pos[0] < SIDE &&
                              center_cell.pos[1] >= 0 && center_cell.pos[1] < SIDE &&
                              center_cell.pos[2] >= 0 && center_cell.pos[2] < SIDE)
                          {
                              center_cell = lattice_curr[center_cell.pos[0]][center_cell.pos[1]][center_cell.pos[2]][0];
                          }
                          else
                          {
                              cerr << "Error: Invalid cell position encountered!" << endl;
                              exit(EXIT_FAILURE);
                          }

                          // Stop if the required number of spins have been initialized
                          if (w == W_DIM)
                          {
                              return;
                          }
                      }
                  }
              }
          }
      }

      // Error handling if no spins were initialized
      if (w == 0)
      {
          cerr << "Error: No spins initialized. Check lattice dimensions and thresholds!" << endl;
          exit(EXIT_FAILURE);
      }
  }

  /*
   * Generates momentum vectors from spin.
   */
  void initMomentum()
  {
      for (unsigned w = 0; w < W_DIM; w++)
      {
          // Access the cell at the center of the current w-slice
          Cell& cell = lattice_curr[CENTER][CENTER][CENTER][w];

          double dx = cell.s[0] - FCENTER;
          double dy = cell.s[1] - FCENTER;
          double dz = cell.s[2] - FCENTER;
          double s[3] = { dx, dy, dz };
          normalize(s); // Normalizes the spin vector `s`
          double v[3];

          if (fabs(s[0]) < epsilon && fabs(s[1]) < epsilon)
          {
              v[0] = 0.0;
              v[1] = 1.0;
              v[2] = 0.0;
          }
          else
          {
              v[0] = 1.0;
              v[1] = 0.0;
              v[2] = 0.0;
          }

          // Compute orthogonal vectors to `s`
          double p1[3];
          cross_product(p1, s, v);
          normalize(p1);

          double p2[3];
          cross_product(p2, s, p1);
          normalize(p2);

          // Calculate lattice positions for marking poles
          unsigned p[3];
          p[0] = (unsigned)((p2[0] + 1.0) * FCENTER);
          p[1] = (unsigned)((p2[1] + 1.0) * FCENTER);
          p[2] = (unsigned)((p2[2] + 1.0) * FCENTER);

          // Validate indices and mark poles
          assert(p[0] < SIDE && p[1] < SIDE && p[2] < SIDE);
          markPoles(p, w);
      }

      puts("\tinitMomentum ok.");
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
          cell.charge = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
          cell.aff = w + 1; // Avoid zero

          // Optional: Add debug/logging if needed
          // printf("Layer %u: charge=%d, aff=%d\n", w, cell.charge, cell.aff);
      }

      puts("initCharges ok.");
  }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
      for (unsigned w = 0; w < W_DIM; w++)
      {
          for (unsigned x = 0; x < SIDE; x++)
          {
              for (unsigned y = 0; y < SIDE; y++)
              {
                  for (unsigned z = 0; z < SIDE; z++)
                  {
                      // Reference to the current cell
                      Cell& cell = lattice_curr[x][y][z][w];

                      // Reference to the central cell in the current layer
                      Cell& cell0 = lattice_curr[CENTER][CENTER][CENTER][w];

                      // Initialize properties of the cell
                      cell.charge = cell0.charge;
                      cell.s[0] = cell0.s[0];
                      cell.s[1] = cell0.s[1];
                      cell.s[2] = cell0.s[2];
                      cell.pos[0] = x;
                      cell.pos[1] = y;
                      cell.pos[2] = z;

                      // Calculate distance from center
                      double dx = static_cast<double>(x - FCENTER);
                      double dy = static_cast<double>(y - FCENTER);
                      double dz = static_cast<double>(z - FCENTER);
                      double d = sqrt(dx * dx + dy * dy + dz * dz);

                      cell.d = static_cast<unsigned>(LIGHT * d);
                      cell.freq = 1;

                      assert(cell.freq < FMAX);

                      // Calculate scaled sine value
                      double sin_scaled = SIDE * fabs(sin(2 * M_PI * d / SIDE));
                      assert(sin_scaled >= 0 && sin_scaled <= UINT_MAX);

                      cell.sin = static_cast<unsigned>(sin_scaled);
                  }
              }
          }
      }

      puts("initGeneral ok.");
  }

  void initSimulation()
  {
	  // printConstants();
      initCharges();
      initSpin();
      initMomentum();
      initGeneral();
      initPoincare();
      saveState0();
      checkPoincare();
      // Calculate the initial entropy of the universe
      entropyCalc.collectData();
      float H0 = entropyCalc.computeEntropy();
      Entropy e = entropyCalc.getEntropy();
      e.setMinEntropy(H0);
      // Replicate the `lattice_curr` into `lattice_draft`
      std::copy(&lattice_curr[0][0][0][0],
      &lattice_curr[0][0][0][0] + BLOCK,
      &lattice_draft[0][0][0][0]);
      // Check consistency
      sanityTest1();
      sanityTest2();
      sanityTest4();
  }
}
