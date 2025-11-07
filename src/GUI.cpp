/*
 * GUI.cpp
 *
 * Implements the rendering routines.
 */

#include <sstream>
#include <cawindow.h>
#include <GUI.h>
#include <text.h>
#include "logo.h"
#include "layers.h"
#include "hslider.h"
#include "vslider.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  extern unsigned L2;
  extern int scenario;
}

namespace splash
{
  extern std::vector<std::string> scenarioOptions;
}

namespace framework
{
  using namespace std;

  extern  Logo *logo;

  #ifdef DEBUG
  extern bool showDebugClick;
  extern double debugClickX;
  extern double debugClickY;
  #endif

  extern unsigned long long timer;
  extern bool pause;
  extern void setOrthographicProjection();
  extern void resetPerspectiveProjection();
  extern bool recordFrames;
  extern bool showExitDialog;

  // Global viewport info
  GLint gViewport[4] = {0, 0, 1920, 1080}; // default values, will be overwritten on resize

  // Gadgets
  vector<Tickbox> data3D;
  vector<Tickbox> delays;
  vector<Radio> views;
  vector<Radio> projection;
  std::unique_ptr<LayerList> list; // Global definition as a smart pointer
  HSlider hslider(0, 0, 0, 0, 0);
  VSlider vslider(1890, 93, 10.0f, 607.0f, 30.0f);
  Tickbox *tomo = new Tickbox(50, 840, "Enable");
  vector<Radio> tomoDirs;
  ProgressBar *progress = nullptr;

  // Static text
  extern string ui_help[11];
  extern string record_help[11];
  bool showHelp = true;
  string steps[3] =
  {
    "Convolution",
    "Diffusion",
    "Relocation"
  };
  // Auxiliary
  unsigned long tbegin;
  int barWidths[3];
  const float GRID_SIZE = 0.5 / EL;
  // Global flag to control rendering mode: single cube or 27 cubes.
  bool MULTICUBE_MODE = false;
  bool helpHover = false;
  unsigned tomo_x, tomo_y, tomo_z;

  extern std::vector<std::string> scenarioHelpTexts;

  bool showScenarioHelp = false;
  Tickbox* scenarioHelpToggle = nullptr;

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
    assert(L3 > 0);
    voxels = (COLORREF*) malloc(L3 * sizeof(COLORREF));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int screenWidth = gViewport[2];
    int screenHeight = gViewport[3];
    float sliderWidth = 400.0f;
    float sliderHeight = 20.0f;
    float thumbWidth = 30.0f;
    float x = (screenWidth - sliderWidth) / 2.0f;
    float y = screenHeight - 80.0f;
    hslider = framework::HSlider(x, y, sliderWidth, sliderHeight, thumbWidth);
    hslider.setThumbPosition(0.5f);
    progress = new ProgressBar(screenWidth);
    tbegin = GetTickCount64();
    lastPositions.resize(W_USED);
    list = std::make_unique<LayerList>(W_USED); // W_DIM is now the constructor argument
    //
    tomo->onToggle = [](bool state)
    {
      std::cout << "Tomography mode: " << (state ? "Enabled" : "Disabled") << std::endl;
    };
    //
    data3D.push_back(Tickbox(50, 100, "Wavefront")); // 0
    data3D.back().onToggle = [](bool state) {
        std::cout << "Wavefront toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 130, "Momentum"));  // 1
    data3D.back().onToggle = [](bool state) {
        std::cout << "Momentum toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 160, "Spin"));      // 2
    data3D.back().onToggle = [](bool state) {
        std::cout << "Spin toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 190, "Sine mask")); // 3
    data3D.back().onToggle = [](bool state) {
        std::cout << "Sine mask toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 220, "Hunting"));   // 4
    data3D.back().onToggle = [](bool state) {
        std::cout << "Hunting toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 250, "Centers"));   // 5
    data3D.back().onToggle = [](bool state) {
        std::cout << "Centers toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 280, "Lattice"));   // 6
    data3D.back().onToggle = [](bool state) {
        std::cout << "Lattice toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 310, "Axes"));      // 7
    data3D.back().onToggle = [](bool state) {
        std::cout << "Axes toggled: " << (state ? "ON" : "OFF") << std::endl;
    };

    data3D.push_back(Tickbox(50, 350, "Plane"));     // 8
    data3D.back().onToggle = [](bool state) {
        std::cout << "Plane toggled: " << (state ? "ON" : "OFF") << std::endl;
    };
    data3D[0].setState(true);
    data3D[5].setState(true);
    data3D[7].setState(true);
    //  checkboxes[8].setState(true);
    //
    delays.push_back(Tickbox(50, 420, "Convolution"));
    delays.push_back(Tickbox(50, 450, "Diffusion"));
    delays.push_back(Tickbox(50, 480, "Relocation"));
    //
    for (Tickbox& box : delays)
    {
      box.onToggle = [&box](bool) {
        framework::CAWindow::instance().onDelayToggled(&box);
      };
    }
    //
    views.push_back(Radio(60, 570, "Isometric"));
    views.push_back(Radio(60, 600, "XY"));
    views.push_back(Radio(60, 630, "YZ"));
    views.push_back(Radio(60, 660, "ZX"));
    views[0].setSelected(true);
    //
    projection.push_back(Radio(60, 740, "Ortho"));
    projection.push_back(Radio(60, 770, "Perspective"));
    projection[1].setSelected(true);
    tomoDirs.clear();
    tomoDirs.push_back(Radio(80, 875, "XY"));
    tomoDirs.push_back(Radio(80, 905, "YZ"));
    tomoDirs.push_back(Radio(80, 935, "ZX"));
    // Initialize progress bar data
    int barWidth = gViewport[2] / 4; // Bar is 1/4 of the screen width
    double totalRatio = (double) FRAME;
    barWidths[0] = (int)(barWidth * (double) CONVOL / totalRatio);
    barWidths[1] = (int)(barWidth * (double) (DIFFUSION - CONVOL) / totalRatio);
    barWidths[2] = (int)(barWidth * (double) (RELOC - DIFFUSION) / totalRatio);
    //
    scenarioHelpToggle = new Tickbox(230, 65, "Scenario Help");
    scenarioHelpToggle->setState(true); // default: visible
    scenarioHelpToggle->onToggle = [](bool state)
   	{
      showScenarioHelp = state;
    };
  }

  /**
   * Renders the GUI.
   */
  void GUIrenderer::render()
  {
    renderClear();
    renderObjects();
    renderUI();
    #ifdef DEBUG
    if (showDebugClick)
    {
      glColor3f(1.0f, 0.0f, 0.0f);
      glPointSize(6.0f);
      glBegin(GL_POINTS);
      glVertex2f(debugClickX, debugClickY);
      glEnd();
    }
    #endif
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
    int textX = viewportWidth / 4;
    int textY = viewportHeight / 3;
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
    glOrtho(0, gViewport[2], 0, gViewport[3], -1, 1);
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
   * Renders momentum line.
   */
  void GUIrenderer::renderMomentum()
  {
    const float GRID_SIZE = 0.5f / EL;
    glPointSize(4.0f);        // Larger points for visibility
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (getCell(lattice_curr, x, y, z, list->getSelected()).pB)
          {
            if (isVoxelVisible(x, y, z))
            {
              glColor3f(1.0f, 1.0f, 0.0f);  // Yellow
            }
            else
            {
              glColor3d(0.4, 0.4, 0.4);
            }
            float px = (x - EL / 2.0) * GRID_SIZE;
            float py = (y - EL / 2.0) * GRID_SIZE;
            float pz = (z - EL / 2.0) * GRID_SIZE;
            glVertex3f(px, py, pz);
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
    const float GRID_SIZE = 0.5f / EL;  // Better spacing
    glPointSize(4.0f);        // Larger points for visibility
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (getCell(lattice_curr, x, y, z, list->getSelected()).sB)
          {
            if (isVoxelVisible(x, y, z))
              glColor3d(0, 1.0, 1.0);   // Cyan
            else
              glColor3d(0.4, 0.4, 0.4);
            float px = (x - EL / 2.0) * GRID_SIZE;
            float py = (y - EL / 2.0) * GRID_SIZE;
            float pz = (z - EL / 2.0) * GRID_SIZE;
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
    const float GRID_SIZE = 0.5f / EL;  // Better spacing
    glPointSize(1.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (!isVoxelVisible(x, y, z))
            continue;
          if (getCell(lattice_curr, x, y, z, list->getSelected()).phiB)
          {
            glColor3d(1.0, 1.0, 0);     // Yellow
            float px = (x - EL / 2.0) * GRID_SIZE;
            float py = (y - EL / 2.0) * GRID_SIZE;
            float pz = (z - EL / 2.0) * GRID_SIZE;
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
    const float GRID_SIZE = 0.5f / EL;  // Better spacing
    glPointSize(5.0f);                  // Larger points for visibility
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (getCell(lattice_curr, x, y, z, list->getSelected()).hB)
          {
            glColor3d(1.0, 1.0, 0);     // Yellow
            float px = (x - EL / 2.0) * GRID_SIZE;
            float py = (y - EL / 2.0) * GRID_SIZE;
            float pz = (z - EL / 2.0) * GRID_SIZE;
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
    glPointSize(2.5f);

    // Define offsets for 27 cubes (-1, 0, 1 along each axis)
    int offsets[3] = {-1, 0, 1};
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
          float px = (x - EL / 2.0) * GRID_SIZE;
          float py = (y - EL / 2.0) * GRID_SIZE;
          float pz = (z - EL / 2.0) * GRID_SIZE;
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
    GLdouble modelview[16];
    GLdouble projection[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    const float GRID_SIZE = 0.5f / EL;
    // --- 3D Points ---
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    for (unsigned w = 0; w < W_USED; w++)
    {
      Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
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
      if (framework::projectPoint(obj, modelview, projection, gViewport, sx, sy))
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


  /**
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
    if (data3D[6].getState())
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode.
      renderCube();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore fill mode.
    }
    // Render the axes
    if (data3D[7].getState())
      renderAxes();
    // Render plane
    if (data3D[8].getState())
      renderPlane();
    if (tomo && tomo->getState())
      renderTomoPlane();
    //
    if (mCamera)
    {
      glPopMatrix();
    }
    glPopMatrix();
  }

  /**
   * Renders the cartesian positive axes with labels.
   */
  void GUIrenderer::renderAxes()
  {
    const float fullSize = 0.5f;              // Cube spans from -0.25 to +0.25
    const float axisLength = fullSize * 0.75f; // ¾ of the cube
    const float labelOffset = 0.02f;          // Small offset for label placement

    glLineWidth(2);
    glBegin(GL_LINES);
    // X-axis
    glColor3f(0.6f, 0.f, 0.f); // Red
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(axisLength, 0.f, 0.f);
    // Y-axis
    glColor3f(0.f, 0.6f, 0.f); // Green
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, axisLength, 0.f);
    // Z-axis
    glColor3f(0.3f, 0.3f, 0.8f); // Blue
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, axisLength);
    glEnd();
    glLineWidth(1);

    // Axis labels
    glColor3f(1.f, 0.f, 0.f); // Red for "x"
    render3Dstring(axisLength + labelOffset, -labelOffset, 0.0f, GLUT_BITMAP_HELVETICA_18, "x");

    glColor3f(0.f, 1.f, 0.f); // Green for "y"
    render3Dstring(-labelOffset, axisLength + labelOffset, 0.0f, GLUT_BITMAP_HELVETICA_18, "y");

    glColor3f(0.3f, 0.3f, 0.8f); // Blue for "z"
    render3Dstring(0.0f, -labelOffset, axisLength + labelOffset, GLUT_BITMAP_HELVETICA_18, "z");
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
    const float eps = -1e-4f;  // Slight offset to avoid z-fighting

    glLineWidth(1.0f);
    glColor4f(1.f, 1.f, 1.f, 0.2f);  // Light white lines with transparency

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

    // Check which projection mode is selected
    if (projection[0].isSelected())
    {
      // Orthographic
      float orthoSize = 0.6f;
      mProjection = glm::ortho(
        -orthoSize * ratio, orthoSize * ratio,
        -orthoSize, orthoSize,
        0.01f, 100.0f
      );
    }
    else
    {
      // Perspective
      mProjection = glm::perspective(glm::radians(45.0f), ratio, .01f, 100.f);
    }
  }


  /*
   * Used to enhance the particle belonging the current layer.
   */
  void GUIrenderer::enhanceVoxel()
  {
    Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, list->getSelected());
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

  /**
   * Renders a help hyperlink at the bottom of the screen.
   */
  void GUIrenderer::renderHyperlink()
  {
    const char* linkText = "Help";

    // Use brighter color if hovered
    if (helpHover)
      glColor3f(0.3f, 0.6f, 1.0f);  // lighter blue
    else
      glColor3f(1.0f, 0.0f, 1.0f);  // normal blue
    // Calculate text width
    int textWidth = 0;
    const char* c = linkText;
    while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
    // Position at bottom center
    int x = (gViewport[2] - textWidth) / 2;
    int y = gViewport[3] - 30;  // 30 pixels from bottom

    // Draw text
    glRasterPos2i(x, y);
    c = linkText;
    while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c++);
    // Draw underline
    glLineWidth(1.0f);
    glBegin(GL_LINES);
      glVertex2i(x, y + 4);
      glVertex2i(x + textWidth, y + 4);
    glEnd();
  }

  void GUIrenderer::renderUI()
  {
    glDisable(GL_DEPTH_TEST);
    setOrthographicProjection();
    glPushMatrix();
    glLoadIdentity();
    renderElapsedTime();
    drawPanel(35, 65, 170, gViewport[3] - 150);
    drawPanel(gViewport[2] - 260, 65, 250, gViewport[3] - 150);
    renderSimulationStats();
    renderLayerInfo();
    renderHelpText();
    renderSliders();
    renderTomoControls();
    renderPauseOverlay();
    renderSectionLabels();
    renderCheckboxes();
    renderDelays();
    renderViewpointRadios();
    renderProjectionRadios();
    renderTomoRadios();
    renderHyperlink();
    unsigned long long displayTimer = replayFrames ? replayTimer : timer;
    progress->update(displayTimer);
    progress->render();
    if (showScenarioHelp)
      renderScenarioHelpPane();


    if (showExitDialog)
    {
      const int boxWidth = 400;
      const int boxHeight = 120;
      int boxLeft = (gViewport[2] - boxWidth) / 2;
      int boxBottom = (gViewport[3] - boxHeight) / 2;

      // Background
      glColor3f(0.1f, 0.1f, 0.1f);
      glBegin(GL_QUADS);
        glVertex2i(boxLeft, boxBottom);
        glVertex2i(boxLeft + boxWidth, boxBottom);
        glVertex2i(boxLeft + boxWidth, boxBottom + boxHeight);
        glVertex2i(boxLeft, boxBottom + boxHeight);
      glEnd();

      // Border
      glColor3f(1.0f, 1.0f, 1.0f);
      glLineWidth(2);
      glBegin(GL_LINE_LOOP);
        glVertex2i(boxLeft, boxBottom);
        glVertex2i(boxLeft + boxWidth, boxBottom);
        glVertex2i(boxLeft + boxWidth, boxBottom + boxHeight);
        glVertex2i(boxLeft, boxBottom + boxHeight);
      glEnd();

      // Text
      glColor3f(1.0f, 1.0f, 0.0f);
      drawString8("Are you sure you want to exit?", boxLeft + 40, boxBottom + 80);
      drawString8("[Y] Yes     [N] No", boxLeft + 40, boxBottom + 40);
    }




    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    int x = gViewport[2] - 240;
    int y = gViewport[3] - 325;
    int w = logo->width()*0.21;
    int h = logo->height()*0.21;
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
    logo->draw(gViewport[2] - 240, gViewport[3] - 325, 0.21);
    #ifdef DEBUG
    if (showDebugClick)
    {
      glColor3f(1.0f, 0.0f, 0.0f);
      glPointSize(6.0f);
      glBegin(GL_POINTS);
      glVertex2f(debugClickX, debugClickY);
      glEnd();
    }
    #endif
    scenarioHelpToggle->draw();

    if (recordFrames)
    {
      static bool blink = true;
      static double lastToggle = glfwGetTime();
      double now = glfwGetTime();
      if (now - lastToggle > 0.5) {  // Toggle every 0.5 seconds
        blink = !blink;
        lastToggle = now;
      }
      if (blink)
      {
    	int ypos = gViewport[3] - 115;
        // Red background box at (230, 620) to (330, 650)
        glColor3f(1.0f, 0.0f, 0.0f);  // Red
        glRectf(230.0f, ypos, 326.0f, ypos + 30);  // (left, bottom, right, top)

        // White "RECORD" text centered inside the box
        glColor3f(1.0f, 1.0f, 1.0f);  // White
        drawString8("RECORD F5", 240, ypos + 20);  // Adjusted to fit inside the box
      }
    }
    if (replayFrames)
    {
      static bool blinkReplay = true;
      static double lastReplayToggle = glfwGetTime();
      double now = glfwGetTime();
      if (now - lastReplayToggle > 0.5) {
        blinkReplay = !blinkReplay;
        lastReplayToggle = now;
      }
      if (blinkReplay)
      {
        int ypos = gViewport[3] - 155;  // Slightly above RECORD box
        glColor3f(0.0f, 0.4f, 1.0f);    // Blue
        glRectf(230.0f, ypos, 326.0f, ypos + 30);
        glColor3f(1.0f, 1.0f, 1.0f);    // White
        drawString8("REPLAY F6", 240, ypos + 20);
      }
    }
    glPopMatrix();
    list->update();
    list->render();
    resetPerspectiveProjection();
    glEnable(GL_DEPTH_TEST);  // 🔹 Re-enable depth testing for 3D rendering
    if (data3D[5].getState())
      renderCounts();
  }

  void GUIrenderer::renderElapsedTime()
  {
    unsigned long millis = GetTickCount64() - tbegin;
    char s[64];
    sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString8(s, 50, gViewport[3] - 40);
  }

  void GUIrenderer::renderSimulationStats()
  {
    unsigned long long displayTimer = replayFrames ? replayTimer : timer;
    char s[64];
    sprintf(s, "Light: %llu   Tick: %llu", displayTimer / automaton::FRAME, displayTimer);
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderLayerInfo()
  {
    char s[64];
    sprintf(s, "L = %u  W = %u", EL, W_USED);
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(1700, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
    int w = framework::list->getSelected();
    sprintf(s, "(Current layer = %u)", w);
    render2Dstring(1730, 88, GLUT_BITMAP_HELVETICA_12, s);
    sprintf(s, "%s", splash::scenarioOptions[scenario].c_str());
    render2Dstring(230, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderHelpText()
  {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (showHelp)
    {
      for (int i = 0; i < 11; ++i)
        drawString8(ui_help[i], mode->width - 630, 20 * i + mode->height - 250);
      for (int i = 0; i < 4; ++i)
        drawString8(record_help[i], 230, 20 * i + mode->height - 250);
    }
  }

  void GUIrenderer::renderSliders()
  {
    // Draw vertical slider
    vslider.draw();
    // Draw horizontal slider only if tomography is enabled
    if (tomo && tomo->getState())
    {
      hslider.draw();

      // Display current slice index
      int sliceNum = hslider.getSliceIndex(automaton::EL);
      char sliceText[32];
      snprintf(sliceText, sizeof(sliceText), "Slice: %d / %d", sliceNum, automaton::EL - 1);

      // Set text color and render label above the slider
      glColor3f(1.0f, 1.0f, 1.0f);
      render2Dstring(hslider.getX(), hslider.getY() - 20, GLUT_BITMAP_HELVETICA_12, sliceText);
      ////////
      if (tomo && tomo->getState())
      {
        int sliceNum = hslider.getSliceIndex(automaton::EL);

        if (tomoDirs[0].isSelected())      // XY
        {
          tomo_z = sliceNum;
        }
        else if (tomoDirs[1].isSelected()) // YZ
        {
          tomo_x = sliceNum;
        }
        else if (tomoDirs[2].isSelected()) // ZX
        {
          tomo_y = sliceNum;
        }
      }
    }
  }

  void GUIrenderer::renderTomoControls()
  {
    tomo->draw();
  }

  void GUIrenderer::renderPauseOverlay()
  {
    if (pause)
      renderCenterBox(" Paused ");
  }

  void GUIrenderer::renderSectionLabels()
  {
    glColor3f(1.0f, 1.0f, 0.5f);
    drawString12("Data", 50, 85);
    drawString12("Delays", 50, 405);
    drawString12("Views", 50, 551);
    drawString12("Projection", 50, 721);
    drawString12("Tomo", 50, 830);
  }

  void GUIrenderer::renderCheckboxes()
  {
    for (Tickbox& checkbox : data3D)
      checkbox.draw();
  }

  void GUIrenderer::renderDelays()
  {
    for (Tickbox& checkbox : delays)
      checkbox.draw();
  }

  void GUIrenderer::renderViewpointRadios()
  {
    for (Radio& radio : views)
      radio.draw();
  }

  void GUIrenderer::renderProjectionRadios()
  {
    for (Radio& radio : projection)
      radio.draw();
  }

  void GUIrenderer::renderTomoRadios()
  {
    for (Radio& radio : tomoDirs)
      radio.draw();
  }

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
          // Determine which axis is fixed based on selected direction
          bool match = false;
          if (tomoDirs[0].isSelected())      // XY → fix Z
            match = (z == sliceIndex);
          else if (tomoDirs[1].isSelected()) // YZ → fix X
            match = (x == sliceIndex);
          else if (tomoDirs[2].isSelected()) // ZX → fix Y
            match = (y == sliceIndex);
          if (!match) continue;
          COLORREF color = automaton::voxels[x * automaton::L2 + y * EL + z];
          if (!color) continue;
          // Extract RGB and normalize
          BYTE r = GetRValue(color);
          BYTE g = GetGValue(color);
          BYTE b = GetBValue(color);
          glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 0.6f);
          // Compute position
          float px = (x - EL / 2.0f) * GRID_SIZE;
          float py = (y - EL / 2.0f) * GRID_SIZE;
          float pz = (z - EL / 2.0f) * GRID_SIZE;
          glVertex3f(px, py, pz);
        }
      }
    }
    glEnd();
  }

  inline bool GUIrenderer::isVoxelVisible(unsigned x, unsigned y, unsigned z)
  {
    if (!framework::tomo || !framework::tomo->getState())
      return true;

    if (framework::tomoDirs[0].isSelected())      // XY → fix Z
      return z == framework::tomo_z;
    if (framework::tomoDirs[1].isSelected())      // YZ → fix X
      return x == framework::tomo_x;
    if (framework::tomoDirs[2].isSelected())      // ZX → fix Y
      return y == framework::tomo_y;

    return true;
  }

  void GUIrenderer::renderTomoPlane()
  {
    if (!tomo || !tomo->getState()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float GRID_SIZE = 0.5f / EL;
    const float HALF = EL / 2.0f;

    // Compute world-space coordinate of the selected slice
    float coord = (hslider.getSliceIndex(EL) - HALF + 0.5f) * GRID_SIZE;

    // Draw the semitransparent plane
    glColor4f(0.3f, 0.8f, 1.0f, 0.2f);  // Light blue, 20% opacity
    glBegin(GL_QUADS);
    if (tomoDirs[0].isSelected()) // XY → fix Z
    {
      glVertex3f(-0.25f, -0.25f, coord);
      glVertex3f( 0.25f, -0.25f, coord);
      glVertex3f( 0.25f,  0.25f, coord);
      glVertex3f(-0.25f,  0.25f, coord);
    }
    else if (tomoDirs[1].isSelected()) // YZ → fix X
    {
      glVertex3f(coord, -0.25f, -0.25f);
      glVertex3f(coord,  0.25f, -0.25f);
      glVertex3f(coord,  0.25f,  0.25f);
      glVertex3f(coord, -0.25f,  0.25f);
    }
    else if (tomoDirs[2].isSelected()) // ZX → fix Y
    {
      glVertex3f(-0.25f, coord, -0.25f);
      glVertex3f( 0.25f, coord, -0.25f);
      glVertex3f( 0.25f, coord,  0.25f);
      glVertex3f(-0.25f, coord,  0.25f);
    }
    glEnd();

    // Draw a brighter point at the axis-plane intersection
    glPointSize(8.0f);
    glColor4f(0.5f, 1.0f, 1.0f, 0.6f);  // Brighter cyan, 60% opacity
    glBegin(GL_POINTS);
    if (tomoDirs[0].isSelected())      // XY → fix Z
      glVertex3f(0.0f, 0.0f, coord);
    else if (tomoDirs[1].isSelected()) // YZ → fix X
      glVertex3f(coord, 0.0f, 0.0f);
    else if (tomoDirs[2].isSelected()) // ZX → fix Y
      glVertex3f(0.0f, coord, 0.0f);
    glEnd();

    glDisable(GL_BLEND);
  }

  void GUIrenderer::drawPanel(int x, int y, int width, int height)
  {
    // Step 1: Drop shadow (bottom-right)
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);  // Semi-transparent black
    glBegin(GL_QUADS);
      glVertex2i(x + 4, y + 4);
      glVertex2i(x + width + 4, y + 4);
      glVertex2i(x + width + 4, y + height + 4);
      glVertex2i(x + 4, y + height + 4);
    glEnd();

    // Step 2: Panel background
    glColor3f(0.1f, 0.1f, 0.1f);  // Dark gray
    glBegin(GL_QUADS);
      glVertex2i(x, y);
      glVertex2i(x + width, y);
      glVertex2i(x + width, y + height);
      glVertex2i(x, y + height);
    glEnd();

    // Step 3: Bevel highlight (top-left)
    glColor3f(0.35f, 0.35f, 0.35f);  // Light gray
    glBegin(GL_LINES);
      glVertex2i(x, y); glVertex2i(x + width, y);         // Top edge
      glVertex2i(x, y); glVertex2i(x, y + height);        // Left edge
    glEnd();

    // Step 4: Bevel shadow (bottom-right)
    glColor3f(0.02f, 0.02f, 0.02f);  // Almost black
    glBegin(GL_LINES);
      glVertex2i(x + width, y); glVertex2i(x + width, y + height);   // Right edge
      glVertex2i(x, y + height); glVertex2i(x + width, y + height);  // Bottom edge
    glEnd();
  }

  void GUIrenderer::renderScenarioHelpPane()
  {
    if (scenario < 0 || scenario >= (int)scenarioHelpTexts.size())
      return;
    const int paneX = 230;
    const int paneY = 90;
    const int paneWidth = 400;
    const int paneHeight = 450;
    // Use the same panel style as other UI panes
    drawPanel(paneX, paneY, paneWidth, paneHeight);
    glColor3f(1.0f, 1.0f, 1.0f);
    // Render each line of the help text separately
    std::istringstream iss(scenarioHelpTexts[scenario]);
    std::string line;
    int lineY = paneY + 25;
    while (std::getline(iss, line))
    {
      drawString12(line.c_str(), paneX + 20, lineY);
      lineY += 20;
    }
  }

  void GUIrenderer::clearVoxels()
  {
    for (unsigned i = 0; i < EL * EL * EL; ++i)
      voxels[i] = RGB(0, 0, 0);  // Black background
  }

}
