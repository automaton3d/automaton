/*
 * GUI_InitWidgets.cpp (merged)
 */

#include "GUI.h"
#include "tickbox.h"
#include "radio.h"
#include "layers.h"
#include "progress.h"
#include "replay_progress.h"
#include "hslider.h"
#include "cawindow.h"
#include <GL/freeglut.h>
#include "model/simulation.h"

namespace framework
{
  using namespace automaton;

  extern GLint gViewport[4];
  extern HSlider hslider;
  extern ReplayProgressBar *replayProgress;
  extern unsigned long tbegin;
  extern std::vector<Radio> tomoDirs;
  extern int barWidths[3];
  extern Tickbox* scenarioHelpToggle;
  extern bool showScenarioHelp;
  extern Tickbox* tomo;

  void initializeWidgets()
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
    scenarioHelpToggle = new Tickbox(230, 105, "Scenario Help");
    scenarioHelpToggle->setState(true); // default: visible
    scenarioHelpToggle->onToggle = [](bool state)
    {
      showScenarioHelp = state;
    };
  }

}
