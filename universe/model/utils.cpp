/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "../GUIrenderer.h"
#include "simulation.h"

namespace automaton
{
  extern bool reloc_x[W_DIM], reloc_y[W_DIM], reloc_z[W_DIM];
  extern bool reloc_w;
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
    // Check if data is not transient
    if (lattice_curr[0][0][0][0].k < UPDATE)
      return;
    int w = framework::list.getSelected();

    /*
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
    */

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
          if (cell.wv)
            voxels[index3D] = RGB(255, 255, 255);  // White voxel
//        else if (cell.t == 0)
  //        voxels[index3D] = RGB(255, 0, 0);//DEBUG
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

  /*
   * Circular Shift in the Fourth Dimension W
   */
  void shiftW()
  {
    // Iterate through each 3D grid position
    for (int x = 0; x < SIDE; x++)
    {
      for (int y = 0; y < SIDE; y++)
      {
        for (int z = 0; z < SIDE; z++)
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
    reloc_w = false;
  }

  /**
   * Circular shift in the X direction.
   */
  void shiftX(unsigned w)
  {
    // Reset relocation flag
    reloc_x[w] = false;
    // Temporary storage for the rightmost column of each xz plane
    Cell temp[SIDE][SIDE];
    // Copy the rightmost column to the temporary storage
    for (unsigned int y = 0; y < SIDE; ++y)
    {
      for (unsigned int z = 0; z < SIDE; ++z)
      {
        temp[y][z] = lattice_draft[SIDE - 1][y][z][w];
      }
    }
    // Shift all other columns one position to the right
    for (unsigned int x = SIDE - 1; x > 0; --x)
    {
      for (unsigned int y = 0; y < SIDE; ++y)
      {
        for (unsigned int z = 0; z < SIDE; ++z)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[((x + SIDE) - 1) % SIDE][y][z][w];
          if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
          {
        	draft.aff = w;
            draft.t = 0;
          }
          else
          {
          	draft.aff = W_DIM;
          }
        }
      }
    }
    // Copy the stored rightmost column to the leftmost column
    for (unsigned int y = 0; y < SIDE; ++y)
    {
      for (unsigned int z = 0; z < SIDE; ++z)
      {
        Cell &draft = lattice_draft[0][y][z][w];
        draft = temp[y][z];
        if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
        {
      	  draft.aff = w;
      	  draft.t = 0;
        }
        else
        {
          draft.aff = W_DIM;
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
    Cell temp[SIDE][SIDE];
    // Copy the topmost row to the temporary storage
    for (unsigned int x = 0; x < SIDE; ++x)
    {
      for (unsigned int z = 0; z < SIDE; ++z)
      {
        temp[x][z] = lattice_draft[x][SIDE - 1][z][w];
      }
    }
    // Shift all other rows one position upwards
    for (unsigned int y = SIDE - 1; y > 0; --y)
    {
      for (unsigned int x = 0; x < SIDE; ++x)
      {
        for (unsigned int z = 0; z < SIDE; ++z)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[x][((y + SIDE) - 1) % SIDE][z][w];
          if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
          {
        	draft.aff = w;
            draft.t = 0;
          }
          else
          {
          	draft.aff = W_DIM;
          }
        }
      }
    }
    // Copy the stored topmost row to the bottommost row
    for (unsigned int x = 0; x < SIDE; ++x)
    {
      for (unsigned int z = 0; z < SIDE; ++z)
      {
        Cell &draft = lattice_draft[x][0][z][w];
        draft = temp[x][z];
        if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
        {
      	  draft.aff = w;
      	  draft.t = 0;
        }
        else
        {
          draft.aff = W_DIM;
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
    Cell temp[SIDE][SIDE];
    // Copy the frontmost slice to the temporary storage
    for (unsigned int x = 0; x < SIDE; ++x)
    {
      for (unsigned int y = 0; y < SIDE; ++y)
      {
        temp[x][y] = lattice_draft[x][y][SIDE - 1][w];
      }
    }
    // Shift all other slices one position towards the front
    for (unsigned int z = SIDE - 1; z > 0; --z)
    {
      for (unsigned int x = 0; x < SIDE; ++x)
      {
        for (unsigned int y = 0; y < SIDE; ++y)
        {
          Cell &draft = lattice_draft[x][y][z][w];
          draft = lattice_draft[x][y][((z + SIDE) - 1) % SIDE][w];
          if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
          {
        	draft.aff = w;
            draft.t = 0;
          }
          else
          {
          	draft.aff = W_DIM;
          }
        }
      }
    }
    // Copy the stored frontmost slice to the backmost slice
    for (unsigned int x = 0; x < SIDE; ++x)
    {
      for (unsigned int y = 0; y < SIDE; ++y)
      {
        Cell &draft = lattice_draft[x][y][0][w];
        draft = temp[x][y];
        if (draft.pos[0] == CENTER && draft.pos[1] == CENTER && draft.pos[2] == CENTER)
        {
      	  draft.aff = w;
      	  draft.t = 0;
        }
        else
        {
          draft.aff = W_DIM;
        }
      }
    }
  }

}
