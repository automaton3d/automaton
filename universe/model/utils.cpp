/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "../GUIrenderer.h"
#include "simulation.h"

namespace automaton
{
  extern const unsigned LIGHT;

  extern bool reloc_x[W_DIM], reloc_y[W_DIM], reloc_z[W_DIM];
  extern bool reloc_w;
  extern Cell lattice_curr[EL][EL][EL][W_DIM];
  extern Cell lattice_draft[EL][EL][EL][W_DIM];
  COLORREF *voxels;

  /**
   * This is a bridge between the model and the graphical framework.
   */
  void updateBuffer()
  {
    // Check if the framework is active and if there are any checkboxes
    if (!framework::active || framework::checkboxes.empty())
      return;
    // Check if data is not transient

    /* TODO
    if (lattice_curr[0][0][0][0].k < UPDATE)
      return;
      */
    int w = framework::list.getSelected();
    // If no layer is selected, exit the function
    if (w == -1)
      return;
    // Iterate over the 3D grid to update the voxel data
    unsigned index3D = 0;
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
        	/*
          Cell &cell = lattice_curr[x][y][z][w];
          // Check if 'wv' is a valid property and update the voxel color
          if (cell.d > cell.t && cell.d <= cell.t + LIGHT)
//          if (cell.wv)
          {
            voxels[index3D] = RGB(255, 255, 255);  // White voxel
            if (cell.t < 3 * LIGHT)
              voxels[index3D] = RGB(255, 0, 255);//DEBUG
          }
          else
            voxels[index3D] = RGB(0, 0, 0);        // Black voxel
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

  /*
   * Prints the CA state for test.
   */
  void printLattice()
  {
      for (unsigned w = 0; w < W_DIM; w++)
      {
          Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
          // Print the 'pole' value of the current cell
          printf("%d  ", cell.pB);
      }
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
      if (magnitude > 0.0)
      {
          vec[0] /= magnitude;
          vec[1] /= magnitude;
          vec[2] /= magnitude;
      }
  }

  /**
   * Marks the poles of a given momentum.
   * Uses the Bresenham line algorithm.
   * (used during initialization)
   *
   * @p the current ray
   * @w the shell in the w dimension
   */
  void markPoles(unsigned p[3], int w)
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
          lattice_curr[x][y][z][w].pB = true;
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
          lattice_curr[x][y][z][w].pB = true;
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
          lattice_curr[x][y][z][w].pB = true;
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
   * Circular Shift in the Fourth Dimension W
   */
  void shiftW()
  {
    // Iterate through each 3D grid position
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          // Save the last element in the 4th dimension for wrapping
          Cell temp = lattice_draft[x][y][z][W_DIM - 1];
          // Shift elements in the 4th dimension "up"
          for (unsigned w = W_DIM - 1; w > 0; w--)
          {
            lattice_draft[x][y][z][w] = lattice_draft[x][y][z][w - 1];
          }
          // Place the saved last element at the start of the 4th dimension
          lattice_draft[x][y][z][0] = temp;
        }
      }
    }
  }

  /**
   * Circular shift in the X direction.
   */
  void shiftX(unsigned w)
  {
    // Reset relocation flag
    reloc_x[w] = false;
    // Temporary storage for the rightmost column of each xz plane
    Cell temp[EL][EL];
    // Copy the rightmost column to the temporary storage
    for (unsigned int y = 0; y < EL; ++y)
    {
      for (unsigned int z = 0; z < EL; ++z)
      {
        temp[y][z] = lattice_draft[EL - 1][y][z][w];
      }
    }
    // Shift all other columns one position to the right
    for (unsigned int x = EL - 1; x > 0; --x)
    {
      for (unsigned int y = 0; y < EL; ++y)
      {
        for (unsigned int z = 0; z < EL; ++z)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[((x + EL) - 1) % EL][y][z][w];
          if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
          {
        	draft.a = w;
            draft.t = 0;
          }
          else
          {
          	draft.a = W_DIM;
          }
        }
      }
    }
    // Copy the stored rightmost column to the leftmost column
    for (unsigned int y = 0; y < EL; ++y)
    {
      for (unsigned int z = 0; z < EL; ++z)
      {
        Cell &draft = lattice_draft[0][y][z][w];
        draft = temp[y][z];
        if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
        {
      	  draft.a = w;
      	  draft.t = 0;
        }
        else
        {
          draft.a = W_DIM;
        }
      }
    }
  }

  /**
   * Circular shift in the Y direction.
   */
  void shiftY(unsigned w)
  {
    // Reset relocation flag
    reloc_y[w] = false;
    // Temporary storage for the topmost row of each xz plane
    Cell temp[EL][EL];
    // Copy the topmost row to the temporary storage
    for (unsigned int x = 0; x < EL; ++x)
    {
      for (unsigned int z = 0; z < EL; ++z)
      {
        temp[x][z] = lattice_draft[x][EL - 1][z][w];
      }
    }
    // Shift all other rows one position upwards
    for (unsigned int y = EL - 1; y > 0; --y)
    {
      for (unsigned int x = 0; x < EL; ++x)
      {
        for (unsigned int z = 0; z < EL; ++z)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[x][((y + EL) - 1) % EL][z][w];
          if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
          {
        	draft.a = w;
            draft.t = 0;
          }
          else
          {
          	draft.a = W_DIM;
          }
        }
      }
    }
    // Copy the stored topmost row to the bottommost row
    for (unsigned int x = 0; x < EL; ++x)
    {
      for (unsigned int z = 0; z < EL; ++z)
      {
        Cell &draft = lattice_draft[x][0][z][w];
        draft = temp[x][z];
        if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
        {
      	  draft.a = w;
      	  draft.t = 0;
        }
        else
        {
          draft.a = W_DIM;
        }
      }
    }
  }

  /**
   * Circular shift in the Z direction.
   */
  void shiftZ(unsigned w)
  {
    // Reset relocation flag
    reloc_z[w] = false;
    // Temporary storage for the frontmost slice of each xy plane
    Cell temp[EL][EL];
    // Copy the frontmost slice to the temporary storage
    for (unsigned int x = 0; x < EL; ++x)
    {
      for (unsigned int y = 0; y < EL; ++y)
      {
        temp[x][y] = lattice_draft[x][y][EL - 1][w];
      }
    }
    // Shift all other slices one position towards the front
    for (unsigned int z = EL - 1; z > 0; --z)
    {
      for (unsigned int x = 0; x < EL; ++x)
      {
        for (unsigned int y = 0; y < EL; ++y)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[x][y][((z + EL) - 1) % EL][w];
          if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
          {
        	draft.a = w;
            draft.t = 0;
          }
          else
          {
          	draft.a = W_DIM;
          }
        }
      }
    }
    // Copy the stored frontmost slice to the backmost slice
    for (unsigned int x = 0; x < EL; ++x)
    {
      for (unsigned int y = 0; y < EL; ++y)
      {
        Cell &draft = lattice_draft[x][y][0][w];
        draft = temp[x][y];
        if (draft.x[0] == CENTER && draft.x[1] == CENTER && draft.x[2] == CENTER)
        {
      	  draft.a = w;
      	  draft.t = 0;
        }
        else
        {
          draft.a = W_DIM;
        }
      }
    }
  }

  // Function to generate uniformly distributed points on the sphere's surface
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

}
