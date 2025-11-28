/*
 * GUI_2D.cpp (modernized, legacy structure preserved)
 */

#include "GUI.h"
#include "globals.h"
#include "layers.h"
#include "progress.h"
#include "replay_progress.h"
#include "logo.h"
#include "hslider.h"
#include "vslider.h"
#include "text_renderer.h"
#include "simulation.h"
#include "color_utils.h"

// Necessário para EL, W_USED, scenario, etc.
#include "model/simulation.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

namespace automaton
{
  extern unsigned EL;
  extern unsigned W_USED;
}

namespace splash
{
  extern std::vector<std::string> scenarioOptions;
}

namespace framework
{
  // Referências a variáveis declaradas em GUI.cpp e globals.h
  extern ReplayProgressBar* replayProgress;
  extern bool showScenarioHelp;
  extern Tickbox* scenarioHelpToggle;
  extern Logo* logo;
  extern bool recordFrames;
  extern double tbegin;
  extern bool showHelp;
  extern std::vector<std::string> scenarioHelpTexts;
  extern HSlider hslider;
  extern VSlider vslider;
  extern Tickbox* tomo;
  extern bool showAboutDialog;
  extern bool helpHover;
  extern int windowWidth;
  extern int windowHeight;
  extern std::unique_ptr<LayerList> layerList;
  extern int GUImode;

  // Help text arrays
  extern const char* ui_help[];
  extern const char* record_help[];

  // ---------------------------------------------------------------------
  // Small helpers for transient 2D drawing (quad/line) with colorProgram2D
  // ---------------------------------------------------------------------
  void drawQuad2D(float x1, float y1, float x2, float y2, const glm::vec3& color)
  {
    if (!colorProgram2D) return;
    glUseProgram(colorProgram2D);
    glm::mat4 ortho = glm::ortho(0.0f, (float)gViewport[2], 0.0f, (float)gViewport[3]);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D, color.r, color.g, color.b);

    float verts[8] = { x1, y1,  x2, y1,  x2, y2,  x1, y2 };
    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  static void drawLine2D(float x1, float y1, float x2, float y2, const glm::vec3& color, float width = 1.0f)
  {
    if (!colorProgram2D) return;
    glUseProgram(colorProgram2D);

    glm::mat4 ortho = glm::ortho(0.0f, (float)gViewport[2], 0.0f, (float)gViewport[3]);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D, color.r, color.g, color.b);

    float verts[4] = { x1, y1, x2, y2 };
    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  /**
   * Opens a pause message box with centered text and an outline.
   */
  void GUIrenderer::renderCenterBox(const char* text)
  {
    if (!textRenderer) return;

    int vw = gViewport[2], vh = gViewport[3];
    int x = vw / 2 - 100;
    int y = vh / 2;

    // Background quad (semi-transparent look via color; alpha is not in this shader)
    drawQuad2D((float)(x - 20), (float)(y - 40),
               (float)(x + 220), (float)(y + 40),
               glm::vec3(0.0f, 0.0f, 0.0f));

    textRenderer->RenderText(text, (float)x, (float)y, 0.8f, glm::vec3(1.0f), vw, vh);
  }

  /**
   * Renders a help hyperlink at the bottom of the screen.
   */
  void GUIrenderer::renderHyperlink()
  {
    if (!textRenderer) return;

    std::string linkText = "Help";
    glm::vec3 color = helpHover ? glm::vec3(0.3f, 0.6f, 1.0f) : glm::vec3(1.0f, 0.0f, 1.0f);
    int w = textRenderer->measureTextWidth(linkText, 0.6f);
    int textWidth = textRenderer->measureTextWidth(linkText, 0.6f);

    int x = (gViewport[2] - textWidth) / 2;   // centraliza horizontalmente
    int y = 30;

    textRenderer->RenderText(linkText, (float)x, (float)y, 0.6f, color, gViewport[2], gViewport[3]);

    // underline
    drawLine2D((float)x, (float)(y + 4), (float)(x + w), (float)(y + 4), color, 1.0f);
  }

void GUIrenderer::renderElapsedTime()
{
    if (!textRenderer) return;

    // real time in seconds (GLFW monotonic clock)
    double now = glfwGetTime();
    double seconds = now - tbegin;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Elapsed %.1f s", seconds);

    textRenderer->RenderText(
        buf,
        50.0f,
        40.0f,
        0.6f,
        glm::vec3(1.0f,1.0f,1.0f),
        gViewport[2], gViewport[3]
    );
}

  void GUIrenderer::renderUI()
  {
    // Modernized: drop matrix stack toggles; manage depth/blend explicitly
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // About dialog
    if (showAboutDialog && textRenderer)
    {
      // Dialog background
      drawQuad2D(200.0f, 150.0f,
                 (float)gViewport[2] - 200.0f, (float)gViewport[3] - 150.0f,
                 glm::vec3(0.0f, 0.0f, 0.0f));

      std::string about =
        "Cellular Automaton Visualizer\n\n"
        "Version 1.0\n\n"
        "Real-time 3D cellular automaton\n"
        "with recording, replay & tomography\n\n"
        "Built with OpenGL + GLFW\n"
        "(c) 2025";

      textRenderer->RenderText(about, 250.0f, 200.0f, 0.6f, glm::vec3(1.0f),
                               gViewport[2], gViewport[3]);
      textRenderer->RenderText("[ Close ]",
                               (float)gViewport[2]/2.0f - 50.0f,
                               (float)gViewport[3] - 180.0f,
                               0.7f, glm::vec3(1.0f, 0.3f, 0.3f),
                               gViewport[2], gViewport[3]);
    }
if (scenarioHelpToggle) {
    scenarioHelpToggle->setFontScale(0.6f);
    scenarioHelpToggle->draw(*textRenderer, gViewport[2], gViewport[3]);
    showScenarioHelp = scenarioHelpToggle->getState();  // <-- sync state
}
    renderElapsedTime();
    
    // Draw left and right panels
    int leftPanelHeight  = gViewport[3] - 170;
    int rightPanelHeight = gViewport[3] - 170;
    drawPanel(35, 60, 170, leftPanelHeight);
    drawPanel(gViewport[2] - 260, 60, 250, rightPanelHeight);

    renderSimulationStats();
    renderLayerInfo();
    renderLayers();
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

    if (logo && logo->valid()) {
      logo->draw(gViewport[2] - 250, gViewport[3] - 350, 1.0f);  // slight tweak for visual balance
    }

    // Now actually render the help pane if toggled
    if (showScenarioHelp) {
      renderScenarioHelpPane();
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
  }

  void GUIrenderer::renderSimulationStats()
  {
    char s[64];
    if (GUImode == REPLAY) {
      std::snprintf(s, sizeof(s), "Light: %llu", timer / automaton::FRAME);
    } else {
      std::snprintf(s, sizeof(s), "Light: %llu Tick: %llu", timer / automaton::FRAME, timer);
    }

    if (textRenderer) {
      textRenderer->RenderText(
        s,
        900.0f, gViewport[3] - 80.0f,
        1.0f,
        glm::vec3(1.0f, 1.0f, 1.0f),
        gViewport[2], gViewport[3]
      );
    }
  }

  void GUIrenderer::renderLayerInfo()
  {
    using namespace automaton;

    if (scenario < 0) return;

    char s[64];
    // L e W
    std::snprintf(s, sizeof(s), "L = %u  W = %u", EL, W_USED);
    if (textRenderer) {
      textRenderer->RenderText(s, 1700.0f, gViewport[3] - 80.0f, 1.0f,
                               glm::vec3(1.0f), gViewport[2], gViewport[3]);
    }

    // Current layer
    if (layerList) {
      int w = layerList->getSelected();
      std::snprintf(s, sizeof(s), "(Current layer = %u)", w);
      if (textRenderer) {
        textRenderer->RenderText(s, 1730.0f, gViewport[3] - 120.0f, 0.5f,
                                 glm::vec3(1.0f), gViewport[2], gViewport[3]);
      }
    }

    // Scenario name
    if (scenario >= 0 && scenario < (int)splash::scenarioOptions.size()) {
      std::snprintf(s, sizeof(s), "%s", splash::scenarioOptions[scenario].c_str());
      if (textRenderer) {
        textRenderer->RenderText(s, 230.0f, gViewport[3] - 80.0f, 1.0f,
                                 glm::vec3(1.0f), gViewport[2], gViewport[3]);
      }
    }

    // Mode
    std::snprintf(s, sizeof(s), "Mode: %s", GUImode == REPLAY ? "Replay" : "Simulation");
    if (textRenderer) {
      textRenderer->RenderText(s, 1400.0f, gViewport[3] - 80.0f, 1.0f,
                               glm::vec3(1.0f), gViewport[2], gViewport[3]);
    }
  }

  void GUIrenderer::renderLayers()
  {
    if (layerList) {
        layerList->render(*textRenderer, gViewport[2], gViewport[3]);
        layerList->update(*textRenderer, gViewport[2], gViewport[3]);
    }
  }
  
  void GUIrenderer::renderHelpText()
  {
    if (showHelp && textRenderer)
    {
      int rightX = gViewport[2] - 630;
      int baseY  = gViewport[3] - 250;
      int leftX  = 230;

      glm::vec3 color(0.6f, 0.6f, 0.6f);

      // UI help
      for (int i = 0; i < 11; ++i) {
        textRenderer->RenderText(
          ui_help[i],
          (float)rightX, (float)(baseY + 20 * i),
          0.5f, color,
          gViewport[2], gViewport[3]
        );
      }

      // Record help
      for (int i = 0; i < 4; ++i) {
        textRenderer->RenderText(
          record_help[i],
          (float)leftX, (float)(baseY + 20 * i),
          0.5f, color,
          gViewport[2], gViewport[3]
        );
      }
    }
  }

  void GUIrenderer::drawPanel(int x, int y, int width, int height)
  {
    // Shadow
    drawQuad2D((float)(x + 4), (float)(y + 4),
               (float)(x + width + 4), (float)(y + height + 4),
               glm::vec3(0.0f, 0.0f, 0.0f)); // simulate alpha via darker color

    // Brighter panel background
    drawQuad2D((float)x, (float)y, (float)(x + width), (float)(y + height),
               glm::vec3(0.18f, 0.18f, 0.18f));

    // Lighter top/left border
    drawLine2D((float)x, (float)y, (float)(x + width), (float)y, glm::vec3(0.45f, 0.45f, 0.45f), 1.0f);
    drawLine2D((float)x, (float)y, (float)x, (float)(y + height), glm::vec3(0.45f, 0.45f, 0.45f), 1.0f);

    // Darker bottom/right border
    drawLine2D((float)(x + width), (float)y, (float)(x + width), (float)(y + height), glm::vec3(0.08f, 0.08f, 0.08f), 1.0f);
    drawLine2D((float)x, (float)(y + height), (float)(x + width), (float)(y + height), glm::vec3(0.08f, 0.08f, 0.08f), 1.0f);
  }

  void GUIrenderer::renderScenarioHelpPane()
  {
    using namespace automaton;

    if (scenario < 0 || scenario >= (int)scenarioHelpTexts.size())
      return;

    const int paneX = 230;
    const int paneY = 150;
    const int paneWidth = 400;
    const int paneHeight = 450;
    drawPanel(paneX, paneY, paneWidth, paneHeight);

    std::istringstream iss(scenarioHelpTexts[scenario]);
    std::string line;
    int lineY = paneY + 25;
    while (std::getline(iss, line))
    {
      if (textRenderer) {
        textRenderer->RenderText(line, (float)(paneX + 20), (float)lineY,
                                 1.2f, glm::vec3(1.0f),
                                 windowWidth, windowHeight);
      }
      lineY += 20;
    }
  }

  void GUIrenderer::clearVoxels()
  {
    for (unsigned i = 0; i < EL * EL * EL; ++i)
      voxels[i] = 0x000000;
  }

  void GUIrenderer::drawRoundedRect(float x, float y, float w, float h, float radius)
  {
    const int seg = 20;
    const float PI = 3.14159265358979323846f;
    std::vector<glm::vec2> pts;
    pts.reserve(seg * 4 + 4);

    // Four corners: TL, TR, BR, BL
    for (int i = 0; i <= seg; ++i) {
      float a = (PI / 2.0f) * (float)i / seg;
      pts.emplace_back(x + radius + radius * cosf(a),
                       y + radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = (PI / 2.0f) + (PI / 2.0f) * (float)i / seg;
      pts.emplace_back(x + w - radius + radius * cosf(a),
                       y + radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = PI + (PI / 2.0f) * (float)i / seg;
      pts.emplace_back(x + w - radius + radius * cosf(a),
                       y + gViewport[3] - radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = 3*PI/2.0f + (PI / 2.0f) * (float)i / seg;
      pts.emplace_back(x + radius + radius * cosf(a),
                       y + gViewport[3] - radius + radius * sinf(a));
    }

    if (!colorProgram2D) return;
    glUseProgram(colorProgram2D);
    glm::mat4 ortho = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D, 0.18f, 0.18f, 0.18f);

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec2), pts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)pts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  void GUIrenderer::drawRoundedRectOutline(float x, float y, float w, float h, float radius)
  {
    const int seg = 20;
    const float PI = 3.14159265358979323846f;
    std::vector<glm::vec2> glmPts;
    glmPts.reserve(seg * 4 + 4);

    for (int i = 0; i <= seg; ++i) {
      float a = (PI / 2.0f) * (float)i / seg;
      glmPts.emplace_back(x + radius + radius * cosf(a),
                          y + radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = (PI / 2.0f) + (PI / 2.0f) * (float)i / seg;
      glmPts.emplace_back(x + w - radius + radius * cosf(a),
                          y + radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = PI + (PI / 2.0f) * (float)i / seg;
      glmPts.emplace_back(x + w - radius + radius * cosf(a),
                          y + gViewport[3] - radius + radius * sinf(a));
    }
    for (int i = 0; i <= seg; ++i) {
      float a = 3*PI/2.0f + (PI / 2.0f) * (float)i / seg;
      glmPts.emplace_back(x + radius + radius * cosf(a),
                          y + gViewport[3] - radius + radius * sinf(a));
    }

    if (!colorProgram2D) return;
    glUseProgram(colorProgram2D);
    glm::mat4 ortho = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D, 0.45f, 0.45f, 0.45f);

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, glmPts.size() * sizeof(glm::vec2), glmPts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)glmPts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  void GUIrenderer::renderSliders()
  {
    if (tomo && tomo->getState())
      hslider.draw(gViewport[2], gViewport[3]);

    vslider.draw(gViewport[2], gViewport[3]);
  }

  void GUIrenderer::renderPauseOverlay()
  {
    if (!pause) return;

    // Full-screen dark overlay
    drawQuad2D(0.0f, 0.0f, (float)gViewport[2], (float)gViewport[3], glm::vec3(0.0f, 0.0f, 0.0f));
    renderCenterBox("PAUSED");
  }

  void GUIrenderer::renderSectionLabels()
  {
    glm::vec3 color(0.8f, 0.8f, 1.0f);

    if (textRenderer) {
      textRenderer->RenderText("Data 3D", 50.0f, gViewport[3] - 140.0f, 0.8f, color, gViewport[2], gViewport[3]);
      textRenderer->RenderText("Delays", 50.0f, gViewport[3] - 460.0f, 0.8f, color, gViewport[2], gViewport[3]);
      textRenderer->RenderText("View", 50.0f, gViewport[3] - 605.0f, 0.8f, color, gViewport[2], gViewport[3]);
      textRenderer->RenderText("Projection", 50.0f, gViewport[3] - 775.0f, 0.8f, color, gViewport[2], gViewport[3]);
      textRenderer->RenderText("Tomography", 50.0f, gViewport[3] - 890.0f, 0.8f, color, gViewport[2], gViewport[3]);
    }
  }

} // namespace framework