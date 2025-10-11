/*
 * mygl.cpp
 *
 * Implements the OpenGL rendering routines.
 */

#include "GUIrenderer.h"

#include "GLutils.h"
#include "layers.h"
#include "slider.h"

namespace framework
{
  using namespace std;

  extern unsigned long long timer;
  extern bool pause;
  extern void setOrthographicProjection();
  extern void resetPerspectiveProjection();

  // Global viewport info
  GLint gViewport[4] = {0, 0, 1920, 1080}; // default values, will be overwritten on resize

  // Gadgets
  vector<Tickbox> checkboxes;
  vector<Tickbox> delays;
  vector<Radio> viewpoint;
  LayerList list;
  LayerSlider slider(1890, 93, 10.0f, 607.0f, 30.0f);

  // Auxiliary
  random_device rd;
  mt19937 gen(rd());
  bool spinFlag;
  unsigned long tbegin;
  int barWidths[3];
  const float GRID_SIZE = 0.5 / EL;
  unsigned lastPos[W_DIM][3];
  // Global flag to control rendering mode: single cube or 27 cubes.
  bool MULTICUBE_MODE = false;

  // Static text
  string help[10] =
  {
    "           c: Print camera Eye, Center, Up",
    "           r: Reset view",
    "           t: Toggle right button to do Pan or First-Person",
    "     x, y, z: Snap camera to axis",
    "  Left-Click: Rotate",
    "Middle-Click: Pan or First-Person",
    " Right-Click: Roll",
    "Scroll-Wheel: Dolly (zoom)",
	"      Escape: EXIT"
  };

  string steps[3] =
  {
    "Convolution",
    "Diffusion",
    "Relocation"
  };

  using namespace automaton;

  /**
   * Default constructor.
   */
  GUIrenderer::GUIrenderer() : Renderer()
  {
  }

  /**
   * Destructor
   */
  GUIrenderer::~GUIrenderer()
  {
  }

  /**
   * Initializes the GUI.
   */
  void GUIrenderer::init()
  {
    voxels = (COLORREF*) malloc(L3 * sizeof(COLORREF));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    tbegin = GetTickCount64();
    //
    checkboxes.push_back(Tickbox(50, 100, "Wavefront")); // 0
    checkboxes.push_back(Tickbox(50, 130, "Momentum"));  // 1
    checkboxes.push_back(Tickbox(50, 160, "Spin"));      // 2
    checkboxes.push_back(Tickbox(50, 190, "Sine mask")); // 3
    checkboxes.push_back(Tickbox(50, 220, "Hunting"));   // 4
    checkboxes.push_back(Tickbox(50, 250, "Centers"));   // 5
    checkboxes.push_back(Tickbox(50, 280, "Lattice"));   // 6
    checkboxes.push_back(Tickbox(50, 310, "Axes"));      // 7
    checkboxes.push_back(Tickbox(50, 350, "Plane"));     // 8
    checkboxes[0].setState(true);
    checkboxes[5].setState(true);
//    checkboxes[7].setState(true);
  //  checkboxes[8].setState(true);
    //
    delays.push_back(Tickbox(50, 420, "Convolution"));
    delays.push_back(Tickbox(50, 450, "Diffusion"));
    delays.push_back(Tickbox(50, 480, "Relocation"));
    //
    viewpoint.push_back(Radio(60, 570, "Isometric"));
    viewpoint.push_back(Radio(60, 600, "XY"));
    viewpoint.push_back(Radio(60, 630, "YZ"));
    viewpoint.push_back(Radio(60, 660, "ZX"));
    viewpoint[0].setSelected(true);
    // Initialize progress bar data
    int barWidth = gViewport[2] / 4; // Bar is 1/4 of the screen width
    double totalRatio = (double) FRAME;
    barWidths[0] = (int)(barWidth * (double) CONVOL / totalRatio);
    barWidths[1] = (int)(barWidth * (double) (DIFFUSION - CONVOL) / totalRatio);
    barWidths[2] = (int)(barWidth * (double) (RELOC - DIFFUSION) / totalRatio);
  }

  /**
   * Renders the GUI.
   */
  void GUIrenderer::render()
  {
    renderClear();
    renderObjects();
    render2DObjects();
  }

  /**
   * Clear the GUI.
   */
  void GUIrenderer::renderClear()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
  }

  void GUIrenderer::renderList()
  {

  }

  /*
   * Renders a progress bar showing a complete light step.
   */
  void GUIrenderer::renderProgressBar()
  {
    int screenWidth = gViewport[2];
    // Progress bar dimensions
    int barWidth = screenWidth / 4;
    int barHeight = 20; // Fixed height
    int barX = (screenWidth - barWidth) / 2; // Center horizontally
    int barY = 100; // Fixed height from bottom
    // Calculate pointer position based on timer % FRAME
    float progress = static_cast<float>(timer % FRAME) / FRAME;
    int pointerX = barX + static_cast<int>(progress * barWidth);
    // Calculate legend parameters
    int x0 = barX + barWidth + 22;
    int w = 20;
    int h = 5;
    int y0 = barY - 15;
    // Draw the sections of the bar with proportional widths
    int accumulatedWidth = 0;
    for (int i = 0; i < 3; i++)
    {
      // Calculate this section's start and end positions
      int sectionStart = accumulatedWidth;
      int sectionEnd = accumulatedWidth + barWidths[i];
      // Set color for each section
      if (timer % FRAME > 2)
      {
        switch (i)
        {
          case 0: glColor3f(0.3f, 0.3f, 0.0f); break;
          case 1: glColor3f(0.5f, 0.0f, 0.0f); break;
          case 2: glColor3f(0.0f, 0.5f, 0.0f); break;
        }
      }
      else
      {
        switch (i)
        {
          case 0: glColor3f(1, 1, 1); break;
          case 1: glColor3f(1, 1, 1); break;
          case 2: glColor3f(1, 1, 1); break;
        }
      }
      // Draw the section
      glBegin(GL_QUADS);
        glVertex2i(barX + sectionStart, barY);
        glVertex2i(barX + sectionEnd, barY);
        glVertex2i(barX + sectionEnd, barY + barHeight);
        glVertex2i(barX + sectionStart, barY + barHeight);
      glEnd();
      // Draw legend
      glBegin(GL_TRIANGLES);
        glVertex2f(x0, y0);
      glVertex2f(x0 + w, y0);
      glVertex2f(x0 + w, y0 + h);
      glVertex2f(x0, y0);
      glVertex2f(x0 + w, y0 + h);
      glVertex2f(x0, y0 + h);
      glEnd();
      glColor3f(0.6f, 0.6f, 0.6f);
      drawString12(steps[i], x0 + w + 10, y0 + 7);
      // Update the accumulated width
      accumulatedWidth += barWidths[i];
      y0 += 20;
    }
    // Draw the progress bar outline
    glColor3f(0.6f, 0.6f, 0.6f); // White outline
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2i(barX-1, barY);
    glVertex2i(barX-1 + barWidth, barY);
    glVertex2i(barX-1 + barWidth, barY + barHeight);
    glVertex2i(barX-1, barY + barHeight);
    glEnd();
    // Draw the pointer
    glColor3f(1.0f, 1.0f, 0.0f); // Red pointer
    glBegin(GL_QUADS);
    glVertex2i(pointerX - 2, barY+2);          // Pointer width: 10
    glVertex2i(pointerX + 2, barY+2);
    glVertex2i(pointerX + 2, barY + barHeight - 2);
    glVertex2i(pointerX - 2, barY + barHeight - 2);
    glEnd();
  }

  /**
   * Opens a pause message box with centered text and an outline.
   */
  void GUIrenderer::renderCenterBox(const char* text)
  {
    int viewportWidth = gViewport[2];
    int viewportHeight = gViewport[3];

    // Calculate text dimensions dynamically
    int textWidth = strlen(text) * 10;
    int textHeight = 10;

    // Center the text in the viewport
    int textX = (viewportWidth - textWidth) / 2;
    int textY = (viewportHeight + textHeight) / 2;

    // Padding for the rectangle
    int paddingX = 15;
    int paddingY = 10;

    // Rectangle dimensions and position
    int rectX = textX - paddingX;
    int rectY = textY - textHeight - paddingY;
    int rectWidth = textWidth + 2 * paddingX;
    int rectHeight = textHeight + 2 * paddingY;

    // Draw the text
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text
    drawString8(text, textX, textY); // Render the text at the calculated position

    // Draw the rectangle around the text
    glColor3f(1.0f, 1.0f, 1.0f); // White color for rectangle
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2i(rectX, rectY);
    glVertex2i(rectX + rectWidth, rectY);
    glVertex2i(rectX + rectWidth, rectY + rectHeight);
    glVertex2i(rectX, rectY + rectHeight);
    glEnd();
  }

  void GUIrenderer::renderCounts()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    // Count duplicates
    std::map<std::pair<int,int>, int> counts;
    for (auto &pos : screenPositions)
    {
      int x = static_cast<int>(pos[0] + 0.5f);
      int y = static_cast<int>(pos[1] + 0.5f);
      counts[{x, y}]++;
    }
    // Save matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport[2], 0, viewport[3], -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 0.0f); // yellow
    for (auto &entry : counts)
    {
      int x = entry.first.first;
      int y = entry.first.second;
      int n = entry.second;
      char label[16];
      sprintf(label, " %d", n);
      glRasterPos2i(x, y); // flip y
      for (char *c = label; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
  }

  /**
   * Renders 2D objects.
   */
  void GUIrenderer::render2DObjects()
  {
    setOrthographicProjection();
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);
    unsigned long millis = GetTickCount64() - tbegin;
    char s[100];
    sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
    drawString8(s, 50, 40);
    sprintf(s, "Light: %llu   Tick: %llu", timer / automaton::FRAME, timer);
    render2Dstring(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
    sprintf(s, "L = %u", EL);
    render2Dstring(1750, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
    int w = framework::list.getSelected();
    sprintf(s, "(Current layer = %u)", w);
    render2Dstring(1730, 78, GLUT_BITMAP_HELVETICA_12, s);
    // Get the primary monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    // Get the video mode of the monitor
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    // Draw the help text
    for(int i = 0; i < 10; i++)
      drawString8(help[i], mode->width - 500, 20 * i + mode->height - 260);
    // Add the progress bar rendering
    renderProgressBar();
    // Add a vertical slider
    slider.draw();
    // Handle pause window
    if (pause)
      renderCenterBox(" Paused ");
    // Draw labels
    glColor3f(1.0f, 1.0f, 0.5f); // Blue
    drawString12("Data", 50, 85);
    drawString12("Delays", 50, 405);
    drawString12("Views", 50, 551);
    glPopMatrix();
    list.update();
    resetPerspectiveProjection();
    renderCounts();
  }

  /**
   * Renders momentum line.
   */
  void GUIrenderer::renderMomentum()
  {
    const float GRID_SIZE = 1.0f / EL;  // Better spacing
    glPointSize(5.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          if (lattice_curr[x][y][z][list.getSelected()].pB)
          {
            glColor3d(1.0, 1.0, 0);     // Yellow
            float px = (x - EL / 2) * GRID_SIZE;
            float py = (y - EL / 2) * GRID_SIZE;
            float pz = (z - EL / 2) * GRID_SIZE;
            glVertex3f(px, py, pz);
            //printf("%f,%f,%f\n", px, py, pz);
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
    const float GRID_SIZE = 1.0f / EL;  // Better spacing
    glPointSize(5.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          if (lattice_curr[x][y][z][list.getSelected()].sB)
          {
            glColor3d(0, 1.0, 1.0);    // Cyan
            float px = (x - EL / 2) * GRID_SIZE;
            float py = (y - EL / 2) * GRID_SIZE;
            float pz = (z - EL / 2) * GRID_SIZE;
            glVertex3f(px, py, pz);
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
    const float GRID_SIZE = 1.0f / EL;  // Better spacing
    glPointSize(5.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          if (lattice_curr[x][y][z][list.getSelected()].phiB)
          {
            glColor3d(1.0, 1.0, 0);     // Yellow
            float px = (x - EL / 2) * GRID_SIZE;
            float py = (y - EL / 2) * GRID_SIZE;
            float pz = (z - EL / 2) * GRID_SIZE;
            glVertex3f(px, py, pz);
          }
        }
      }
    }
    glEnd();
  }

  /**
   * Renders the sine squared mask.
   */
  void GUIrenderer::renderHunting()
  {
    const float GRID_SIZE = 1.0f / EL;  // Better spacing
    glPointSize(5.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          if (lattice_curr[x][y][z][list.getSelected()].hB)
          {
            glColor3d(1.0, 1.0, 0);     // Yellow
            float px = (x - EL / 2) * GRID_SIZE;
            float py = (y - EL / 2) * GRID_SIZE;
            float pz = (z - EL / 2) * GRID_SIZE;
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
    // Cell spacing.
    const float GRID_SIZE =  0.5 / EL;
    // Size of each lattice point.
    glPointSize(2.0f);

    // Define offsets for 27 cubes (-1, 0, 1 along each axis)
    int offsets[3] = {-1, 0, 1};
    glBegin(GL_POINTS);
    for (int x = 0; x < EL; x++)
    {
      for (int y = 0; y < EL; y++)
      {
        for (int z = 0; z < EL; z++)
        {
          COLORREF color = automaton::voxels[x * L2 + y * EL + z];
          if (!color)
            continue;
          // Extract the R, G, B components.
          BYTE r = GetRValue(color);
          BYTE g = GetGValue(color);
          BYTE b = GetBValue(color);

          // Convert to normalized values between 0.0 and 1.0.
          GLdouble red   = r / 255.0;
          GLdouble green = g / 255.0;
          GLdouble blue  = b / 255.0;
          float alpha = 0.5;
          // Set the OpenGL color.
          glColor4d(red, green, blue, alpha);
          // Base position of the current voxel.
          float px = (x - EL / 2) * GRID_SIZE;
          float py = (y - EL / 2) * GRID_SIZE;
          float pz = (z - EL / 2) * GRID_SIZE;
          if (MULTICUBE_MODE)
          {
            // Render 27 cubes by translating the base grid position.
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
            // Render a single cube.
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
      screenPositions.clear();

      // Get matrices and viewport
      GLint viewport[4];
      GLdouble modelview[16];
      GLdouble projection[16];
      glGetIntegerv(GL_VIEWPORT, viewport);
      glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
      glGetDoublev(GL_PROJECTION_MATRIX, projection);

      const float GRID_SIZE = 0.5f / EL;

      // --- 3D Points ---
      glPointSize(8.0f);
      glBegin(GL_POINTS);
      for (unsigned w = 0; w < W_DIM; w++)
      {
        Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
        float alpha = 0.5f;
        float r = 0.7f + (w & 1) * 0.3f;
        float g = 0.7f + ((w >> 1) & 1) * 0.3f;
        float b = 0.7f + ((w >> 2) & 1) * 0.3f;

        float px = (EL - cell.x[0] - 0.5f) * GRID_SIZE - 0.25f;
        float py = (EL - cell.x[1] - 0.5f) * GRID_SIZE - 0.25f;
        float pz = (EL - cell.x[2] - 0.5f) * GRID_SIZE - 0.25f;

        glColor4f(r, g, b, alpha);
        glVertex3f(px, py, pz);

        // Manual projection
        float sx, sy;
        float obj[3] = {px, py, pz};
        if (framework::projectPoint(obj, modelview, projection, viewport, sx, sy))
        {
          screenPositions.push_back({sx, sy});
        }
      }
      glEnd();
      // --- Count duplicates ---
      std::map<std::pair<int,int>, int> counts;
      for (auto &pos : screenPositions)
      {
        int x = static_cast<int>(pos[0] + 0.5f);
        int y = static_cast<int>(pos[1] + 0.5f);
        counts[{x, y}]++;
      }
  }

  /*
   * Renders 2D active controls of the GUI.
   */
  void GUIrenderer::renderGadgets()
  {
    glDisable(GL_DEPTH_TEST);
    setOrthographicProjection();
    glPointSize(1);
    glPushMatrix();
    // Properties selection
    for (Tickbox& checkbox : checkboxes)
    {
      checkbox.draw();
    }
    // Layer list
    list.render();
    // Displayed data set
    for (Tickbox& checkbox : delays)
    {
      checkbox.draw();
    }
    // Camera viewpoint
    for (Radio& radio : viewpoint)
    {
      radio.draw();
    }
    glFlush();
    glPopMatrix();
    resetPerspectiveProjection();
  }

  /*
   * Renders 2D and 3D objects controlled by the mouse and keyboard.
   */
  void GUIrenderer::renderObjects()
  {
    glEnable(GL_DEPTH_TEST);
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(mProjection));
    if (mCamera)
    {
      glPushMatrix();
      glMultMatrixf(mCamera->getMatrixFlat());
    }
    if (checkboxes[0].getState())
      renderWavefront();
    if (checkboxes[1].getState())
      renderMomentum();
    if (checkboxes[2].getState())
      renderSpin();
    if (checkboxes[3].getState())
      renderSineMask();
    if (checkboxes[4].getState())
      renderHunting();
    if (checkboxes[5].getState())
      renderCenters();
    if (checkboxes[6].getState())
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode.
      renderCube();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore fill mode.
    }
    // Render the axes
    if (checkboxes[7].getState())
      renderAxes();
    // Render plane
    if (checkboxes[8].getState())
      renderPlane();
    //
    if (mCamera)
    {
      glPopMatrix();
    }
    glPopMatrix();
    // Render 2D active objects
    renderGadgets();
  }

/*
 * Renders the cartesian positive axes with labels.
 */
void GUIrenderer::renderAxes()
{
  // Render the axes
  glLineWidth(2);
  glBegin(GL_LINES);

  // X-axis
  glColor3f(0.6f, 0.f, 0.f); // Red for the X-axis
  glVertex3f(0.0f, 0.f, 0.f);
  glVertex3f(0.45f, 0.f, 0.f);

  // Y-axis
  glColor3f(0.f, 0.6f, 0.f); // Green for the Y-axis
  glVertex3f(0.f, 0.f, 0.f);
  glVertex3f(0.f, 0.45f, 0.f);

  // Z-axis
  glColor3f(0.3f, 0.3f, 0.8f); // Blue for the Z-axis
  glVertex3f(0.0f, 0.f, 0.f);
  glVertex3f(0.f, 0.f, 0.45f);

  glEnd();
  glLineWidth(1);
  // Add labels
  glColor3f(1.f, 0.f, 0.f); // Red for "x" label
  render3Dstring(0.453f, -0.02f, 0.0f, GLUT_BITMAP_HELVETICA_18, "x");
  glColor3f(0.f, 1.f, 0.f); // Green for "y" label
  render3Dstring(-0.02f, 0.453f, 0.0f, GLUT_BITMAP_HELVETICA_18, "y");
  glColor3f(0.3f, 0.3f, 0.8f); // Blue for "z" label
  render3Dstring(0.0f, -0.02f, 0.453f, GLUT_BITMAP_HELVETICA_18, "z");
}

/*
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
/*
 * Renders a reference plane to help in the visualization.
 */
void GUIrenderer::renderPlane()
{
  float p, d = .1f, mn = -1.f, mx = 1.f, eps = -1e-4f;
  int i, n = 20;
  glLineWidth(1);
  glBegin(GL_LINES);
  glColor4f(1.f, 1.f, 1.f, .2f); // uniform color for all lines
  for (i = 0; i <= n; ++i)
  {
    p = mn + i * d;
    // Lines parallel to the x-axis (constant z)
    glVertex3f(p, eps, mn);
    glVertex3f(p, eps, mx);
    // Lines parallel to the z-axis (constant x)
    glVertex3f(mn, eps, p);
    glVertex3f(mx, eps, p);
  }
  glEnd();
}

  /*
   * Resizes the GUI.
   */
  void GUIrenderer::resize(int width, int height)
  {
    if (height == 0) height = 1;
    GLfloat ratio = width / (GLfloat) height;
    glViewport(0, 0, width, height);
    // Update global viewport
    gViewport[0] = 0;
    gViewport[1] = 0;
    gViewport[2] = width;
    gViewport[3] = height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    mProjection = glm::perspective(glm::radians(45.0f), ratio, .01f, 100.f);
  }

  /*
   * Used to enhance the particle belonging the current layer.
   */
  void GUIrenderer::enhanceVoxel()
  {
    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][list.getSelected()];
    float cx = (EL - cell.x[0] - 0.5f) * GRID_SIZE - 0.25f;
    float cy = (EL - cell.x[1] - 0.5f) * GRID_SIZE - 0.25f;
    float cz = (EL - cell.x[2] - 0.5f) * GRID_SIZE - 0.25f;
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor3d(0.7, 0.7, 0.7);
    // Generate a random pattern around the center
    const int POINT_COUNT = 12; // Number of random points
    const float MAX_OFFSET = 0.005f; // Maximum offset for random points
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
