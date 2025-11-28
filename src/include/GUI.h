/*
 * GUI.h
 *
 * Declares the Graphical Unit Interface rendering routines.
 */

#ifndef GUI_H
#define GUI_H

#include "menubar.h"
#include "renderer.h"
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include "button.h"
#include "dropdown.h"

#undef DEBUG
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>       // value_ptr
#include <glm/gtc/matrix_transform.hpp> // perspective

#include "model/simulation.h"
#include "radio.h"
#include "layers.h"
#include "text_renderer.h"
#include "tickbox.h"
#include "progress.h"
#include "globals.h"

struct AxisProjection { float x0,y0,x1,y1; };
extern AxisProjection gAxisProj[3];
extern bool gAxisProjValid;

namespace automaton
{
  extern unsigned L2;
}

namespace framework
{
  // -----------------------------
  // GUI mode enumeration
  // -----------------------------
  enum GUIMode {
      SIMULATION = 0,
      REPLAY     = 1,
      STATISTICS = 2
  };

  // Global GUI mode variable (defined once in GUI.cpp)
  extern int GUImode;
  extern bool MULTICUBE_MODE;
  extern unsigned tomo_x, tomo_y, tomo_z;

  extern ProgressBar *progress;
  extern bool helpHover;
  extern bool active;
  extern std::vector<Tickbox> data3D;
  extern std::unique_ptr<LayerList> layerList;
  extern std::vector<Tickbox> delays;
  extern std::vector<Radio> views;
  extern std::vector<Radio> projection;

  extern bool replayFrames;
  extern unsigned long long replayTimer;
  extern std::unique_ptr<MenuBar> menuBar;

  class GUIrenderer : public Renderer
  {
  public:
      GUIrenderer();
      virtual ~GUIrenderer();

      void init();
      virtual void render();
      void resize(int width, int height);
      void setProjection(const glm::mat4& proj) { mProjection_ = proj; }
      void clearVoxels();
      void initMenuDropdowns();
      void handleMenuSelection();
      // ✅ Novo: acesso ao TextRenderer
      TextRenderer& getTextRenderer() { return textRenderer_; }

  private:
      // Internal rendering methods
      void renderAxes();
      void renderCenter();
      void renderClear();
      void renderCube();
      void renderPlane();
      void renderObjects();
      void renderWavefront();
      void renderCounts();
      void renderMomentum();
      void renderSpin();
      void renderSineMask();
      void renderHunting();
      void renderCenters();
      void renderCenterBox(const char* text);
      void renderUI();
      void renderElapsedTime();
      void renderSimulationStats();
      void renderLayerInfo();
      void renderLayers();
      void renderHelpText();
      void renderSliders();
      void renderTomoControls();
      void renderPauseOverlay();
      void renderSectionLabels();
      void render3Dboxes();
      void renderDelays();
      void renderViewpointRadios();
      void renderProjectionRadios();
      void renderTomoRadios();
      void renderHyperlink();
      void renderSlice();
      void renderScenarioHelpPane();
      void renderTomoPlane();
      void drawPanel(int x, int y, int width, int height);
      void drawRoundedRect(float x, float y, float w, float h, float radius);
      void drawRoundedRectOutline(float x, float y, float w, float h, float radius);
      void renderExitDialog();
      void enhanceVoxel();

      // Helper methods
      inline bool isVoxelVisible(unsigned x, unsigned y, unsigned z)
      {
          if (x >= automaton::EL) return false;
          if (y >= automaton::W_USED) return false;
          if (z >= automaton::L2) return false;
          return true;
      }

  private:
      glm::mat4 mProjection_;
      glm::mat4 mOrtho;
      glm::mat4 mPerspective;

      std::vector<std::array<unsigned, 3>> lastPositions_;
      std::vector<std::array<float, 2>> screenPositions_;
      std::map<std::pair<int,int>, int> counts_;

      // ✅ Novo membro para texto
      TextRenderer textRenderer_;
  };

  void initializeWidgets();
  void promptExit();

} // end namespace framework

#endif // GUI_H
