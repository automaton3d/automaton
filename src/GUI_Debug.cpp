/*
 * GUI_Debug.cpp
 */


#include "GUI.h"
#include <GL/freeglut.h>
#include <sstream>

namespace framework {

void GUIrenderer::renderCounts()
{
#ifdef DEBUG
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    std::ostringstream oss;
    oss << "Voxel count: " << lastPositions_.size();

    glColor3f(1.0f, 1.0f, 0.0f);
    render2Dstring(30, 1040, GLUT_BITMAP_9_BY_15, oss.str().c_str());

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
#endif
}

} // namespace framework


#ifdef DEBUG
#include <sstream>
#include <cawindow.h>
#include <GUI.h>
#include <cmath>
#include <GL/freeglut.h>
#include "logo.h"
#include "layers.h"
#include "hslider.h"
#include "vslider.h"
#include "button.h"
#include "text.h"
#include "replay_progress.h"
#include "recorder.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern int GUImode;

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
  extern FrameRecorder recorder;
  extern size_t replayIndex;

  // Global viewport info
  GLint gViewport[4] = {0, 0, 1920, 1080}; // default values, will be overwritten on resize

  // Gadgets
  vector<Tickbox> data3D;
  vector<Tickbox> delays;
  vector<Radio> views;
  vector<Radio> projection;
  std::unique_ptr<LayerList> layerList; // Global definition as a smart pointer
  HSlider hslider(0, 0, 0, 0, 0);
  VSlider vslider(1890, 93, 10.0f, 607.0f, 30.0f);
  Tickbox *tomo = new Tickbox(50, 840, "Enable");
  vector<Radio> tomoDirs;
  ProgressBar *progress = nullptr;
  ReplayProgressBar *replayProgress = nullptr;

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
    replayProgress = new ReplayProgressBar((int)screenWidth, (int)screenHeight);
    tbegin = GetTickCount64();
    layerList = std::make_unique<LayerList>(W_USED); // W_DIM is now the constructor argument
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
    drawString(text, textX, textY, 8);
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
    for (auto &pos : screenPositions_)
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
    const int CENTER_INT = EL / 2;
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (unsigned x = 0; x < EL; x++)
    {
      for (unsigned y = 0; y < EL; y++)
      {
        for (unsigned z = 0; z < EL; z++)
        {
          if (getCell(lattice_curr, x, y, z, layerList->getSelected()).pB)
          {
            if (isVoxelVisible(x, y, z))
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
              float px = (x - cx) * GRID_SIZE;
              float py = (y - cy) * GRID_SIZE;
              float pz = (z - cz) * GRID_SIZE;
              glVertex3f(px, py, pz);
            }
            else
            {
              // Normal rendering mode (centered on global lattice center)
              float px = ((int)x - CENTER_INT) * GRID_SIZE;
              float py = ((int)y - CENTER_INT) * GRID_SIZE;
              float pz = ((int)z - CENTER_INT) * GRID_SIZE;
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
          if (getCell(lattice_curr, x, y, z, layerList->getSelected()).sB)
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
              float px = (x - cx) * GRID_SIZE;
              float py = (y - cy) * GRID_SIZE;
              float pz = (z - cz) * GRID_SIZE;
              glVertex3f(px, py, pz);
            }
            else
            {
              float px = ((int)x - CENTER_INT) * GRID_SIZE;
              float py = ((int)y - CENTER_INT) * GRID_SIZE;
              float pz = ((int)z - CENTER_INT) * GRID_SIZE;
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
          if (getCell(lattice_curr, x, y, z, layerList->getSelected()).phiB)
          {
            glColor3d(1.0, 1.0, 0);
            float px = ((int)x - CENTER_INT) * GRID_SIZE;
            float py = ((int)y - CENTER_INT) * GRID_SIZE;
            float pz = ((int)z - CENTER_INT) * GRID_SIZE;
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
          if (getCell(lattice_curr, x, y, z, layerList->getSelected()).hB)
          {
            glColor3d(1.0, 1.0, 0);
            float px = ((int)x - CENTER_INT) * GRID_SIZE;
            float py = ((int)y - CENTER_INT) * GRID_SIZE;
            float pz = ((int)z - CENTER_INT) * GRID_SIZE;
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

          float px = ((int)x - CENTER_INT) * GRID_SIZE;
          float py = ((int)y - CENTER_INT) * GRID_SIZE;
          float pz = ((int)z - CENTER_INT) * GRID_SIZE;

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
    GLdouble modelview[16];
    GLdouble projection[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
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

      float px = ((int)cell.x[0] - CENTER_INT) * GRID_SIZE;
      float py = ((int)cell.x[1] - CENTER_INT) * GRID_SIZE;
      float pz = ((int)cell.x[2] - CENTER_INT) * GRID_SIZE;

      glColor4f(r, g, b, alpha);
      glVertex3f(px, py, pz);

      // Manual projection
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
      mProjection_ = glm::ortho(
        -orthoSize * ratio, orthoSize * ratio,
        -orthoSize, orthoSize,
        0.01f, 100.0f
      );
    }
    else
    {
      // Perspective
      mProjection_ = glm::perspective(glm::radians(45.0f), ratio, .01f, 100.f);
    }
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
      cx = (center[0] - EL / 2) * GRID_SIZE;
      cy = (center[1] - EL / 2) * GRID_SIZE;
      cz = (center[2] - EL / 2) * GRID_SIZE;
    }
    else
    {
      // In simulation mode, use the cell position at the static center
      Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
      cx = (cell.x[0] - EL / 2) * GRID_SIZE;
      cy = (cell.x[1] - EL / 2) * GRID_SIZE;
      cz = (cell.x[2] - EL / 2) * GRID_SIZE;
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

  /**
   * Renders a help hyperlink at the bottom of the screen.
   */
  void GUIrenderer::renderHyperlink()
  {
    const char* linkText = "Help";

    if (helpHover)
      glColor3f(0.3f, 0.6f, 1.0f);
    else
      glColor3f(1.0f, 0.0f, 1.0f);

    int textWidth = 0;
    const char* c = linkText;
    while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);

    int x = (gViewport[2] - textWidth) / 2;
    int y = gViewport[3] - 30;

    glRasterPos2i(x, y);
    c = linkText;
    while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c++);

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
    render3Dboxes();
    renderDelays();
    renderViewpointRadios();
    renderProjectionRadios();
    renderTomoRadios();
    renderHyperlink();

    if (GUImode == SIMULATION)
    {
      if (scenario >= 0)
      {
        progress->update(timer);
        progress->render();
      }
    }
    else if (GUImode == REPLAY)
    {
       replayProgress->render();
    }

    if (showScenarioHelp)
      renderScenarioHelpPane();

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

    if (scenario >= 0)
      scenarioHelpToggle->draw();

    if (recordFrames)
    {
      static bool blink = true;
      static double lastToggle = glfwGetTime();
      double now = glfwGetTime();
      if (now - lastToggle > 0.5) {
        blink = !blink;
        lastToggle = now;
      }
      if (blink)
      {
        int ypos = gViewport[3] - 115;
        glColor3f(1.0f, 0.0f, 0.0f);
        glRectf(230.0f, ypos, 326.0f, ypos + 30);

        glColor3f(1.0f, 1.0f, 1.0f);
        drawString("RECORD F5", 240, ypos + 20, 8);
      }
    }
    glPopMatrix();

    if (scenario >= 0)
    {
      layerList->update();
      layerList->render();
    }

    resetPerspectiveProjection();
    glEnable(GL_DEPTH_TEST);

    if (data3D[5].getState())
      renderCounts();
  }

  void GUIrenderer::renderElapsedTime()
  {
    unsigned long millis = GetTickCount64() - tbegin;
    char s[64];
    sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawString(s, 50, gViewport[3] - 40, 8);
  }

  void GUIrenderer::renderSimulationStats()
  {
      char s[64];
      if (GUImode == REPLAY) {
          sprintf(s, "Light: %llu", timer / automaton::FRAME);
      } else {
          sprintf(s, "Light: %llu Tick: %llu", timer / automaton::FRAME, timer);
      }
      glColor3f(1.0f, 1.0f, 1.0f);
      render2Dstring(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderLayerInfo()
  {
    if (scenario < 0)
      return;
    char s[64];
    sprintf(s, "L = %u  W = %u", EL, W_USED);
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(1700, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
    int w = framework::layerList->getSelected();
    sprintf(s, "(Current layer = %u)", w);
    render2Dstring(1730, 88, GLUT_BITMAP_HELVETICA_12, s);
    if (scenario >= 0)
    {
      sprintf(s, "%s", splash::scenarioOptions[scenario].c_str());
      render2Dstring(230, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
    }
    sprintf(s, "Mode = %s", GUImode == REPLAY ? "Replay" : "Simulation");
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(1400, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderHelpText()
  {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (showHelp)
    {
      for (int i = 0; i < 11; ++i)
        drawString(ui_help[i], mode->width - 630, 20 * i + mode->height - 250, 8);
      for (int i = 0; i < 4; ++i)
        drawString(record_help[i], 230, 20 * i + mode->height - 250, 8);
    }
  }

  void GUIrenderer::renderSliders()
  {
    if (scenario >= 0)
      vslider.draw();

    if (tomo && tomo->getState())
    {
      hslider.draw();

      int sliceNum = hslider.getSliceIndex(automaton::EL);
      char sliceText[32];
      snprintf(sliceText, sizeof(sliceText), "Slice: %d / %d", sliceNum, automaton::EL - 1);

      glColor3f(1.0f, 1.0f, 1.0f);
      render2Dstring(hslider.getX(), hslider.getY() - 20, GLUT_BITMAP_HELVETICA_12, sliceText);

      if (tomo && tomo->getState())
      {
        int sliceNum = hslider.getSliceIndex(automaton::EL);

        if (tomoDirs[0].isSelected())
        {
          tomo_z = sliceNum;
        }
        else if (tomoDirs[1].isSelected())
        {
          tomo_x = sliceNum;
        }
        else if (tomoDirs[2].isSelected())
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
    drawString("Data", 50, 85, 12);
    drawString("Delays", 50, 405, 12);
    drawString("Views", 50, 551, 12);
    drawString("Projection", 50, 721, 12);
    drawString("Tomo", 50, 830, 12);
  }

  void GUIrenderer::render3Dboxes()
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

  inline bool GUIrenderer::isVoxelVisible(unsigned x, unsigned y, unsigned z)
  {
    if (!framework::tomo || !framework::tomo->getState())
      return true;

    if (framework::tomoDirs[0].isSelected())
      return z == framework::tomo_z;
    if (framework::tomoDirs[1].isSelected())
      return x == framework::tomo_x;
    if (framework::tomoDirs[2].isSelected())
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

  void GUIrenderer::drawPanel(int x, int y, int width, int height)
  {
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glBegin(GL_QUADS);
      glVertex2i(x + 4, y + 4);
      glVertex2i(x + width + 4, y + 4);
      glVertex2i(x + width + 4, y + height + 4);
      glVertex2i(x + 4, y + height + 4);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
      glVertex2i(x, y);
      glVertex2i(x + width, y);
      glVertex2i(x + width, y + height);
      glVertex2i(x, y + height);
    glEnd();

    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINES);
      glVertex2i(x, y); glVertex2i(x + width, y);
      glVertex2i(x, y); glVertex2i(x, y + height);
    glEnd();

    glColor3f(0.02f, 0.02f, 0.02f);
    glBegin(GL_LINES);
      glVertex2i(x + width, y); glVertex2i(x + width, y + height);
      glVertex2i(x, y + height); glVertex2i(x + width, y + height);
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

    drawPanel(paneX, paneY, paneWidth, paneHeight);
    glColor3f(1.0f, 1.0f, 1.0f);

    std::istringstream iss(scenarioHelpTexts[scenario]);
    std::string line;
    int lineY = paneY + 25;
    while (std::getline(iss, line))
    {
      drawString(line.c_str(), paneX + 20, lineY, 12);
      lineY += 20;
    }
  }

  void GUIrenderer::clearVoxels()
  {
    for (unsigned i = 0; i < EL * EL * EL; ++i)
      voxels[i] = RGB(0, 0, 0);
  }

  void framework::GUIrenderer::drawRoundedRect(float x, float y, float w, float h, float radius)
  {
      const int seg = 20;
      glBegin(GL_POLYGON);

      for (int i = 0; i <= seg; ++i) {
          float a = (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + radius + radius * cosf(a),
                     y + radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = (M_PI / 2.0f) + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + w - radius + radius * cosf(a),
                     y + radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = M_PI + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + w - radius + radius * cosf(a),
                     y + h - radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = 3*M_PI/2.0f + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + radius + radius * cosf(a),
                     y + h - radius + radius * sinf(a));
      }
      glEnd();
  }

  void framework::GUIrenderer::drawRoundedRectOutline(float x, float y, float w, float h, float radius)
  {
      const int seg = 20;
      glBegin(GL_LINE_LOOP);

      for (int i = 0; i <= seg; ++i) {
          float a = (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + radius + radius * cosf(a),
                     y + radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = (M_PI / 2.0f) + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + w - radius + radius * cosf(a),
                     y + radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = M_PI + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + w - radius + radius * cosf(a),
                     y + h - radius + radius * sinf(a));
      }
      for (int i = 0; i <= seg; ++i) {
          float a = 3*M_PI/2.0f + (M_PI / 2.0f) * (float)i / seg;
          glVertex2f(x + radius + radius * cosf(a),
                     y + h - radius + radius * sinf(a));
      }
      glEnd();
  }

}
#endif
