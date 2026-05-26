/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include <GUI.h>
#include <vector>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <set>
#include <random>
#include <iostream>
#include <cstdlib>

#include "model/simulation.h"
#include "model/geometry.h"

namespace automaton
{
  extern std::vector<Cell> lattice_curr;
  extern std::vector<Cell> lattice_mirror;

  // ============================================================
  // SPHERICAL HELPERS
  // ============================================================

  inline void antipodalWrap(int& x, int& y, int& z)
  {
    const int R = EL / 2;

    int cx = x - R;
    int cy = y - R;
    int cz = z - R;

    int r2 = cx*cx + cy*cy + cz*cz;

    if (r2 > R*R)
    {
      cx = -cx;
      cy = -cy;
      cz = -cz;

      x = cx + R;
      y = cy + R;
      z = cz + R;
    }
  }

  // ============================================================
  // COLOR
  // ============================================================

  bool isColorNeutral(unsigned char c1, unsigned char c2)
  {
    unsigned res = (c1 ^ c2) & 7;

    return ((res == 0 || res == 7) &&
            c1 &&
            c2 &&
            c1 != 7 &&
            c2 != 7);
  }

  bool neutralColor(Cell &a, Cell &b)
  {
    int color_a = a.ch & 0x07;
    int color_b = b.ch & 0x07;

    return (color_a ^ color_b) == 0x07;
  }

  bool neutralWeak(Cell &a, Cell &b)
  {
    int weak_a = (a.ch >> 3) & 0x03;
    int weak_b = (b.ch >> 3) & 0x03;

    return (weak_a ^ weak_b) == 0x03;
  }

  // ============================================================
  // VECTOR MATH
  // ============================================================

  void cross_product(double result[3],
                     const double a[3],
                     const double b[3])
  {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
  }

  void normalize(double vec[3])
  {
    double magnitude =
        sqrt(vec[0]*vec[0] +
             vec[1]*vec[1] +
             vec[2]*vec[2]);

    if (magnitude <= 1e-12)
    {
      vec[0] = 1.0;
      vec[1] = 0.0;
      vec[2] = 0.0;
      return;
    }

    vec[0] /= magnitude;
    vec[1] /= magnitude;
    vec[2] /= magnitude;
  }

  // ============================================================
  // MOMENTUM MARKING
  // ============================================================

  void markPoints(unsigned p[3], int w)
  {
    int x0 = CENTER;
    int y0 = CENTER;
    int z0 = CENTER;

    int x1 = (int)p[0];
    int y1 = (int)p[1];
    int z1 = (int)p[2];

    int dx = x1 - x0;
    int dy = y1 - y0;
    int dz = z1 - z0;

    int steps = std::max({ abs(dx), abs(dy), abs(dz) });

    if (steps == 0)
      return;

    for (int i = 0; i <= steps; ++i)
    {
      double t = (double)i / (double)steps;

      int x = (int)round(x0 + dx*t);
      int y = (int)round(y0 + dy*t);
      int z = (int)round(z0 + dz*t);

      antipodalWrap(x, y, z);

      if (!isInsideSphere(x,y,z))
        continue;

      getCell(lattice_curr, x, y, z, w).pB = true;
    }
  }

  // ============================================================
  // SHELL GENERATION
  // ============================================================

  void add_symmetric_points(
      std::set<std::tuple<int,int,int>>& points,
      int x, int y, int z,
      double C)
  {
    int ix_p = (int)std::round( x + C);
    int ix_n = (int)std::round(-x + C);

    int iy_p = (int)std::round( y + C);
    int iy_n = (int)std::round(-y + C);

    int iz_p = (int)std::round( z + C);
    int iz_n = (int)std::round(-z + C);

    points.emplace(ix_p, iy_p, iz_p);
    points.emplace(ix_p, iy_p, iz_n);
    points.emplace(ix_p, iy_n, iz_p);
    points.emplace(ix_p, iy_n, iz_n);

    points.emplace(ix_n, iy_p, iz_p);
    points.emplace(ix_n, iy_p, iz_n);
    points.emplace(ix_n, iy_n, iz_p);
    points.emplace(ix_n, iy_n, iz_n);
  }

  std::vector<std::tuple<int,int,int>>
  generateShell(int EL_local)
  {
    std::set<std::tuple<int,int,int>> unique_points;

    const int R = EL_local / 2;

    for (int x = 0; x < EL_local; ++x)
    {
      for (int y = 0; y < EL_local; ++y)
      {
        for (int z = 0; z < EL_local; ++z)
        {
          int cx = x - R;
          int cy = y - R;
          int cz = z - R;

          double r = sqrt(
              (double)(
                  cx*cx +
                  cy*cy +
                  cz*cz));

          if (fabs(r - R) < 0.75)
          {
            unique_points.emplace(x,y,z);
          }
        }
      }
    }

    return std::vector<std::tuple<int,int,int>>(
        unique_points.begin(),
        unique_points.end());
  }

  // ============================================================
  // MIRROR SHIFT
  // ============================================================

  void shiftMirror()
  {
    std::vector<Cell> temp(BLOCK);

    for (unsigned w = 0; w < W_USED; ++w)
    {
      unsigned nw = (w + 1) % W_USED;

      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
            getCell(temp, x,y,z,nw) =
                getCell(lattice_mirror, x,y,z,w);
          }
        }
      }
    }

    lattice_mirror.swap(temp);
  }

  // ============================================================
  // GLOBAL RELOCATION
  // ============================================================

  void relocateGlobal(unsigned dx,
                      unsigned dy,
                      unsigned dz)
  {
    std::vector<Cell> temp = lattice_curr;

    for (unsigned w = 0; w < W_USED; ++w)
    {
      for (unsigned x = 0; x < EL; ++x)
      {
        for (unsigned y = 0; y < EL; ++y)
        {
          for (unsigned z = 0; z < EL; ++z)
          {
            const Cell& src =
                getCell(lattice_curr, x,y,z,w);

            int nx = (int)x + (int)dx;
            int ny = (int)y + (int)dy;
            int nz = (int)z + (int)dz;

            antipodalWrap(nx, ny, nz);

            if (!isInsideSphere(nx,ny,nz))
              continue;

            Cell& dst =
                getCell(temp, nx,ny,nz,w);

            dst.pB   = src.pB;
            dst.sB   = src.sB;
            dst.a    = src.a;
            dst.phiB = src.phiB;
            dst.t    = src.t;
            dst.f    = src.f;
            dst.d    = src.d;
            dst.s2B  = src.s2B;
            dst.kB   = src.kB;
            dst.bB   = src.bB;
            dst.hB   = src.hB;
            dst.cB   = src.cB;
          }
        }
      }
    }

    lattice_curr.swap(temp);
  }

  // ============================================================
  // DEBUG
  // ============================================================

  void printParams()
  {
    std::cout << "L:\t"         << EL        << std::endl;
    std::cout << "W:\t"         << W_USED    << std::endl;
    std::cout << "DIAG:\t"      << DIAG      << std::endl;
    std::cout << "RMAX:\t"      << RMAX      << std::endl;
    std::cout << "CONVOL:\t"    << CONVOL    << std::endl;
    std::cout << "SLOT1:\t"     << SLOT1     << std::endl;
    std::cout << "SLOT2:\t"     << SLOT2     << std::endl;
    std::cout << "SLOT3:\t"     << SLOT3     << std::endl;
    std::cout << "SLOT4:\t"     << SLOT4     << std::endl;
    std::cout << "DIFFUSION:\t" << DIFFUSION << std::endl;
    std::cout << "SLOT5:\t"     << SLOT5     << std::endl;
    std::cout << "SLOT6:\t"     << SLOT6     << std::endl;
    std::cout << "SLOT7:\t"     << SLOT7     << std::endl;
    std::cout << "RELOC:\t"     << RELOC     << std::endl;
    std::cout << "REISSUE:\t"   << REISSUE   << std::endl;
  }

  void printLattice(int w)
  {
    puts("Case: phiB");

    for (unsigned z = 0; z < EL; z++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned x = 0; x < EL; x++)
        {
          Cell& cell =
              getCell(lattice_curr, x,y,z,w);

          printf("%d ", cell.phiB);
        }

        printf("\n");
      }

      printf("z=%d\n", z);
    }

    printf("\n");
  }

  bool sanityTest()
  {
    return (
      CONVOL < SLOT1 &&
      SLOT1 < SLOT2 &&
      SLOT2 < SLOT3 &&
      SLOT3 < SLOT4 &&
      SLOT4 < DIFFUSION &&
      DIFFUSION < RELOC &&
      RELOC < REISSUE
    );
  }

  bool sanityTest2()
  {
    for (unsigned w = 0; w < W_USED; w++)
    {
      for (unsigned z = 0; z < EL; z++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned x = 0; x < EL; x++)
          {
            Cell& cell =
                getCell(lattice_curr, x,y,z,w);

            if (!ZERO(cell.c))
              return false;
          }
        }
      }
    }

    return true;
  }

  bool sanityTest3()
  {
    for (unsigned w = 0; w < W_USED; w++)
    {
      Cell& seed =
          getCell(lattice_curr, 0,0,0,w);

      for (unsigned z = 0; z < EL; z++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned x = 0; x < EL; x++)
          {
            Cell& cell =
                getCell(lattice_curr, x,y,z,w);

            if (seed.c[0] != cell.c[0] ||
                seed.c[1] != cell.c[1] ||
                seed.c[2] != cell.c[2])
            {
              return false;
            }
          }
        }
      }
    }

    return true;
  }

  unsigned getRandomUnsigned(unsigned modulus)
  {
    if (modulus <= 1)
      return 0;

    unsigned r;

    do
    {
      r = rand() % modulus;
    }
    while (r == 0);

    return r;
  }

} // namespace automaton