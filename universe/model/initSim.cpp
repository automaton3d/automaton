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
  extern vector<Cell> lattice_current;
  extern vector<Cell> lattice_draft;
  extern EntropyCalculator entropyCalc;

  /*
   * Generate spin vectors.
   */
  void initSpin()
  {
    int r_max = CENTER;
    int n_steps = CENTER;
    double step_size = 1.0 / n_steps;

    // Get the center cell in the lattice
    Cell& ptr = lattice_current[CENTER * SIDE2 + CENTER * SIDE + CENTER];

    unsigned w = 0; // Counter for number of spins initialized
    for (int x = 0; x < SIDE; x++)
    {
      for (int y = 0; y < SIDE; y++)
      {
        for (int z = 0; z < SIDE; z++)
        {
          int dx = x - CENTER;
          int dy = y - CENTER;
          int dz = z - CENTER;
          int distance_squared = dx * dx + dy * dy + dz * dz;

          for (int i = 0; i < n_steps; i++)
          {
            double d = r_max - 1 + i * step_size;
            double next_d = r_max - 1 + (i + 1) * step_size;

            if (distance_squared >= d * d && distance_squared < next_d * next_d)
            {
              ptr.s[0] = x;
              ptr.s[1] = y;
              ptr.s[2] = z;
              w++;

              ptr = lattice_current[ptr.pos[0] * SIDE2 + ptr.pos[1] * SIDE + ptr.pos[2]];

              if (w == W_DIM)
              {
                return;
              }
            }
          }
        }
      }
    }

    // Handle error if no spins initialized
    if (w == 0)
    {
      cerr << "Error: No spins initialized!" << endl;
      exit(EXIT_FAILURE);  // Better error handling than assert(0)
    }
  }

  /*
   * Generates momentum vectors from spin.
   */
  void initMomentum()
  {
    Cell* ptr = &lattice_current[CENTER * SIDE2 + CENTER * SIDE + CENTER];
    for (unsigned w = 0; w < W_DIM; w++, ptr += SIDE3)
    {
      double dx = ptr->s[0] - FCENTER;
      double dy = ptr->s[1] - FCENTER;
      double dz = ptr->s[2] - FCENTER;
      double s[3] = { dx, dy, dz };
      normalize(s);
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

      double p1[3];
      cross_product(p1, s, v);
      normalize(p1);
      double p2[3];
      cross_product(p2, s, p1);
      normalize(p2);

      unsigned p[3];
      p[0] = (unsigned)((p2[0] + 1.0) * FCENTER);
      p[1] = (unsigned)((p2[1] + 1.0) * FCENTER);
      p[2] = (unsigned)((p2[2] + 1.0) * FCENTER);

      markPoles(p, w);

      assert(p[0] < SIDE && p[1] < SIDE && p[2] < SIDE);
    }
    puts("\tinitMomentum ok.");
  }

  /*
   * Initializes charges and affinity in the central
   * cell of every layer.
   */
  void initCharges()
  {
    int c2 = CENTER * SIDE2 + CENTER * SIDE + CENTER;
    Cell* ptr = &lattice_current[c2];
    for (unsigned w = 0; w < W_DIM; w++, ptr += SIDE3)
    {
      char w0 = w % 2;
      char w1 = (w >> 1) % 2;
      char q = w0 ^ w1;
      ptr->charge = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
      ptr->aff = w + 1;  // avoid zero
    }
    puts("initCharges ok.");
  }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
    Cell* ptr = &lattice_current[0];
    for (unsigned index = 0; index < BLOCK; index++, ptr++)
    {
      int x = (index / SIDE2) % SIDE;
      int y = (index / SIDE) % SIDE;
      int z = index % SIDE;
      int w = index / SIDE3;

      Cell* cell0 = &lattice_current[SIDE3 * w];
      ptr->charge = cell0->charge;
      ptr->s[0] = cell0->s[0];
      ptr->s[1] = cell0->s[1];
      ptr->s[2] = cell0->s[2];
      ptr->pos[0] = x;
      ptr->pos[1] = y;
      ptr->pos[2] = z;

      double dx = (double)(x - FCENTER);
      double dy = (double)(y - FCENTER);
      double dz = (double)(z - FCENTER);
      double d = sqrt(dx * dx + dy * dy + dz * dz);
      ptr->d = (unsigned)(LIGHT * d);
      ptr->freq = 1;

      assert(ptr->freq < FMAX);

      double sin_scaled = SIDE * fabs(sin(2 * M_PI * d / SIDE));
      assert(sin_scaled >= 0 && sin_scaled <= UINT_MAX);

      ptr->sin = (unsigned)sin_scaled;
      ptr->ctrl = 1; // Start in the Update step
    }
    puts("initGeneral ok.");
  }

  void sanityTest()
  {
    printf("%u + 1 + %u + 3*%u + 3*%u + %u = %u == %u\n", CONVOL, SIDE, 3 * CENTER, 3 * SIDE, LIGHT, CONVOL + 1 + SIDE + 3 * CENTER + 3 * SIDE + LIGHT, FRAME);
    unsigned ns = 0;
    Cell* ptr = &lattice_current[0];
    for (unsigned i = 0; i < BLOCK; i++, ptr++)
    {
      if (ptr->s[0] != CENTER || ptr->s[1] != CENTER || ptr->s[2] != CENTER)
        ns++;
    }
    if (ns == BLOCK)
      puts("sanity ok.");
    else
      printf("sanity failed: s=%d != %u\n", ns, W_DIM);
  }

  void initSimulation()
  {
    initCharges();
    initSpin();
    initMomentum();
    initGeneral();
    initPoincare();
    sanityTest();
	saveState0();
	checkPoincare();
	// Calculate the initial entropy of the universe
	entropyCalc.collectData();
	H0 = entropyCalc.computeEntropy();
    // Replicate main in draft
    lattice_draft = lattice_current;
  }
}
