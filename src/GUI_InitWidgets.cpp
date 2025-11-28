#include "GUI.h"
#include "tickbox.h"
#include "radio.h"
#include "layers.h"
#include "progress.h"
#include "replay_progress.h"
#include "hslider.h"
#include "ca_window.h"
#include "globals.h"
#include "model/simulation.h"
#include <vector>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <GLFW/glfw3.h>

namespace framework
{
  using namespace automaton;

  // Estas variáveis framework:: estão corretas:
  extern HSlider hslider;
  extern ReplayProgressBar *replayProgress;
  extern double tbegin;
  extern std::vector<Radio> tomoDirs;
  extern Tickbox* scenarioHelpToggle;
  extern bool showScenarioHelp;
  extern Tickbox* tomo;
  extern ProgressBar* progress;  // ADICIONE se não estiver
  extern std::unique_ptr<LayerList> layerList;  // ADICIONE se não estiver
  extern int barWidths[3];

  void initializeWidgets()
 {
    assert(L3 > 0);
    voxels.assign(L3, 0); // inicializa todos os voxels com cor preta

    // Configuração básica do contexto OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Inicialização do slider horizontal
    hslider = HSlider((gViewport[2] - 400.0f) / 2.0f,
                      80.0f,
                      400.0f, 20.0f, 30.0f);
    hslider.setThumbPosition(0.5f);

    // Barras de progresso
    if (!progress) {
        progress = new ProgressBar(
            gViewport[2] / 2,   // posição X (centro horizontal)
            100,                // posição Y (altura da barra na tela)
            gViewport[2] / 4,   // largura da barra (¼ da tela)
            20                  // altura da barra
        );
    }
    
    if (!replayProgress) {
        replayProgress = new ReplayProgressBar(gViewport[2], gViewport[3]);
    }

    // Tempo inicial multiplataforma
    tbegin = glfwGetTime();

    // Lista de camadas
    layerList = std::make_unique<LayerList>(W_USED);

    // Tomografia
    if (tomo) {
        tomo->onToggle = [](bool state) {
            std::cout << "Tomography mode: " << (state ? "Enabled" : "Disabled") << '\n';
        };
    }

    // Tickboxes de dados 3D

    data3D = {
        Tickbox(50,gViewport[3] - 50,"Wavefront"),
        Tickbox(50,gViewport[3] - 80,"Momentum"),
        Tickbox(50,gViewport[3] - 110,"Spin"),
        Tickbox(50,gViewport[3] - 140,"Sine mask"),
        Tickbox(50,gViewport[3] - 170,"Hunting"),
        Tickbox(50,gViewport[3] - 200,"Centers"),
        Tickbox(50,gViewport[3] - 230,"Lattice"),
        Tickbox(50,gViewport[3] - 260,"Axes"),
        Tickbox(50,gViewport[3] - 290,"Plane")
    };
    data3D[0].setState(true);
    data3D[5].setState(true);
    data3D[7].setState(true);

    // Tickboxes de delays
    delays = {
        Tickbox(50,gViewport[3] - 370,"Convolution"),
        Tickbox(50,gViewport[3] - 400,"Diffusion"),
        Tickbox(50,gViewport[3] - 430,"Relocation")
    };
    for (Tickbox& box : delays) {
        box.onToggle = [&box](bool) {
            CAWindow::instance().onDelayToggled(&box);
        };
    }

    // Radios de visão e projeção
    views = {
        Radio(60,gViewport[3] - 510,"Isometric"),
        Radio(60,gViewport[3] - 540,"XY"),
        Radio(60,gViewport[3] - 570,"YZ"),
        Radio(60,gViewport[3] - 600,"ZX")
    };
    views[0].setSelected(true);

    projection = {
        Radio(60,gViewport[3] - 680,"Ortho"),
        Radio(60,gViewport[3] - 710,"Perspective")
    };
    projection[1].setSelected(true);

    // Radios de tomografia
    tomoDirs = {
        Radio(80,gViewport[3] - 815,"XY"),
        Radio(80,gViewport[3] - 845,"YZ"),
        Radio(80,gViewport[3] - 875,"ZX")
    };

    // Cálculo das larguras da barra de progresso
    int barWidth = gViewport[2] / 4;
    double totalRatio = static_cast<double>(FRAME);
    barWidths[0] = static_cast<int>(barWidth * static_cast<double>(CONVOL) / totalRatio);
    barWidths[1] = static_cast<int>(barWidth * static_cast<double>(DIFFUSION - CONVOL) / totalRatio);
    barWidths[2] = static_cast<int>(barWidth * static_cast<double>(RELOC - DIFFUSION) / totalRatio);

    // Toggle de ajuda de cenário
    if (!scenarioHelpToggle) {
        scenarioHelpToggle = new Tickbox(230,gViewport[3] - 105,"Scenario Help");
    }
    scenarioHelpToggle->setState(false);
    scenarioHelpToggle->onToggle = [](bool state) {
        showScenarioHelp = state;
    };
}

}