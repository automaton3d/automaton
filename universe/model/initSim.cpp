/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "simulation.h"
#include <cstring>
#include <iostream>
#include <cmath>
#include <set>
#include <tuple>

namespace automaton
{
  using namespace std;

  const double epsilon = 1e-9;

  extern Cell *lattice_current, *lattice_draft, *lattice_mirror;

  /*
   * Generate spin vectors.
   */
  void initSpin()
  {
    // Maximum radius of the greatest sphere
    int r_max = CENTER;
    // Calculate number of steps based on L
    // (let's take L/2 as a reference for the steps)
    int n_steps = CENTER;
    // Interval step size (difference between consecutive steps)
    double step_size = 1.0 / n_steps;
    Cell *ptr = lattice_current + CENTER * SIDE2 + CENTER * SIDE + CENTER;
    for (unsigned w = 0;;)
    {
      for (int x = 0; x < SIDE; x++)
      {
        for (int y = 0; y < SIDE; y++)
        {
          for (int z = 0; z < SIDE; z++)
          {
            // Compute the squared distance from the center
            int dx = x - CENTER;
            int dy = y - CENTER;
            int dz = z - CENTER;
            int distance_squared = dx * dx + dy * dy + dz * dz;
            // Check for distance lying between r_max-1 and r_max
            for (int i = 0; i < n_steps; i++)
            {
              // Define the distance at step i (d) and check if the point
              // is within the range [d^2, (d + step_size)^2]
              double d = r_max - 1 + i * step_size;
              double next_d = r_max - 1 + (i + 1) * step_size;
              if (distance_squared >= d * d && distance_squared < next_d * next_d)
              {
                ptr->s[0] = x;
                ptr->s[1] = y;
                ptr->s[2] = z;
                w++;
                ptr += SIDE3;
                if (w == W_DIM)
                  return;
              }
            }
          }
        }
      }
      if (w == 0)
        break;
    }
    // Catastrophic exit
    assert(0);
  }

  /*
   * Generates momentum vectors from spin.
   */
  void initMomentum()
  {
    Cell *ptr = lattice_current + CENTER * SIDE2 +
      CENTER * SIDE + CENTER;
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
          // Handle special case where s is aligned with the z-axis
          v[0] = 0.0;
          v[1] = 1.0;
          v[2] = 0.0;
      }
      else
      {
          // Default spin vector
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
      // Map normalized vector to unsigned integer range
      unsigned p[3];
      p[0] = (unsigned)((p2[0] + 1.0) * FCENTER);
      p[1] = (unsigned)((p2[1] + 1.0) * FCENTER);
      p[2] = (unsigned)((p2[2] + 1.0) * FCENTER);
      // Mark the poles in the layer
      markPoles(p, w);
      // Ensure the values are in the expected range
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
   Cell *ptr = lattice_current + c2;
   // Calculate the central index for all layers in the w dimension
   for (unsigned w = 0; w < W_DIM; w++, ptr += SIDE3)
   {
     char w0 = w % 2;
     char w1 = (w >> 1) % 2;
     char q = w0 ^ w1;
     ptr->charge = (w % 8) | (w0 << 3) | (1 << 4) | (q << 5);
     ptr->aff = w + 1;     // avoid zero
   }
   puts("initCharges ok.");
 }

 void initDebug()
 {
   Cell *cell = lattice_current + CENTER * SIDE2 + CENTER * SIDE + CENTER;
   cell->c[0] = SIDE/4;
   cell->c[1] = 0;
   cell->c[2] = 0;
   cell->reloc = 1;
   cell->k = 0;
   puts("initDebug ok.");
 }

  /**
   * Function to initialize the lattice.
   */
  void initGeneral()
  {
    // Initialize other properties
    Cell *ptr = lattice_current;
    for (unsigned index = 0; index < BLOCK; index++, ptr++)
    {
      // x, y, z here are spatial coordinates
      int x = (index / SIDE2) % SIDE;
      int y = (index / SIDE) % SIDE;
      int z = index % SIDE;
      // z is the layer coordinate
      int w = index / SIDE3;printf("w=%d\n", w);
      Cell *cell0 = lattice_current + SIDE3 * w;
      ptr->charge = cell0->charge;
      // Spread spin
      ptr->s[0] = cell0->s[0];
      ptr->s[1] = cell0->s[1];
      ptr->s[2] = cell0->s[2];
      // Assign cell coordinates
      ptr->pos[0] = x;
      ptr->pos[1] = y;
      ptr->pos[2] = z;
      // Calculate distances relative to the center
      double dx = (double)(x - FCENTER);
      double dy = (double)(y - FCENTER);
      double dz = (double)(z - FCENTER);
      double d = sqrt(dx * dx + dy * dy + dz * dz);
      // Initialize cell properties
      ptr->d = (unsigned)(LIGHT * d);
      // Define frequency and assertion
      ptr->freq = 1;
      assert(ptr->freq < FMAX);
      // Calculate 'sin' value based on the distance
      double sin_scaled = SIDE * fabs(sin(2 * M_PI * d / SIDE));
      assert(sin_scaled >= 0 && sin_scaled <= UINT_MAX);
      ptr->sin = (unsigned)sin_scaled;

      ptr->ctrl = 1; // DEBUG
    }
    puts("initGeneral ok.");
  }

  void sanityTest()
  {
    unsigned ns = 0;
    Cell *ptr = lattice_current;
    for (unsigned i = 0; i < BLOCK; i++, ptr++)
    {
      if (ptr->s[0] != CENTER || ptr->s[1] != CENTER || ptr->s[2] != CENTER)
        ns++;
    }
    if (ns == W_DIM)
      puts("sanity ok.");
    else
      printf("sanity failed: s=%d\n", ns);
  }

  /**
   * Hub for initialization routines.
   */
  void initSimulation()
  {
    // Allocate memory
    voxels = (COLORREF *)malloc(SIDE3 * sizeof(COLORREF));
    lattice_current   = (Cell *)malloc(BLOCK * sizeof(Cell));
    lattice_draft  = (Cell *)malloc(BLOCK * sizeof(Cell));
    lattice_mirror = (Cell *)malloc(BLOCK * sizeof(Cell));
    assert(lattice_current != nullptr && lattice_draft != nullptr &&
           lattice_mirror != nullptr);
    // Clean the lattice
    memset(lattice_current, 0, BLOCK * sizeof(Cell));
    // Initialize data
    initCharges();
    initSpin();
    initMomentum();
    initGeneral();
    initDebug();
    sanityTest();
    // Replicate main in draft
    memcpy(lattice_draft, lattice_current, BLOCK * sizeof(Cell));
    fflush(stdout);
  }

}
