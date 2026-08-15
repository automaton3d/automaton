/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include <GUI.h>
#include <vector>
#include <algorithm>
#include <random>

#include "model/simulation.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  extern unsigned CENTER;
  extern std::vector<Cell> lattice_curr;
  extern std::vector<Cell> lattice_mirror;

  /**
   * Used in color processing.
   */
  bool isColorNeutral(unsigned char c1, unsigned char c2)
  {
    unsigned res = (c1 ^ c2) & 7;
    return ((res == 0 || res == 7) && c1 && c2 && c1 != 7 && c2 != 7);
  }

  /**
   * Shifts lattice_mirror slices along w-dimension with periodic boundary conditions
   */
  void shiftMirror()
  {
    // Temporary storage for one W-slice
    std::vector<Cell> temp(static_cast<size_t>(EL) * EL * EL);
    // Save the last W-slice
    for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
          temp[(x * EL + y) * EL + z] = getCell(lattice_mirror, x, y, z, W_USED - 1);
    // Shift all slices forward
    for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
          for (unsigned w = W_USED - 1; w > 0; --w)
            getCell(lattice_mirror, x, y, z, w) = getCell(lattice_mirror, x, y, z, w - 1);
    // Wrap saved slice to position 0
    for (unsigned x = 0; x < EL; ++x)
      for (unsigned y = 0; y < EL; ++y)
        for (unsigned z = 0; z < EL; ++z)
          getCell(lattice_mirror, x, y, z, 0) = temp[(x * EL + y) * EL + z];
  }

  /*
   * Tests color neutrality.
   */
  bool neutralColor(Cell &a, Cell &b)
  {
    int color_a = a.ch & 0x07;
    int color_b = b.ch & 0x07;
    return (color_a ^ color_b) == 0x07;
  }

  /*
   * Tests weak neutrality.
   */
  bool neutralWeak(Cell &a, Cell &b)
  {
    int weak_a = (a.ch >> 3) & 0x03;
    int weak_b = (b.ch >> 3) & 0x03;
    return (weak_a ^ weak_b) == 0x03;
  }

  /**
     * Relocate all cells in lattice_curr by offsets (dx, dy, dz).
     * - Wraps around with modulo EL.
     * - Copies dynamic state fields (excluding ch, k, and c[]).
     * - Leaves x[] untouched (fixed coordinates).
     * - Mirror lattice is not touched.
     */
  void relocateGlobal(unsigned dx, unsigned dy, unsigned dz)
  {
    dx %= EL; dy %= EL; dz %= EL;

    // Temporary buffer
    std::vector<Cell> temp(BLOCK);
    std::copy(lattice_curr.begin(), lattice_curr.begin() + BLOCK, temp.begin());

    for (unsigned w = 0; w < W_USED; ++w)
    {
      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
            const Cell& src = getCell(lattice_curr, x, y, z, w);

            unsigned nx = (x + dx) % EL;
            unsigned ny = (y + dy) % EL;
            unsigned nz = (z + dz) % EL;

            Cell& dst = getCell(temp, nx, ny, nz, w);

            // Copy only the relevant properties
            dst.pB   = src.pB;
            dst.sB   = src.sB;
            dst.a    = src.a;
            dst.phiB = src.phiB;
            dst.t    = src.t;
            dst.f    = src.f;
            dst.r2   = src.r2;
            dst.s2B  = src.s2B;
            dst.kB   = src.kB;
            dst.bB   = src.bB;
            dst.hB   = src.hB;
            dst.cB   = src.cB;

            dst.kind       = src.kind;
            dst.parent     = src.parent;
            dst.spin_target= src.spin_target;
            dst.pair_idx   = src.pair_idx;
            dst.m[0] = src.m[0];
            dst.m[1] = src.m[1];
            dst.m[2] = src.m[2];
            dst.reloc[0] = src.reloc[0];
            dst.reloc[1] = src.reloc[1];
            dst.reloc[2] = src.reloc[2];

            // Leave x[], ch, k, and c[] untouched
          }
        }
      }
    }

    // Commit back
    std::copy(temp.begin(), temp.begin() + BLOCK, lattice_curr.begin());
  }

  /**
   * Prints the main parameters.
   */
  void printParams()
  {
    cout << "L:\t"         << EL        << endl;
    cout << "W:\t"         << W_USED     << endl;
    cout << "DIAG:\t"      << DIAG      << endl;
    cout << "RMAX:\t"      << RMAX      << endl;
    cout << "CONVOL:\t"    << CONVOL    << endl;
    cout << "SLOT1:\t"     << SLOT1     << endl;
    cout << "SLOT2:\t"     << SLOT2     << endl;
    cout << "SLOT3:\t"     << SLOT3     << endl;
    cout << "SLOT4:\t"     << SLOT4     << endl;
    cout << "DIFFUSION:\t" << DIFFUSION << endl;
    cout << "SLOT5:\t"     << SLOT5     << endl;
    cout << "SLOT6:\t"     << SLOT6     << endl;
    cout << "SLOT7:\t"     << SLOT7     << endl;
    cout << "RELOC:\t"     << RELOC     << endl;
    cout << "REISSUE:\t"   << REISSUE   << endl;
  }

  /*
   * Prints the CA state for test.
   */
  void printLattice(int w)
  {
    puts("Case: phiB");
    for (unsigned z = 0; z < EL; z++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned x = 0; x < EL; x++)
        {
          // Reference to the current cell
          Cell& cell = getCell(lattice_curr, x, y, z, w);
          printf("%d ", cell.phiB);
        }
        printf("\n");
      }
      printf("z=%d\n", z);
    }
    printf("\n");
  }

  /**
   * Tests if is ok.
   */
  bool sanityTest()
  {
    /*
    cout << "RMAX\t" << RMAX << endl;
    cout << "CONVOL\t" << CONVOL << endl;
    cout << "SLOT1\t" << SLOT1 << endl;
    cout << "SLOT2\t" << SLOT2 << endl;
    cout << "SLOT3\t" << SLOT3 << endl;
    cout << "SLOT4\t" << SLOT4 << endl;
    cout << "DIFFUSION\t" << DIFFUSION << endl;
    cout << "RELOC\t" << RELOC << endl;
    */
    return (CONVOL < SLOT1 && SLOT1 < SLOT2 && SLOT2 < SLOT3 && SLOT3 < SLOT4 && SLOT4 < DIFFUSION && DIFFUSION < RELOC && RELOC < REISSUE);
  }

  /**
   * Check if all c vectors are null.
   */
  bool sanityTest2()
  {
    for (unsigned w = 0; w < EL; w++)
    {
      for (unsigned z = 0; z < EL; z++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned x = 0; x < EL; x++)
          {
            Cell& cell = getCell(lattice_curr, x, y, z, w);
            if (!ZERO(cell.c))
            {
              return false;
            }
          }
        }
      }
    }
    return true;
  }

  bool sanityTest3()
  {
    for (unsigned w = 0; w < EL; w++)
    {

      Cell& seed = getCell(lattice_curr, 0, 0, 0, w);
      for (unsigned z = 0; z < EL; z++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned x = 0; x < EL; x++)
          {
            Cell& cell = getCell(lattice_curr, x, y, z, w);
            if (seed.c[0] != cell.c[0] || seed.c[1] != cell.c[1] || seed.c[2] != cell.c[2])
            {
              return false;
            }
          }
        }
      }
    }
    return true;
  }

  unsigned int getRandomUnsigned(unsigned int modulus)
  {
    static std::mt19937 rng(std::random_device{}()); // Seed once
    std::uniform_int_distribution<unsigned int> dist(0, modulus - 1);
    return dist(rng);
  }

}
