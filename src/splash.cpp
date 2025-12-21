/*
 * splash.cpp - Single-window version
 * Runs inside existing GLFW window passed from main.cpp
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glm_host_only.h"
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <windows.h>

#include "simulation.h"
#include "button.h"
#include "draw_utils.h"
#include "dropdown.h"
#include "globals.h"
#include "logo.h"
#include "text_renderer.h"
#include "tickbox.h"
#include "shader.h"
#include "projection_manager.h"
#include "splash.h"

// External globals from main.cpp
extern TextRenderer* textRenderer;
extern unsigned int colorProgram2D;
extern unsigned int textProgram;

// ---------------------------------------------------------------------
// Forward declarations of input callbacks
// ---------------------------------------------------------------------
static int winW();
static int winH();
static void getMousePos(int& mx, int& my);
static int myTopDown(int my_bottom);

void mouseButtonCallback(GLFWwindow*, int, int, int);
void scrollCallback(GLFWwindow*, double, double);
void keyCallback(GLFWwindow*, int, int, int, int);
void passiveMotionCallback(GLFWwindow*, double, double);

// ---------------------------------------------------------------------
// Helpers (2D projection, panels, title)
// ---------------------------------------------------------------------
static const glm::mat4& proj2D() { return ProjectionManager::instance().get2DOrtho(); }
static int winW() { return ProjectionManager::instance().getWidth(); }
static int winH() { return ProjectionManager::instance().getHeight(); }

static void drawRaisedPanel(float x, float y, float w, float h)
{
    drawQuad2D(x, y, x + w, y + h, glm::vec3(0.85f, 0.85f, 0.9f), proj2D());
    drawLineLoop2D({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}},
                   glm::vec3(0.7f, 0.7f, 0.8f), proj2D(), 2.0f);
}

static void drawTitle()
{
    if (!textRenderer) return;

    const std::string title = "It from bit: a concrete attempt";
    float scale = 0.6f;

    float textW = textRenderer->measureTextWidth(title, scale);
    float x = (winW() - textW) * 0.5f;
    float y = winH() - 40.0f;

    glUseProgram(textProgram);
    const glm::mat4& P = proj2D();
    glUniformMatrix4fv(glGetUniformLocation(textProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(P));

    textRenderer->RenderText(title, x, y, scale, glm::vec3(0.2f, 0.4f, 0.9f),
                             winW(), winH());
    glUseProgram(0);
}

// ---------------------------------------------------------------------
// Namespace splash: state, setup, API
// ---------------------------------------------------------------------
namespace splash {
    int lattice_size = 21;
    int numLayers = 10;
    int selection = -1;
    bool shouldExit = false;
    bool helpHover = false;

    Button* simBtn = nullptr;
    Button* statBtn = nullptr;
    Button* replayBtn = nullptr;
    Button* helpLink = nullptr;
    Tickbox* startPausedBox = nullptr;
    Dropdown* sizeDropdown = nullptr;
    Dropdown* layerDropdown = nullptr;
    Dropdown* scenarioDropdown = nullptr;
    framework::Logo* logo_splash = nullptr;

    // window atual (para obter cursor, maximizar, etc.)
    GLFWwindow* window = nullptr;

    const std::vector<std::string> scenarioOptions = {
        "Wrapping test", "Relocate test", "Orphan test", "Contraction test",
        "Hunting test", "Reissue test", "Dispersion test", "Full simulation"
    };

    // Criação de widgets, sem loop interno
    static void setupUI()
    {
        int rightX = winW() - 250;
        int screenH = winH();

        simBtn    = new Button(rightX, screenH - 260, 200, 40, "Simulation");
        statBtn   = new Button(rightX, screenH - 180, 200, 40, "Statistics");
        replayBtn = new Button(rightX, screenH - 110, 200, 40, "Replay");
        helpLink  = new Button((winW() - 100)/2, screenH - 45, 100, 20, "Help");

        std::vector<std::string> sizes, layers;
        for (int s = 5; s <= 89; s += 2) sizes.push_back(std::to_string(s));
        for (int l : {10,12,14,16,18,20,24,28,32,38,44,52,60,70,82,96,112,130,
                      150,174,200,230,264,300,320,340,360,364})
            layers.push_back(std::to_string(l));

        sizeDropdown      = new Dropdown(50, screenH - 280, 200, 30, sizes, screenH, "Lattice Size" );
        layerDropdown     = new Dropdown(50, screenH - 230, 200, 30, layers, screenH, "Layers");
        scenarioDropdown  = new Dropdown(50, screenH - 180, 200, 30, scenarioOptions, screenH, "Scenario");
        startPausedBox    = new Tickbox(rightX, 315, "Start Paused");

        // Logo depois de shader estar pronto (feito em main antes)
        logo_splash = new framework::Logo("logo_bar.png");

        sizeDropdown->setSelectedIndex(8);   // 21
        layerDropdown->setSelectedIndex(0);  // 10
    }

    // -----------------------------------------------------------------
    // Public API: initialize, render, cleanup
    // -----------------------------------------------------------------
    int initialize(GLFWwindow* win)
    {
        window = win;
        shouldExit = false;
        helpHover = false;

        // Registrar callbacks
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, passiveMotionCallback);

        // Criar widgets
        setupUI();
        return 0;
    }

    void cleanup()
    {
        delete simBtn; simBtn = nullptr;
        delete statBtn; statBtn = nullptr;
        delete replayBtn; replayBtn = nullptr;
        delete helpLink; helpLink = nullptr;
        delete startPausedBox; startPausedBox = nullptr;
        delete sizeDropdown; sizeDropdown = nullptr;
        delete layerDropdown; layerDropdown = nullptr;
        delete scenarioDropdown; scenarioDropdown = nullptr;
        delete logo_splash; logo_splash = nullptr;
    }

    void render()
    {
        glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Título
        drawTitle();

        // Logo
        if (logo_splash) {
            logo_splash->draw((winW() - 200) / 2, 60, 1.0f);
        }

        int w = winW(), h = winH();

        // Link de ajuda
        if (helpLink) helpLink->drawAsHyperlink(*textRenderer, helpHover, w, h);

        // Painéis
        drawRaisedPanel(30, 295, w - 60, 180);
        drawRaisedPanel(w - 260, w - 290, 220, 80);

        // Botões
        if (simBtn)    simBtn->draw(colorProgram2D, *textRenderer, w, h);
        if (statBtn)   statBtn->draw(colorProgram2D, *textRenderer, w, h);
        if (replayBtn) replayBtn->draw(colorProgram2D, *textRenderer, w, h);

        // Dropdowns (sempre desenhar o estado atual)
        if (sizeDropdown)     sizeDropdown->draw(*textRenderer);
        if (layerDropdown)    layerDropdown->draw(*textRenderer);
        if (scenarioDropdown) scenarioDropdown->draw(*textRenderer);

        // Tickbox
        if (startPausedBox) {
            startPausedBox->setFontScale(0.32f);
            startPausedBox->draw(*textRenderer);
        }
    }
} // namespace splash

// ---------------------------------------------------------------------
// Input callbacks
// ---------------------------------------------------------------------
/*
static void getMousePos(int& mx, int& my)
{
    double x, y;
    glfwGetCursorPos(splash::window, &x, &y);
    mx = static_cast<int>(x);
    my = winH() - static_cast<int>(y); // bottom-up Y
}
*/

/*
static void getMousePos(int& mx, int& my)
{
    double x, y;
    glfwGetCursorPos(splash::window, &x, &y);
    mx = static_cast<int>(x);
    my = static_cast<int>(y);  // Keep top-down, DON'T convert!
}
*/

/*

static void getMousePos(int& mx, int& my)
{
    double x, y;
    glfwGetCursorPos(splash::window, &x, &y);
    mx = static_cast<int>(x);
    my = static_cast<int>(y);  // Top-down (raw GLFW)
}
*/

static void getMousePos(int& mx, int& my)
{
    double x, y;
    glfwGetCursorPos(splash::window, &x, &y);
    mx = static_cast<int>(x);
    my = winH() - static_cast<int>(y); // bottom-up for Button
}

static int myTopDown(int my_bottom)
{
    return winH() - my_bottom;
}

void mouseButtonCallback(GLFWwindow*, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    int mx, my;
    getMousePos(mx, my);

    // Dropdowns recebem cliques com Y bottom-up
    bool handled = false;
    if (splash::sizeDropdown)     handled = splash::sizeDropdown->handleClick(mx, my);
    if (!handled && splash::layerDropdown)    handled = splash::layerDropdown->handleClick(mx, my);
    if (!handled && splash::scenarioDropdown) handled = splash::scenarioDropdown->handleClick(mx, my);

    if (!handled) {
        if (splash::sizeDropdown)     splash::sizeDropdown->close();
        if (splash::layerDropdown)    splash::layerDropdown->close();
        if (splash::scenarioDropdown) splash::scenarioDropdown->close();
    }

    // Botões (usa conversão para top-down se necessário)
    if (splash::simBtn && splash::simBtn->contains(mx, my, winH())) {
        automaton::calculateParameters(splash::lattice_size, splash::numLayers);
        if (automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
            currentMode = SIMULATION;
            glfwMaximizeWindow(splash::window);

            int w, h;
            glfwGetFramebufferSize(splash::window, &w, &h);
            ProjectionManager::instance().setViewport(w, h);

            latticeSize = splash::sizeDropdown ? splash::sizeDropdown->getSelectedIndex() : latticeSize;
            numLayers   = splash::layerDropdown ? splash::layerDropdown->getSelectedIndex() : numLayers;
            scenario    = splash::scenarioDropdown ? splash::scenarioDropdown->getSelectedIndex() : scenario;
            initPaused  = splash::startPausedBox ? splash::startPausedBox->getState() : false;
            splash::shouldExit = true;
        }
        else {
        	MessageBoxA(NULL,
              "Failed to allocate lattice.\nPlease try other parameters.",
              "Allocation Error",
              MB_OK | MB_ICONERROR);
        }
    }
    else if (splash::statBtn && splash::statBtn->contains(mx, my, winH())) {
        automaton::calculateParameters(splash::lattice_size, splash::numLayers);
        if (automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
            currentMode = STATISTICS;
            scenario    = 7;
            splash::shouldExit = true;
        }
        else {
        	MessageBoxA(NULL,
              "Failed to allocate lattice.\nPlease try other parameters.",
              "Allocation Error",
              MB_OK | MB_ICONERROR);
        }
    }
    else if (splash::replayBtn && splash::replayBtn->contains(mx, my, winH())) {
        automaton::calculateParameters(splash::lattice_size, splash::numLayers);
        if (automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
            currentMode = REPLAY;
            glfwMaximizeWindow(splash::window);

            int w, h;
            glfwGetFramebufferSize(splash::window, &w, &h);
            ProjectionManager::instance().setViewport(w, h);
            splash::shouldExit = true;
        }
        else {
        	MessageBoxA(NULL,
              "Failed to allocate lattice.\nPlease try other parameters.",
              "Allocation Error",
              MB_OK | MB_ICONERROR);
        }
    }
    else if (splash::helpLink && splash::helpLink->contains(mx, my, winH())) {
      system("start https://github.com/automaton3d/automaton/blob/master/help.md");
    }


    if (splash::startPausedBox) {
        int myTopDown = winH() - my;  // Convert bottom-up to top-down
        splash::startPausedBox->onClick(mx, myTopDown);
    }

}

void scrollCallback(GLFWwindow*, double, double yoffset)
{
    int dir = (yoffset > 0) ? -1 : 1;
    if (splash::sizeDropdown && splash::sizeDropdown->isOpen_)         splash::sizeDropdown->scroll(dir);
    if (splash::layerDropdown && splash::layerDropdown->isOpen_)       splash::layerDropdown->scroll(dir);
    if (splash::scenarioDropdown && splash::scenarioDropdown->isOpen_) splash::scenarioDropdown->scroll(dir);
}

void keyCallback(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        automaton::calculateParameters(splash::lattice_size, splash::numLayers);
        if (automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
            currentMode = SIMULATION;
            scenario    = splash::scenarioDropdown ? splash::scenarioDropdown->getSelectedIndex() : scenario;
            pause       = splash::startPausedBox ? splash::startPausedBox->getState() : false;
            splash::shouldExit = true;
        }
    }
}

void passiveMotionCallback(GLFWwindow*, double xpos, double ypos)
{
    int mx = (int)xpos;
    int my = winH() - (int)ypos;      // bottom-up (Dropdown usa esta convenção)
    int my_top = myTopDown(my);       // top-down para hover de botão (se necessário)

    if (splash::helpLink) splash::helpHover = splash::helpLink->contains(mx, my_top, winH());
    if (splash::sizeDropdown)     splash::sizeDropdown->updateHover(mx, my);
    if (splash::layerDropdown)    splash::layerDropdown->updateHover(mx, my);
    if (splash::scenarioDropdown) splash::scenarioDropdown->updateHover(mx, my);
}
