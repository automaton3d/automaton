/*
 * GUI_3D.cpp (merged)
 */

#include "GUI.h"
#include "model/simulation.h"
#include <GL/freeglut.h>
#include <glm/gtc/type_ptr.hpp>
#include "recorder.h"
#include "projection.h"

AxisProjection gAxisProj[3];
bool gAxisProjValid = false;

namespace automaton
{
  extern unsigned EL;
  extern unsigned L2;
  extern int scenario;
}

namespace framework
{
  extern int GUImode;
  extern bool MULTICUBE_MODE;
  extern GLint gViewport[4];
  extern Tickbox *tomo;

  using namespace automaton;

  int vis_dx = 0;
  int vis_dy = 0;
  int vis_dz = 0;

  /**
    * Renders momentum line.
    */
  void GUIrenderer::renderMomentum()
  {
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          int wx = (x + vis_dx + EL) % EL;
          int wy = (y + vis_dy + EL) % EL;
          int wz = (z + vis_dz + EL) % EL;
          if (getCell(lattice_curr, wx, wy, wz, layerList->getSelected()).pB)
          {
            if (isVoxelVisible(wx, wy, wz))
            {
              glColor3f(1.0f, 1.0f, 0.0f);  // Yellow
            }
            else
            {
              glColor3d(0.4, 0.4, 0.4);
            }
            if (GUImode == REPLAY)
            {
              // Shift the momentum point by the layer's center
              unsigned w = layerList->getSelected();
              const auto& center = automaton::lcenters[w];
              float cx = (float)center[0];
              float cy = (float)center[1];
              float cz = (float)center[2];

              // Calculate position relative to the layer's center (cx, cy, cz)

              float px = (int)(x - cx) * GRID_SIZE;
              float py = (int)(y - cy) * GRID_SIZE;
              float pz = (int)(z - cz) * GRID_SIZE;
              glVertex3f(px, py, pz);
            }
            else
            {
              // Normal rendering mode (centered on global lattice center)
              float px = (int)(x - CENTER_INT) * GRID_SIZE;
              float py = (int)(y - CENTER_INT) * GRID_SIZE;
              float pz = (int)(z - CENTER_INT) * GRID_SIZE;

              glVertex3f(px, py, pz);
            }
          }
        }
      }
    }
    glEnd();
  }

  /**
   * Renders the spiral pattern.
   */
  void GUIrenderer::renderSpin()
  {
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          int wx = (x + vis_dx + EL) % EL;
          int wy = (y + vis_dy + EL) % EL;
          int wz = (z + vis_dz + EL) % EL;
          if (getCell(lattice_curr, wx, wy, wz, layerList->getSelected()).pB)
          {
            if (isVoxelVisible(x, y, z))
              glColor3d(0, 1.0, 1.0);   // Cyan
            else
              glColor3d(0.4, 0.4, 0.4);
            if (GUImode == REPLAY)
            {
              unsigned w = layerList->getSelected();
              const auto& center = automaton::lcenters[w];
              float cx = (float)center[0];
              float cy = (float)center[1];
              float cz = (float)center[2];

              // Calculate position relative to the layer's center (cx, cy, cz)

              float px = (int)(x - cx) * GRID_SIZE;
              float py = (int)(y - cy) * GRID_SIZE;
              float pz = (int)(z - cz) * GRID_SIZE;
              glVertex3f(px, py, pz);
            }
            else
            {
              float px = (int)(x - CENTER_INT) * GRID_SIZE;
              float py = (int)(y - CENTER_INT) * GRID_SIZE;
              float pz = (int)(z - CENTER_INT) * GRID_SIZE;
              glVertex3f(px, py, pz);
            }
          }
        }
      }
    }
    glEnd();
  }

  /**
   * Renders the sine squared mask.
   */
  void GUIrenderer::renderSineMask()
  {
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (!isVoxelVisible(x, y, z))
            continue;
          int wx = (x + vis_dx + EL) % EL;
          int wy = (y + vis_dy + EL) % EL;
          int wz = (z + vis_dz + EL) % EL;
          if (getCell(lattice_curr, wx, wy, wz, layerList->getSelected()).pB)
          {
            glColor3d(1.0, 1.0, 0);
            float px = (int)(x - CENTER_INT) * GRID_SIZE;
            float py = (int)(y - CENTER_INT) * GRID_SIZE;
            float pz = (int)(z - CENTER_INT) * GRID_SIZE;

            glVertex3f(px, py, pz);
          }
        }
      }
    }
    glEnd();
  }

  /**
   * Renders the hunting pattern.
   */
  void GUIrenderer::renderHunting()
  {
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          int wx = (x + vis_dx + EL) % EL;
          int wy = (y + vis_dy + EL) % EL;
          int wz = (z + vis_dz + EL) % EL;
          if (getCell(lattice_curr, wx, wy, wz, layerList->getSelected()).pB)
          {
            glColor3d(1.0, 1.0, 0);
            float px = (int)(x - CENTER_INT) * GRID_SIZE;
            float py = (int)(y - CENTER_INT) * GRID_SIZE;
            float pz = (int)(z - CENTER_INT) * GRID_SIZE;
            glVertex3f(px, py, pz);
          }
        }
      }
    }
    glEnd();
  }

  /**
   * Draws the points representing the active wavefront cells of
   * the current layer.
   */
  void GUIrenderer::renderWavefront()
  {
    const float GRID_SIZE = 0.5 / EL;
    glPointSize(2.5f);

    int offsets[3] = {-1, 0, 1};
    const int CENTER_INT = EL / 2;

    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          COLORREF color = automaton::voxels[x * automaton::L2 + y * EL + z];
          if (!color)
            continue;

          BYTE r = GetRValue(color);
          BYTE g = GetGValue(color);
          BYTE b = GetBValue(color);

          GLdouble red   = r / 255.0;
          GLdouble green = g / 255.0;
          GLdouble blue  = b / 255.0;
          float alpha = 0.5;

          glColor4d(red, green, blue, alpha);

          float px = ((int)((x + vis_dx) % EL) - CENTER_INT) * GRID_SIZE;
          float py = ((int)((y + vis_dy) % EL) - CENTER_INT) * GRID_SIZE;
          float pz = ((int)((z + vis_dz) % EL) - CENTER_INT) * GRID_SIZE;

          if (MULTICUBE_MODE)
          {
            for (int dx : offsets)
            {
              for (int dy : offsets)
              {
                for (int dz : offsets)
                {
                  glVertex3f(px + dx * 0.5, py + dy * 0.5, pz + dz * 0.5);
                }
              }
            }
          }
          else
          {
            glVertex3f(px, py, pz);
          }
        }
      }
    }
    glEnd();
    enhanceVoxel();
  }

  /*
   * Render the center of the bubbles only.
   * All layers (w dimension) contribute.
   */
  void GUIrenderer::renderCenters()
  {
    // Clear previous positions
    screenPositions_.clear();

    // Get matrices and viewport
    GLfloat modelview[16];
    GLfloat projection[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;

    glPointSize(8.0f);
    glBegin(GL_POINTS);
    for (unsigned w = 0; w < W_USED; w++)
    {
      const auto& center = automaton::lcenters[w];
      unsigned cx = center[0];
      unsigned cy = center[1];
      unsigned cz = center[2];
      Cell &cell = getCell(lattice_curr, cx, cy, cz, w);
      float alpha = 0.5f;
      float r = 0.7f + (w & 1) * 0.3f;
      float g = 0.7f + ((w >> 1) & 1) * 0.3f;
      float b = 0.7f + ((w >> 2) & 1) * 0.3f;

      float px = ((int)((int)cell.x[0] + vis_dx) - CENTER_INT) * GRID_SIZE;
      float py = ((int)((int)cell.x[1] + vis_dy) - CENTER_INT) * GRID_SIZE;
      float pz = ((int)((int)cell.x[2] + vis_dz) - CENTER_INT) * GRID_SIZE;

      glColor4f(r, g, b, alpha);
      glVertex3f(px, py, pz);

      // Manual projection using GLfloat (updated from old GLdouble)
      float sx, sy;
      float obj[3] = {px, py, pz};
      if (framework::projectPoint(obj, modelview, projection, gViewport, sx, sy))
      {
        screenPositions_.push_back({sx, sy});
      }
    }
    glEnd();

    // Count duplicates
    std::map<std::pair<int,int>, int> counts;
    for (auto &pos : screenPositions_)
    {
      int x = static_cast<int>(pos[0] + 0.5f);
      int y = static_cast<int>(pos[1] + 0.5f);
      counts[{x, y}]++;
    }
  }

  /**
   * Renders 2D and 3D objects controlled by the mouse and keyboard.
   */
  void GUIrenderer::renderObjects()
  {
    glEnable(GL_DEPTH_TEST);
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(mProjection_));
    if (mCamera)
    {
      glPushMatrix();
      glMultMatrixf(mCamera->getMatrixFlat());
    }
    if (scenario >= 0)
    {
      if (data3D[0].getState())
        renderWavefront();
      if (data3D[1].getState())
        renderMomentum();
      if (data3D[2].getState())
        renderSpin();
      if (data3D[3].getState())
        renderSineMask();
      if (data3D[4].getState())
        renderHunting();
      if (data3D[5].getState())
        renderCenters();
    }
    if (data3D[6].getState())
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      renderCube();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (data3D[7].getState())
      renderAxes();
    if (data3D[8].getState())
      renderPlane();
    if (tomo && tomo->getState())
      renderTomoPlane();

    if (mCamera)
    {
      glPopMatrix();
    }
    glPopMatrix();
  }

  /**
   * Renders the cartesian positive axes with labels.
   * IMPROVED: Now includes complete axis projection caching for mouse picking.
   */
  void GUIrenderer::renderAxes()
  {
    const float fullSize = 0.5f;
    const float axisLength = fullSize * 0.75f;
    const float labelOffset = 0.02f;

    glLineWidth(2);
    glBegin(GL_LINES);
    // X-axis
    glColor3f(0.6f, 0.f, 0.f);
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(axisLength, 0.f, 0.f);
    // Y-axis
    glColor3f(0.f, 0.6f, 0.f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, axisLength, 0.f);
    // Z-axis
    glColor3f(0.3f, 0.3f, 0.8f);
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, axisLength);
    glEnd();
    glLineWidth(1);

    // Axis labels
    glColor3f(1.f, 0.f, 0.f);
    render3Dstring(axisLength + labelOffset, -labelOffset, 0.0f, GLUT_BITMAP_HELVETICA_18, "x");

    glColor3f(0.f, 1.f, 0.f);
    render3Dstring(-labelOffset, axisLength + labelOffset, 0.0f, GLUT_BITMAP_HELVETICA_18, "y");

    glColor3f(0.3f, 0.3f, 0.8f);
    render3Dstring(0.0f, -labelOffset, axisLength + labelOffset, GLUT_BITMAP_HELVETICA_18, "z");

    // Draw thumb if active
    if (thumb.active) {
        glPointSize(10.0f);
        glColor3f(1.0f, 0.0f, 0.0f); // red dot
        glBegin(GL_POINTS);
        if (thumb.axis == 0) glVertex3f(thumb.position, 0.0f, 0.0f);
        if (thumb.axis == 1) glVertex3f(0.0f, thumb.position, 0.0f);
        if (thumb.axis == 2) glVertex3f(0.0f, 0.0f, thumb.position);
        glEnd();
    }

    // === IMPROVED: Complete axis projection caching for mouse picking ===
    GLfloat model[16], proj[16];
    GLint viewport[4];
    glGetFloatv(GL_MODELVIEW_MATRIX, model);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Origin (shared by all axes)
    float ox, oy;
    projectPoint((float[3]){0.0f, 0.0f, 0.0f}, model, proj, viewport, ox, oy);
    gAxisProj[0].x0 = gAxisProj[1].x0 = gAxisProj[2].x0 = ox;
    gAxisProj[0].y0 = gAxisProj[1].y0 = gAxisProj[2].y0 = oy;

    // Endpoints per axis
    projectPoint((float[3]){axisLength, 0.0f, 0.0f}, model, proj, viewport,
                 gAxisProj[0].x1, gAxisProj[0].y1);
    projectPoint((float[3]){0.0f, axisLength, 0.0f}, model, proj, viewport,
                 gAxisProj[1].x1, gAxisProj[1].y1);
    projectPoint((float[3]){0.0f, 0.0f, axisLength}, model, proj, viewport,
                 gAxisProj[2].x1, gAxisProj[2].y1);

    gAxisProjValid = true;
  }

  /**
   * Renders the 3D space outline.
   */
  void GUIrenderer::renderCube()
  {
    GLfloat alpha = 0.6f;
    glBegin(GL_QUADS);
    glColor4f(0.45f, 0.13f, 0.13f, alpha);
    // Right face
    glVertex3f( 0.25f, -0.25f, -0.25f);
    glVertex3f( 0.25f,  0.25f, -0.25f);
    glVertex3f( 0.25f,  0.25f,  0.25f);
    glVertex3f( 0.25f, -0.25f,  0.25f);
    // Left face
    glVertex3f(-0.25f, -0.25f, -0.25f);
    glVertex3f(-0.25f,  0.25f, -0.25f);
    glVertex3f(-0.25f,  0.25f,  0.25f);
    glVertex3f(-0.25f, -0.25f,  0.25f);
    // Top face
    glVertex3f(-0.25f,  0.25f, -0.25f);
    glVertex3f( 0.25f,  0.25f, -0.25f);
    glVertex3f( 0.25f,  0.25f,  0.25f);
    glVertex3f(-0.25f,  0.25f,  0.25f);
    // Bottom face
    glVertex3f(-0.25f, -0.25f, -0.25f);
    glVertex3f( 0.25f, -0.25f, -0.25f);
    glVertex3f( 0.25f, -0.25f,  0.25f);
    glVertex3f(-0.25f, -0.25f,  0.25f);
    // Front face
    glVertex3f(-0.25f, -0.25f,  0.25f);
    glVertex3f( 0.25f, -0.25f,  0.25f);
    glVertex3f( 0.25f,  0.25f,  0.25f);
    glVertex3f(-0.25f,  0.25f,  0.25f);
    // Back face
    glVertex3f(-0.25f, -0.25f, -0.25f);
    glVertex3f( 0.25f, -0.25f, -0.25f);
    glVertex3f( 0.25f,  0.25f, -0.25f);
    glVertex3f(-0.25f,  0.25f, -0.25f);
    glEnd();
  }

  /*
   * Renders a reference plane to help in the visualization.
   */
  void GUIrenderer::renderPlane()
  {
    const float GRID_SIZE = 0.5f / EL;
    const float half = EL * GRID_SIZE / 2.0f;
    const float eps = -1e-4f;

    glLineWidth(1.0f);
    glColor4f(1.f, 1.f, 1.f, 0.2f);

    glBegin(GL_LINES);
    for (unsigned i = 0; i <= EL; ++i)
    {
      float p = -half + i * GRID_SIZE;

      // Lines parallel to X (constant Z)
      glVertex3f(p, eps, -half);
      glVertex3f(p, eps,  half);

      // Lines parallel to Z (constant X)
      glVertex3f(-half, eps, p);
      glVertex3f( half, eps, p);
    }
    glEnd();
  }


  /*
   * Used to enhance the particle belonging the current layer.
   */
  void GUIrenderer::enhanceVoxel()
  {
    unsigned w = layerList->getSelected();
    const float GRID_SIZE = 0.5f / EL;

    float cx, cy, cz;

    if (GUImode == REPLAY)
    {
      // In replay mode, use the actual relocated center from lcenters
      const auto& center = automaton::lcenters[w];
      cx = ((center[0] + vis_dx) - EL / 2) * GRID_SIZE;
      cy = ((center[1] + vis_dy) - EL / 2) * GRID_SIZE;
      cz = ((center[2] + vis_dz) - EL / 2) * GRID_SIZE;

    }
    else
    {
      // In simulation mode, use the cell position at the static center
      Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
      cx = ((cell.x[0] + vis_dx) - EL / 2) * GRID_SIZE;
      cy = ((cell.x[1] + vis_dy) - EL / 2) * GRID_SIZE;
      cz = ((cell.x[2] + vis_dz) - EL / 2) * GRID_SIZE;
    }

    glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor3d(0.7, 0.7, 0.7);

    const int POINT_COUNT = 12;
    const float MAX_OFFSET = 0.005f;
    for (int i = 0; i < POINT_COUNT; ++i)
    {
      float dx = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      float dy = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      float dz = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      glVertex3f(cx + dx, cy + dy, cz + dz);
    }
    glEnd();
  }

}
