/*
 * GUI_Tomo.cpp
 */

#include "GUI.h"
#include <GL/freeglut.h>
#include "hslider.h"
#include "tickbox.h"
#include "radio.h"
#include "model/simulation.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned L2;
}

namespace framework
{
  extern Tickbox *tomo;
  extern HSlider hslider;
  extern vector<Radio> tomoDirs;
  extern unsigned tomo_x, tomo_y, tomo_z;

  void GUIrenderer::renderSlice()
  {
    if (!tomo || !tomo->getState()) return;

    const float GRID_SIZE = 0.5f / EL;
    unsigned sliceIndex = hslider.getSliceIndex(EL);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; ++x)
    {
      for (unsigned y = 0; y < EL; ++y)
      {
        for (unsigned z = 0; z < EL; ++z)
        {
          bool match = false;
          if (tomoDirs[0].isSelected())
            match = (z == sliceIndex);
          else if (tomoDirs[1].isSelected())
            match = (x == sliceIndex);
          else if (tomoDirs[2].isSelected())
            match = (y == sliceIndex);
          if (!match) continue;
          COLORREF color = automaton::voxels[x * automaton::L2 + y * EL + z];
          if (!color) continue;

          BYTE r = GetRValue(color);
          BYTE g = GetGValue(color);
          BYTE b = GetBValue(color);
          glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.6f);

          float px = (x - EL / 2) * GRID_SIZE;
          float py = (y - EL / 2) * GRID_SIZE;
          float pz = (z - EL / 2) * GRID_SIZE;
          glVertex3f(px, py, pz);
        }
      }
    }
    glEnd();
  }

  void GUIrenderer::renderTomoControls()
  {
    tomo->draw();
  }

  void GUIrenderer::renderTomoPlane()
  {
    if (!tomo || !tomo->getState()) return;

    // Update the tomo slice coordinates from the slider
    unsigned sliceIndex = hslider.getSliceIndex(EL);
    if (tomoDirs[0].isSelected())
      tomo_z = sliceIndex;  // XY plane
    else if (tomoDirs[1].isSelected())
      tomo_x = sliceIndex;  // YZ plane
    else if (tomoDirs[2].isSelected())
      tomo_y = sliceIndex;  // ZX plane

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float GRID_SIZE = 0.5f / EL;
    const float HALF = EL / 2.0f;

    float coord = (hslider.getSliceIndex(EL) - HALF + 0.5f) * GRID_SIZE;

    glColor4f(0.3f, 0.8f, 1.0f, 0.2f);
    glBegin(GL_QUADS);
    if (tomoDirs[0].isSelected())
    {
      glVertex3f(-0.25f, -0.25f, coord);
      glVertex3f( 0.25f, -0.25f, coord);
      glVertex3f( 0.25f,  0.25f, coord);
      glVertex3f(-0.25f,  0.25f, coord);
    }
    else if (tomoDirs[1].isSelected())
    {
      glVertex3f(coord, -0.25f, -0.25f);
      glVertex3f(coord,  0.25f, -0.25f);
      glVertex3f(coord,  0.25f,  0.25f);
      glVertex3f(coord, -0.25f,  0.25f);
    }
    else if (tomoDirs[2].isSelected())
    {
      glVertex3f(-0.25f, coord, -0.25f);
      glVertex3f( 0.25f, coord, -0.25f);
      glVertex3f( 0.25f, coord,  0.25f);
      glVertex3f(-0.25f, coord,  0.25f);
    }
    glEnd();

    glPointSize(8.0f);
    glColor4f(0.5f, 1.0f, 1.0f, 0.6f);
    glBegin(GL_POINTS);
    if (tomoDirs[0].isSelected())
      glVertex3f(0.0f, 0.0f, coord);
    else if (tomoDirs[1].isSelected())
      glVertex3f(coord, 0.0f, 0.0f);
    else if (tomoDirs[2].isSelected())
      glVertex3f(0.0f, coord, 0.0f);
    glEnd();

    glDisable(GL_BLEND);
  }

  /*
  inline bool GUIrenderer::isVoxelVisible(unsigned x, unsigned y, unsigned z)
  {
    if (!tomo || !tomo->getState())
      return true;

    if (tomoDirs[0].isSelected())
      return z == tomo_z;
    if (tomoDirs[1].isSelected())
      return x == tomo_x;
    if (tomoDirs[2].isSelected())
      return y == tomo_y;

    return true;
  }
  */
}
