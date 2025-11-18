/*
 * GUI_2D.cpp
 */

#include <sstream>
#include "GUI.h"
#include <GL/freeglut.h>
#include "text.h"
#include "layers.h"
#include "progress.h"
#include "replay_progress.h"
#include "logo.h"
#include "hslider.h"

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
  extern int scenario;
}

namespace splash
{
  extern std::vector<std::string> scenarioOptions;
}

namespace framework
{
  using namespace std;
  using namespace automaton;
  extern GLint gViewport[4];
  extern ReplayProgressBar *replayProgress;
  extern bool showScenarioHelp;
  extern Tickbox* scenarioHelpToggle;
  extern  Logo *logo;
  extern bool recordFrames;
  extern unsigned long tbegin;
  extern bool showHelp;
  extern string record_help[11];
  extern string ui_help[11];
  extern std::vector<std::string> scenarioHelpTexts;
  extern HSlider hslider;
  extern Tickbox *tomo;

  bool showHelp = true;

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
    // Switch to 2D orthographic top-left origin
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Cast to integers to avoid floating-point precision issues
    int width = (int)gViewport[2];
    int height = (int)gViewport[3];
    glOrtho(0.0, (double)width, (double)height, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // --- DRAW GADGETS ---

    // Draw real elapsed time
    renderElapsedTime();

    // Draw left and right panels
    int leftPanelHeight  = gViewport[3] - 170;
    int rightPanelHeight = gViewport[3] - 170;
    drawPanel(35, 60, 170, leftPanelHeight);
    drawPanel(gViewport[2] - 260, 60, 250, rightPanelHeight);

    // Render gadgets in the panels

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
    logo->draw(gViewport[2] - 240, gViewport[3] - 340, 0.21f);

    // Draw the simulation progress bar
    if (GUImode == SIMULATION)
    {
      if (scenario >= 0)
      {
        progress->update(timer);
        progress->render();
      }
    }
    // Draw the replay progress bar
    else if (GUImode == REPLAY)
    {
      replayProgress->render();
    }

    // Draw the scenario help
    if (scenario >= 0)
      scenarioHelpToggle->draw();
    if (showScenarioHelp)
      renderScenarioHelpPane();

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

    // --- DRAW THE MENU BAR ---

    // Draw menu bar background at the top and menus (menus should be on top)
    renderMenuBar();

    // Restore depth writes and optionally depth test to previous state
    glDepthMask(GL_TRUE);
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    // Pop modelview for UI
    glPopMatrix();

    // After 2D UI, update and render 3D overlay widgets (layer list)
    if (scenario >= 0)
    {
      layerList->update();
      layerList->render();
    }

    // Restore original projection
    resetPerspectiveProjection();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
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
      render2Dstring(900, 80, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderLayerInfo()
  {
    if (scenario < 0)
      return;
    char s[64];
    sprintf(s, "L = %u  W = %u", EL, W_USED);
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(1700, 80, GLUT_BITMAP_TIMES_ROMAN_24, s);
    int w = framework::layerList->getSelected();
    sprintf(s, "(Current layer = %u)", w);
    render2Dstring(1730, 120, GLUT_BITMAP_HELVETICA_12, s);
    if (scenario >= 0)
    {
      sprintf(s, "%s", splash::scenarioOptions[scenario].c_str());
      render2Dstring(230, 80, GLUT_BITMAP_TIMES_ROMAN_24, s);
    }
    sprintf(s, "Mode: %s", GUImode == REPLAY ? "Replay" : "Simulation");
    glColor3f(1.0f, 1.0f, 1.0f);
    render2Dstring(1400, 80, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }

  void GUIrenderer::renderHelpText()
  {
    if (showHelp)
    {
      int rightX = gViewport[2] - 630;
      int baseY = gViewport[3] - 250;
      int leftX = 230;
      glColor3f(0.6f, 0.6f, 0.6f);
      for (int i = 0; i < 11; ++i)
        drawString(ui_help[i], rightX, baseY + (20 * i), 8);
      for (int i = 0; i < 4; ++i)
        drawString(record_help[i], leftX, baseY + (20 * i), 8);
    }
  }

  void GUIrenderer::drawPanel(int x, int y, int width, int height)
  {
      // Shadow (unchanged)
      glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
      glBegin(GL_QUADS);
        glVertex2i(x + 4, y + 4);
        glVertex2i(x + width + 4, y + 4);
        glVertex2i(x + width + 4, y + height + 4);
        glVertex2i(x + 4, y + height + 4);
      glEnd();

      // 🔥 Brighter panel background
      glColor3f(0.18f, 0.18f, 0.18f);
      glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x + width, y);
        glVertex2i(x + width, y + height);
        glVertex2i(x, y + height);
      glEnd();

      // Lighter top/left border
      glColor3f(0.45f, 0.45f, 0.45f);
      glBegin(GL_LINES);
        glVertex2i(x, y); glVertex2i(x + width, y);
        glVertex2i(x, y); glVertex2i(x, y + height);
      glEnd();

      // Darker bottom/right border
      glColor3f(0.08f, 0.08f, 0.08f);
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
    const int paneY = 150;
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

  void GUIrenderer::renderSliders()
  {
	if (tomo->getState())
      hslider.draw();
    vslider.draw();
  }

  void GUIrenderer::renderPauseOverlay()
  {
      extern bool pause;
      if (!pause) return;

      // Dim screen overlay
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);

      glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
      glBegin(GL_QUADS);
          glVertex2f(-1.0f, -1.0f);
          glVertex2f( 1.0f, -1.0f);
          glVertex2f( 1.0f,  1.0f);
          glVertex2f(-1.0f,  1.0f);
      glEnd();

      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
      glPopMatrix();

      // ✅ ADD THIS: Render the "PAUSED" text box
      renderCenterBox("PAUSED");
  }

  void GUIrenderer::renderSectionLabels()
  {
      // Set up for 2D text rendering
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glDisable(GL_DEPTH_TEST);

      glColor3f(0.8f, 0.8f, 1.0f);
      render2Dstring(50, 90, GLUT_BITMAP_9_BY_15, "Data 3D");
      render2Dstring(50, 410, GLUT_BITMAP_9_BY_15, "Delays");
      render2Dstring(50, 555, GLUT_BITMAP_9_BY_15, "View");
      render2Dstring(50, 725, GLUT_BITMAP_9_BY_15, "Projection");
      render2Dstring(50, 830, GLUT_BITMAP_9_BY_15, "Tomography");

      glEnable(GL_DEPTH_TEST);
      glPopMatrix();
  }

  void GUIrenderer::renderMenuBar()
  {
      // Draw background bar across the top
      drawPanel(0, 0, gViewport[2], 30);

      // Draw labels at fixed pixel positions
      glColor3f(0.9f, 0.9f, 0.9f);
      drawString("File", 10, 20, 8);
      drawString("Help", 120, 20, 8);

      // Draw dropdowns if open
      glDisable(GL_DEPTH_TEST); // ensure menus are visible on top
      if (fileMenu->isOpen_)
          fileMenu->draw(gViewport[2], gViewport[3]);
      if (helpMenu->isOpen_)
          helpMenu->draw(gViewport[2], gViewport[3]);
      glEnable(GL_DEPTH_TEST);
  }

}


