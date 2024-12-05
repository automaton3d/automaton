/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "simulation.h"
#include "../mygl.h"

namespace automaton
{
  extern Cell lattice_curr[SIDE][SIDE][SIDE][W_DIM];
  extern Cell lattice_draft[SIDE][SIDE][SIDE][W_DIM];
  COLORREF *voxels;

  /**
   * This is a bridge between the model and the graphical framework.
   */
  void updateBuffer()
  {
      // Check if the framework is active and if there are any checkboxes
      if (!framework::active || framework::checkboxes.empty())
          return;
      // Find the selected layer
      int w = -1; // Initialize to -1 to indicate no layer selected
      for (unsigned i = 0; i < W_DIM; i++)
      {
          if (framework::layers[i].isSelected())
          {
              w = i; // Set the selected layer
              break; // Exit loop once the layer is found
          }
      }
      // If no layer is selected, exit the function
      if (w == -1)
          return;
      // Iterate over the 3D grid to update the voxel data
      unsigned index3D = 0;
      for (int x = 0; x < SIDE; x++)
      {
          for (int y = 0; y < SIDE; y++)
          {
              for (int z = 0; z < SIDE; z++)
              {
                Cell &cell = lattice_curr[x][y][z][w];
                  // Check if 'wv' is a valid property and update the voxel color
                  if (cell.wv)  // Corrected to access the correct cell
                      voxels[index3D] = RGB(255, 255, 255);  // White voxel
                  else
                      voxels[index3D] = RGB(0, 0, 0);        // Black voxel
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
          printf("%d  ", cell.pole);  // Assuming 'pole' is a member of Cell
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

    /*
     * Counts the charges in dimension W.
     */
    void compileNetCharges(Cell *mirror, Cell *draft)
    {
      // Compile blindly net charges
      if (mirror->charge & C0_MASK)
        draft->net_c0++;
      else
        draft->net_c0--;
      //
      if (mirror->charge & C1_MASK)
        draft->net_c1++;
      else
        draft->net_c1--;
      //
      if (mirror->charge & C2_MASK)
        draft->net_c2++;
      else
        draft->net_c2--;
      //
      if (mirror->charge & Q_MASK)
        draft->net_q++;
      else
        draft->net_q--;
      //
      if (mirror->charge & W0_MASK)
        draft->net_w0++;
      else
        draft->net_w0--;
      //
      if (mirror->charge & W1_MASK)
        draft->net_w1++;
      else
        draft->net_w1--;
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
          lattice_curr[x][y][z][w].pole = true;
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
          lattice_curr[x][y][z][w].pole = true;
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
          lattice_curr[x][y][z][w].pole = true;
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

    /**
     * Implements a circular shift of the fourth dimension (w) in
     * the lattice_draft array for every [x][y][z] coordinate.
     */
    void shiftW()
    {
        for (unsigned x = 0; x < SIDE; ++x)
        {
            for (unsigned y = 0; y < SIDE; ++y)
            {
                for (unsigned z = 0; z < SIDE; ++z)
                {
                    // Save the last element for wrapping
                    Cell last = lattice_draft[x][y][z][W_DIM - 1];
                    // Use `std::copy_backward` to shift elements efficiently
                    std::copy_backward(&lattice_draft[x][y][z][0],
                                       &lattice_draft[x][y][z][W_DIM - 1],
                                       &lattice_draft[x][y][z][W_DIM]);
                    // Set the first element to the saved last element
                    lattice_draft[x][y][z][0] = last;
                }
            }
        }
    }

}
