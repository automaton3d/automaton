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
#include <atomic>
#include "button.h"
#include "dropdown.h"

#define GLM_FORCE_RADIANS

#include "glm_host_only.h"
#include "simulation.h"
#include "radio.h"
#include "layers.h"
#include "text_renderer.h"
#include "tickbox.h"
#include "progress.h"
#include "globals.h"
#include "app_context.h"

extern AxisProjection gAxisProj[3];
extern bool gAxisProjValid;

namespace automaton
{
  extern unsigned L2;
}

namespace framework
{
  extern const std::vector<std::string> record_help;
  extern const std::vector<std::string> ui_help;

  // Global GUI mode variable (defined once in GUI.cpp)
  extern bool MULTICUBE_MODE;
  extern unsigned tomo_x, tomo_y, tomo_z;

  extern ProgressBar *progress;
  extern bool helpHover;
  extern bool active;
  extern std::vector<Tickbox> data3D;
  extern std::unique_ptr<LayerList> layerList;
  extern std::vector<Tickbox> delays;
  extern std::vector<Radio> views;
  extern std::vector<Radio> projectRads;

  //extern bool replayFrames;
  extern unsigned long long replayTimer;
  extern std::atomic<bool> replayFrames;

  void init();
  void resize(int width, int height);
  void clearVoxels();
  void initMenuDropdowns();
  void handleMenuSelection();
  // ✅ Novo: acesso ao TextRenderer

  // Internal rendering methods
  void renderAxes();
  void renderCenter();
  void renderClear();
  void renderCube();
  void renderGrid();
  void render3DObjects();
  void renderWavefront();
  void renderCounts();
  void renderMomentum(AppContext& ctx);
  void renderSpin();
  void renderSineMask();
  void renderHunting();
  void renderCenters();
  void renderUI();
  void renderElapsedTime();
  void renderSimulationStats();
  void renderComputeStats();
  void renderLayerInfo();
  void renderLayers();
  void renderHelpText();
  void renderSliders();
  void renderTomoControls();
  void renderPauseOverlay();
  void renderSectionLabels();
  void render3Dboxes();
  void renderDelays();
  void renderScenarioHelpToggle();
  void renderViewpointRadios();
  void renderProjectionRadios();
  void renderTomoRadios();
  void renderHyperlink();
  void renderSlice();
  void renderScenarioHelpPane();
  // In GUI.h – replace the old declaration with this:
  void drawPanel(float x, float y, float w, float h,
                 const glm::vec3& bgColor = glm::vec3(0.05f, 0.05f, 0.1f),
                 const glm::vec3& borderColor = glm::vec3(0.3f, 0.3f, 0.8f),
                 float borderThickness = 2.0f,
                 const glm::mat4& proj = ProjectionManager::instance().get2DOrtho());
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

  extern glm::mat4 mProjection_;
  extern glm::mat4 mOrtho;
  extern glm::mat4 mPerspective;

  extern std::vector<std::array<unsigned, 3>> lastPositions_;
  extern std::vector<std::array<float, 2>> screenPositions_;
  extern std::map<std::pair<int,int>, int> counts_;

  // ✅ Novo membro para texto
  extern TextRenderer hudText;

  void initializeWidgets(AppContext& ctx);
  void promptExit();

} // end namespace framework

#endif // GUI_H
