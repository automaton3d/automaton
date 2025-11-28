/*
 * GUI.cpp
 *
 * Implements the GUIrenderer orchestration logic.
 */

#include "GUI.h"
#include "globals.h"
#include "hslider.h"
#include "vslider.h"
#include "tickbox.h"
#include "replay_progress.h"
#include "progress.h"
#include "logo.h"
#include "ca_window.h"
#include "text_renderer.h"
#include "dropdown.h"
#include "radio.h"
#include "layers.h"
#include "model/simulation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "menubar.h"

namespace framework
{
    // -----------------------------------------------------------------
    // Global / static variables that belong to the framework namespace
    // -----------------------------------------------------------------
    std::unique_ptr<MenuBar> menuBar;

    int barWidths[3] = {0, 0, 0};

    bool MULTICUBE_MODE = false;
    int GUImode = SIMULATION;

    bool helpHover = false;
    bool showHelp = false;
    bool showAboutDialog = false;

    std::vector<Tickbox> data3D;
    std::vector<Tickbox> delays;
    std::vector<Radio> views;
    std::vector<Radio> projection;
    std::unique_ptr<LayerList> layerList;

    HSlider hslider(0, 0, 0, 0, 0);
    VSlider vslider(1890, 93, 10.0f, 607.0f, 30.0f);
    Tickbox* tomo = nullptr;
    Tickbox* scenarioHelpToggle = nullptr;

    ProgressBar* progress = nullptr;
    ReplayProgressBar* replayProgress = nullptr;

    int vis_dx = 0;
    int vis_dy = 0;
    int vis_dz = 0;

    int windowWidth = 800;
    int windowHeight = 600;

    double tbegin = 0;

    std::vector<Radio> tomoDirs;
    unsigned tomo_x = 0;
    unsigned tomo_y = 0;
    unsigned tomo_z = 0;

    GLuint colorProgram3D = 0;
    GLint colorMvpLoc3D = -1;
    GLint colorColorLoc3D = -1;

    bool showScenarioHelp = false;
    std::vector<std::string> scenarioHelpTexts;
    
    Logo* logo = nullptr; 
    
    extern const char* ui_help[];
    extern const char* record_help[];

    // -----------------------------------------------------------------
    // GUIrenderer implementation
    // -----------------------------------------------------------------
    GUIrenderer::GUIrenderer() : Renderer() {}

    GUIrenderer::~GUIrenderer()
    {
        delete tomo;
        delete scenarioHelpToggle;
        delete progress;
        delete replayProgress;
    }

    void GUIrenderer::init()
    {
        initializeWidgets();
        initMenuDropdowns();

        if (!tomo) {
            tomo = new Tickbox(50, gViewport[3] - 800, "Enable");
        }

        if (!progress) {
            progress = new ProgressBar(gViewport[2] / 2, 100, gViewport[2] / 4, 20);
            progress->init(gViewport[2], 100, 100, 100, automaton::FRAME);
        }

        if (!replayProgress) {
            replayProgress = new ReplayProgressBar(gViewport[2], gViewport[3]);
        }
        if (!logo) {
            logo = new Logo("logo_bar.png");
        }
        if (!scenarioHelpToggle) {
            scenarioHelpToggle = new Tickbox(50, gViewport[3] - 120, "Show Scenario Help");
            float labelCol[3] = {0.0f, 0.0f, 0.0f};
            scenarioHelpToggle->setColor(nullptr, labelCol, nullptr, nullptr);
        }
    }

    void GUIrenderer::initMenuDropdowns()
    {
        menuBar = std::make_unique<MenuBar>(
            textRenderer,
            static_cast<float>(gViewport[2]),
            static_cast<float>(gViewport[3])
        );

        auto fileMenu = std::make_unique<Menu>("File");
        fileMenu->AddItem("Open", [](){ loadReplay(); });
        fileMenu->AddItem("Save", [](){ saveReplay(); });
        fileMenu->AddItem("Exit", [](){ requestExit(); });
        menuBar->AddMenu(std::move(fileMenu));

        auto helpMenu = std::make_unique<Menu>("Help");
        helpMenu->AddItem("GUI keys", [](){ showHelp = true; });
        helpMenu->AddItem("About", [](){ showAboutDialog = true; });
        menuBar->AddMenu(std::move(helpMenu));

    }

    void GUIrenderer::handleMenuSelection()
    {
        // Old Dropdown handling is now completely replaced by MenuBar
        // (the callbacks in initMenuDropdowns already do the work)
    }

    void GUIrenderer::render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (!textRenderer) {
            static bool warned = false;
            if (!warned) {
                std::cerr << "WARNING: textRenderer not initialized before GUI render.\n";
                warned = true;
            }
        }

        renderClear();
        renderObjects();
        renderUI();

        // Render the new menu bar on top of everything
        if (menuBar) menuBar->Render();
    }

void GUIrenderer::resize(int width, int height)
{
    if (height == 0) height = 1;
    float ratio = static_cast<float>(width) / static_cast<float>(height);

    glViewport(0, 0, width, height);
    gViewport[2] = width;
    gViewport[3] = height;

    windowWidth  = width;
    windowHeight = height;

    mOrtho       = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    mPerspective = glm::perspective(glm::radians(45.0f), ratio, 0.01f, 100.0f);

    if (progress)     progress->resize(width, height);
    if (replayProgress) replayProgress->setPosition(width, height, height - 100);
    if (menuBar)      menuBar->Resize(static_cast<float>(width), static_cast<float>(height));
    hslider.resize(width, height);
}

    void GUIrenderer::renderClear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
    }

    // -----------------------------------------------------------------
    // Helper functions
    // -----------------------------------------------------------------

    void requestExit()
    {
        framework::CAWindow::instance().pendingExit = true;
    }

    void showAboutWindow()
    {
        showAboutDialog = true;
    }
}