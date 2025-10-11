/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "../GUIrenderer.h"
#include "simulation.h"

namespace automaton
{
  extern Cell lattice_curr[EL][EL][EL][W_DIM];
  extern Cell lattice_mirror[EL][EL][EL][W_DIM];
  COLORREF *voxels;

  /**
   * This is a bridge between the model and the graphical framework.
   */
  void updateBuffer()
  {
    int w = framework::list.getSelected();
    // Iterate over the 3D grid to update the voxel data
    unsigned index3D = 0;
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          Cell &cell = lattice_curr[x][y][z][w];
          if (cell.t == cell.d)
          {
            if (cell.a == W_DIM)
            {
              voxels[index3D] = RGB(255, 0, 0); // Red (orphan)
            }
            else
            {
              voxels[index3D] = RGB(255, 255, 255); // White (wavefront match)
            }
          }
          else if (cell.cB)
          {
            voxels[index3D] = RGB(0, 0, 255); // Blue (contraction/reissue)
          }
          else
          {
            voxels[index3D] = RGB(0, 0, 0); // Black
          }
          // DEBUG
          /*
          if (cell.a == W_DIM && cell.t != cell.d)
          {
            voxels[index3D] = RGB(255, 255, 0); //
          }
          */
          index3D++;
        }
      }
    }
  }

  /**
   * Used in color processing.
   */
  bool isColorNeutral(unsigned char c1, unsigned char c2)
  {
    unsigned res = (c1 ^ c2) & 7;
    return ((res == 0 || res == 7) && c1 && c2 && c1 != 7 && c2 != 7);
  }

  void cross_product(double result[3], const double a[3], const double b[3])
  {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
  }

  void normalize(double vec[3])
  {
    double magnitude = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
    // Avoid division by zero
    if (magnitude <= 1e-12)
    {
      vec[0] = 1.0; vec[1] = 0.0; vec[2] = 0.0; // Vetor padrão
    }
    else if (magnitude > 0.0)
    {
      vec[0] /= magnitude;
      vec[1] /= magnitude;
      vec[2] /= magnitude;
    }
  }

  /**
   * Marks the momentum points.
   * Uses the Bresenham line algorithm.
   * (used during initialization)
   *
   * @p the current ray
   * @w the shell in the w dimension
   */
  void markPoints(unsigned p[3], int w)
  {
    // Calculate the center of the lattice
    int cx = CENTER, cy = CENTER, cz = CENTER;
    // Calculate the deltas
    int dx = p[0] - cx;
    int dy = p[1] - cy;
    int dz = p[2] - cz;
    // Determine the step direction
    int sx = (dx > 0) ? 1 : -1;
    int sy = (dy > 0) ? 1 : -1;
    int sz = (dz > 0) ? 1 : -1;
    dx = abs(dx);
    dy = abs(dy);
    dz = abs(dz);
    // Determine the dominant axis
    int ax = 2 * dx;
    int ay = 2 * dy;
    int az = 2 * dz;
    int x = cx, y = cy, z = cz;
    if (dx >= dy && dx >= dz)
    {
      // X is dominant
      int yd = ay - dx;
      int zd = az - dx;
      for (int i = 0; i <= dx; i++)
      {
        lattice_curr[x][y][z][w].pB = true; //printf("\t%d,%d,%d:%d\n", x, y, z, w);
        x += sx;
        if (yd >= 0)
        {
          y += sy;
          yd -= ax;
        }
        if (zd >= 0)
        {
          z += sz;
          zd -= ax;
        }
        yd += ay;
        zd += az;
      }
    }
    else if (dy >= dx && dy >= dz)
    {
      // Y is dominant
      int xd = ax - dy;
      int zd = az - dy;
      for (int i = 0; i <= dy; i++)
      {
        lattice_curr[x][y][z][w].pB = true; //printf("\t%d,%d,%d:%d\n", x, y, z, w);
        y += sy;
        if (xd >= 0)
        {
          x += sx;
          xd -= ay;
        }
        if (zd >= 0)
        {
          z += sz;
          zd -= ay;
        }
        xd += ax;
        zd += az;
      }
    }
    else
    {
      // Z is dominant
      int xd = ax - dz;
      int yd = ay - dz;
      for (int i = 0; i <= dz; i++)
      {
        lattice_curr[x][y][z][w].pB = true; //printf("\t%d,%d,%d:%d\n", x, y, z, w);
        z += sz;
        if (xd >= 0)
        {
          x += sx;
          xd -= az;
        }
        if (yd >= 0)
        {
          y += sy;
          yd -= az;
        }
        xd += ax;
        yd += ay;
      }
    }
  }

  /*
   * Function to generate uniformly distributed points on the sphere's surface
   */
  std::vector<std::tuple<int, int, int>> generateUniformSpherePoints(int R, int u, int SIDE)
  {
    std::vector<std::tuple<int, int, int>> points;
    int offset = SIDE / 2; // Offset to shift coordinates into the range [0, L-1]
    for (int i = 0; i < u; ++i)
    {
      double phi = acos(1 - 2.0 * (i + 0.5) / u); // Polar angle
      double theta = M_PI * (1 + std::sqrt(5)) * i; // Azimuthal angle
      // Convert to Cartesian coordinates
      double x = R * sin(phi) * cos(theta);
      double y = R * sin(phi) * sin(theta);
      double z = R * cos(phi);
      // Round to integers and apply offset
      points.emplace_back(
        static_cast<int>(round(x)) + offset,
        static_cast<int>(round(y)) + offset,
        static_cast<int>(round(z)) + offset
      );
    }
    // Remove duplicates caused by rounding
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
  }

  /**
   * Shifts lattice_mirror slices along w-dimension with periodic boundary conditions
   */
  void shiftMirror()
  {
    // Temporary storage for the last slice in W (to wrap around)
    Cell temp[EL][EL][EL]= {};
    // Save the last W-slice
    for (int x = 0; x < EL; ++x)
      for (int y = 0; y < EL; ++y)
        for (int z = 0; z < EL; ++z)
          temp[x][y][z] = lattice_mirror[x][y][z][W_DIM - 1];
    // Shift all slices forward
    for (int x = 0; x < EL; ++x)
      for (int y = 0; y < EL; ++y)
        for (int z = 0; z < EL; ++z)
          for (int w = W_DIM - 1; w > 0; --w)
            lattice_mirror[x][y][z][w] = lattice_mirror[x][y][z][w - 1];
    // Wrap the saved slice into position 0
    for (int x = 0; x < EL; ++x)
      for (int y = 0; y < EL; ++y)
        for (int z = 0; z < EL; ++z)
            lattice_mirror[x][y][z][0] = temp[x][y][z];
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
   * Prints the main parameters.
   */
  void printConstants()
  {
    cout << "DIAG:\t"      << DIAG      << endl;
    cout << "RMAX:\t"      << RMAX      << endl;
    cout << "CONVOL:\t"    << CONVOL    << endl;
    cout << "DIFFUSION:\t" << DIFFUSION << endl;
    cout << "RELOC:\t"     << RELOC     << endl;
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
          Cell& cell = lattice_curr[x][y][z][w];
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
    cout << "RMAX\t" << RMAX << endl;
    cout << "CONVOL\t" << CONVOL << endl;
	cout << "SLOT1\t" << SLOT1 << endl;
	cout << "SLOT2t" << SLOT2 << endl;
	cout << "SLOT3\t" << SLOT3 << endl;
	cout << "SLOT4\t" << SLOT4 << endl;
	cout << "DIFFUSION\t" << DIFFUSION << endl;
	cout << "RELOC\t" << RELOC << endl;
    return (CONVOL < SLOT1 && SLOT1 < SLOT2 && SLOT2 < SLOT3 && SLOT3 < SLOT4 && SLOT4 < DIFFUSION && DIFFUSION < RELOC);
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
            Cell& cell = lattice_curr[x][y][z][w];
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

      Cell& seed = lattice_curr[0][0][0][w];
      for (unsigned z = 0; z < EL; z++)
      {
        for (unsigned y = 0; y < EL; y++)
        {
          for (unsigned x = 0; x < EL; x++)
          {
            Cell& cell = lattice_curr[x][y][z][w];
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

}
