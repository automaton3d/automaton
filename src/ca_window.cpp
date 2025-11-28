/*
 * =============
 * ca_window.cpp
 * =============
 */

#ifdef _WIN32
  #include <windows.h>
  #define NOMINMAX
  #define WIN32_LEAN_AND_MEAN
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GUI.h"
#include "simulation.h"
#include "ca_window.h"
#include "model/simulation.h"
//#include "logo.h"
#include "recorder.h"
#include "replay_progress.h"
#include "tinyfiledialogs.h"
#include "shader_utils.h"
#include "text_renderer.h"
#include "projection.h"
#include "globals.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "[GLFW ERROR] (" << error << "): " << description << std::endl;
    // Optionally: Breakpoint here in a debugger
}

namespace framework
{
  void requestExit();

  #ifdef DEBUG
  double debugClickX = -1;
  double debugClickY = -1;
  bool showDebugClick = false;
  #endif

  using namespace std;

  GLFWwindow* loadingWindow = nullptr;
  void saveReplay();
  void loadReplay();

  // UI state variables
  bool momentum, wavefront, single, partial, full, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;

  // Trackball
  extern VSlider vslider;
  extern HSlider hslider;
  extern Tickbox *tomo;
  extern vector<Radio> tomoDirs;
  extern Tickbox* scenarioHelpToggle;
  extern ReplayProgressBar *replayProgress;
  extern int vis_dx;
  extern int vis_dy;
  extern int vis_dz;

  // Logo
  //Logo *logo = nullptr;

  // Frame recorder
  FrameRecorder recorder;
  bool recordFrames = false;
  bool replayFrames = false;
  size_t replayIndex = 0;

  bool savePopup = false;
  bool loadPopup = false;

  bool toastActive = false;
  double toastStartTime = 0.0;
  std::string toastMessage;
  unsigned long long replayTimer = 0;
  bool showExitDialog = false;

  // Axis constants
  const float fullSize = 0.5f;
  const float axisLength = fullSize * 0.75f;

bool updateReplay()
{
    if (recorder.frames.empty())
        return false;

    if (replayIndex < recorder.frames.size())
    {
        const Frame& currentFrame = recorder.frames[replayIndex];

        // Reconstruct voxels and lattice
        recorder.reconstructVoxels(currentFrame, voxels, layerList->getSelected());
        recorder.applyFrame(currentFrame, lattice_curr);

        // Update lcenters
        for (const auto& layer : currentFrame.layers)
        {
            if (layer.w < automaton::W_USED)
            {
                automaton::lcenters[layer.w][0] = layer.center_x;
                automaton::lcenters[layer.w][1] = layer.center_y;
                automaton::lcenters[layer.w][2] = layer.center_z;
            }
        }

        // Update replay progress bar
        if (replayProgress)
            replayProgress->update(replayIndex + 1, recorder.frames.size());

        ++replayIndex;
        timer += FRAME;
    }

    return true;
}

/**
 * Function to show the loading window
 */
int showLoadingWindow()
{
    // Randomize seed
    srand(0); // or time(NULL)

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Calculate the position to center the window
    int xPos = (mode->width - 300) / 2;
    int yPos = (mode->height - 50) / 2;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    loadingWindow = glfwCreateWindow(300, 50, "Loading...", nullptr, nullptr);
    glfwSetWindowPos(loadingWindow, xPos, yPos);

    if (!loadingWindow) {
        std::cerr << "Failed to create loading window!" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(loadingWindow);
    
    // Initialize GLAD for loading window
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD for loading window\n";
        glfwDestroyWindow(loadingWindow);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    glfwShowWindow(loadingWindow);
    
    // Create text shader
    unsigned int textShaderID = framework::compileTextShader();

    textRenderer = new TextRenderer();

    // Font loading with multiple fallback paths
    bool fontLoaded = false;

    const char* fontPaths[] = {
        "fonts/arial.ttf",                                      // Primary location
        "arial.ttf",                                            // Fallback in root
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",     // Linux
        "/System/Library/Fonts/Supplemental/Arial.ttf",        // macOS
        "C:/Windows/Fonts/arial.ttf",                          // Windows
        nullptr
    };

    for (int i = 0; fontPaths[i] != nullptr; ++i) {
        if (textRenderer->init(fontPaths[i], 24, textShaderID)) {
            std::cout << "Font loaded successfully: " << fontPaths[i] << std::endl;
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        std::cerr << "WARNING: Could not load any font! Text will not render.\n"
                  << "Please place Arial.ttf in a 'fonts/' folder next to the executable.\n";
    }

    // Ellipsis animation variables
    int ellipsisCount = 2;
    int maxEllipsisCount = 12;
    double lastEllipsisChangeTime = glfwGetTime();
    double ellipsisChangeInterval = 0.5; // seconds

    int step = 0;
    while (!glfwWindowShouldClose(loadingWindow))
    {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        // Build loading message
        std::string loadingMessage = "Loading";
        for (int i = 0; i < ellipsisCount; ++i) {
            loadingMessage += ".";
        }

        // Draw the loading message at the center using TextRenderer
        if (textRenderer) {
            textRenderer->RenderText(
                loadingMessage,
                100, 25,               // position inside 300x50 window
                0.8f,                  // scale
                glm::vec3(1.0f, 1.0f, 1.0f), // white
                300, 50                // window size
            );
        }

        // Update ellipsis count
        if (glfwGetTime() - lastEllipsisChangeTime >= ellipsisChangeInterval) {
            ellipsisCount = (ellipsisCount % maxEllipsisCount) + 1;
            lastEllipsisChangeTime = glfwGetTime();
        }

        // Swap buffers
        glfwSwapBuffers(loadingWindow);

        // Call initialization routines
        if (automaton::initSimulation(step++))
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Close the loading window
    glfwDestroyWindow(loadingWindow);
    return EXIT_SUCCESS;
}

/**
 * Opens a Save File dialog and returns the chosen filename
 * Returns empty string if user cancels
 */
std::string getSaveFileName()
{
    const char* filters[] = { "*.dat" };

    const char* filename = tinyfd_saveFileDialog(
        "Save Replay As",                // Title
        "replay.dat",                    // Default filename
        1,                               // Number of filters
        filters,                         // Filters
        "Replay Files (*.dat)"           // Description
    );

    if (filename) {
        return std::string(filename);
    }
    return "";
}

/**
 * Opens an Open File dialog and returns the chosen filename
 * Returns empty string if user cancels
 */
std::string getOpenFileName()
{
    const char* filters[] = { "*.dat" };

    const char* filename = tinyfd_openFileDialog(
        "Open Replay",                   // Title
        "",                              // Default path/filename
        1,                               // Number of filters
        filters,                         // Filters
        "Replay Files (*.dat)",          // Description
        0                                // Allow multiple selection? (0 = no)
    );

    if (filename) {
        return std::string(filename);
    }
    return "";
}

/**
 * Saves the current replay to file
 */
void saveReplay()
{
    if (GUImode == SIMULATION && !recordFrames && !replayFrames && !pause)
    {
        std::string filename = getSaveFileName();
        if (!filename.empty())
        {
            try {
                recorder.saveToFile(filename);
                savePopup = true;
            }
            catch (const std::exception& e) {
                toastMessage = "Failed to save replay: " + std::string(e.what());
                toastStartTime = glfwGetTime();
                toastActive = true;
            }
        }
    }
}

/**
 * Loads a replay from file
 */
void loadReplay()
{
    if (GUImode == REPLAY)
    {
        if (!recordFrames && !replayFrames && !pause)
        {
            std::string filename = getOpenFileName();
            if (!filename.empty())
            {
                try {
                    recorder.loadFromFile(filename);
                    timer = recorder.savedTimer;
                    scenario = recorder.savedScenario;
                    loadPopup = true;
                }
                catch (const std::exception& e) {
                    toastMessage = "Failed to load replay: " + std::string(e.what());
                    toastStartTime = glfwGetTime();
                    toastActive = true;
                }
            }
        }
        // Ensure scenario help is disabled when loading replay
        scenarioHelpToggle->setState(false);
    }
}

  // Methods definition

  CAWindow::CAWindow() :  mWindow_(0)
  {
  }

  CAWindow::~CAWindow()
  {
  }

/**
 * Updates the projection matrix based on radio selection
 */
void CAWindow::updateProjection()
{

    float ratio = static_cast<float>(gViewport[2]) / static_cast<float>(gViewport[3]);

    // Default orthographic scale (same used everywhere else)
    const float orthoSize = 0.6f;

    if (projection[0].isSelected())
    {
        // Orthographic projection
        glm::mat4 orthoProj = glm::ortho(
            -orthoSize * ratio,  orthoSize * ratio,
            -orthoSize,          orthoSize,
             0.01f,              100.0f
        );
        mRenderer_.setProjection(orthoProj);
    }
    else if (projection[1].isSelected())
    {
        // Perspective projection (FOV = 65 degrees)
        const float fov = glm::radians(65.0f);
        glm::mat4 perspProj = glm::perspective(
            fov,
            ratio,
            0.01f,
            100.0f
        );
        mRenderer_.setProjection(perspProj);
    }
}

/*
 * UI zones reserved for sliders and checkboxes
 */
constexpr int BOTTOM_UI_ZONE = 120;  // Reserve bottom 120 pixels for UI
constexpr int LEFT_UI_ZONE   = 200;  // Reserve left 200 pixels for UI

/*
 * Helper function to check if click is in 3D interaction zone
 */
bool isIn3DZone(double xpos, double ypos, int windowWidth, int windowHeight)
{
    // Convert ypos to bottom-origin coordinates
    double bottomY = static_cast<double>(windowHeight) - ypos;

    // Return true only if click is outside reserved UI zones
    return (bottomY > BOTTOM_UI_ZONE && xpos > LEFT_UI_ZONE);
}


CAWindow & CAWindow::instance()
{
    static CAWindow i;
    return i;
}

/*
 * Runs the simulation.
 * (independent of the GUI thread)
 */
void CAWindow::SimulateThread()
{
    std::puts("Simulation thread launched...");
    instance().isThreadReady_ = true;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Prepare the mirror grid before starting
    automaton::swap_lattices();
    while (!instance().stopSimThread)
    {
        if (!pause)
        {
            if (replayFrames)
            {
                if (updateReplay())
                {
                    timer++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
                else
                {
                    replayFrames = false;
                    toastMessage = "No frames to replay.";
                    toastStartTime = glfwGetTime();
                    toastActive = true;
                }
            }
            else if (GUImode == SIMULATION)
            {
                if (automaton::simulation() && recordFrames)
                    recorder.recordFrame(automaton::lattice_curr, timer, scenario);

                automaton::updateBuffer();
                timer++;
            }
        }
        else
        {
            static bool prevTomoState = false;
            bool currentTomoState = (tomo && tomo->getState());
            if (currentTomoState || prevTomoState != currentTomoState)
                automaton::updateBuffer();
            prevTomoState = currentTomoState;
        }
    }
    std::puts("Simulation thread ended.");
}

int CAWindow::run()
{
    // Launch the loading window
    showLoadingWindow();

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int width  = mode->width;
    int height = mode->height;

    // Create a borderless GLFW window and center it on the screen
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    puts("GUI thread launched...");
    glfwSetErrorCallback(&CAWindow::errorCallback);

    mWindow_ = glfwCreateWindow(width, height, "Cellular automaton", nullptr, nullptr);
    if (!mWindow_) {
        glfwTerminate();
        perror("GUI window failed.");
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(mWindow_);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(mWindow_);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSwapInterval(1);

    // Initialize text renderer
    unsigned int textShaderID = framework::compileTextShader();
    textRenderer = new TextRenderer();
    if (!textRenderer->init("fonts/arial.ttf", 24, textShaderID)) {
        std::cerr << "ERROR: Failed to load font for main window." << std::endl;
    }

    // Register callbacks
    glfwSetCursorPosCallback(mWindow_, &CAWindow::moveCallback);
    glfwSetKeyCallback(mWindow_, &CAWindow::keyCallback);
    glfwSetMouseButtonCallback(mWindow_, &CAWindow::buttonCallback);
    glfwSetScrollCallback(mWindow_, &CAWindow::scrollCallback);
    glfwSetWindowSizeCallback(mWindow_, &CAWindow::sizeCallback);

    // Initialize renderer and interactor
    mInteractor_.setCamera(&mCamera_);
    mRenderer_.setCamera(&mCamera_);
    mAnimator_.setInteractor(&mInteractor_);
    mRenderer_.init();
    //logo = new Logo("logo.png");
    sizeCallback(mWindow_, width, height);

    // Correct isometric camera initialization
    {
        glm::vec3 eye    = instance().mCamera_.getEye();
        glm::vec3 center = instance().mCamera_.getCenter();
        float length     = glm::length(eye - center);

        glm::vec3 isoDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        instance().mCamera_.setEye(center + isoDir * length);
        instance().mCamera_.setUp(glm::vec3(0, 1, 0));
        instance().mCamera_.update();
        instance().mInteractor_.setCamera(&instance().mCamera_);
    }
    mRenderer_.clearVoxels();

    // Launch the simulation thread
    std::thread simThread([this]() { SimulateThread(); });
    simThread.detach();

    // Main loop
    while (!glfwWindowShouldClose(mWindow_))
    {
        if (pendingExit) {
            pendingExit = false;
            showExitDialog = true;
        }

        mRenderer_.render();

        glfwSwapBuffers(mWindow_);
        mRenderer_.handleMenuSelection();
        mInteractor_.update();

        if (!pause) {
            mAnimator_.animate();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        glfwPollEvents();

        if (savePopup) {
            toastMessage = "Frames saved successfully.";
            toastStartTime = glfwGetTime();
            toastActive = true;
            savePopup = false;
        }
        if (loadPopup) {
            toastMessage = "Frames loaded successfully.";
            toastStartTime = glfwGetTime();
            toastActive = true;
            loadPopup = false;
        }
    }

    puts("GUI thread ended.");
    stopSimThread = true;
    glfwDestroyWindow(mWindow_);
    glfwTerminate();
    return EXIT_SUCCESS;
}


  void CAWindow::onDelayToggled(Tickbox* toggled)
  {
  int i = 0;
    for (const auto &box : delays)
    {
      switch (i)
      {
        case 0:
          automaton::convol_delay = box.getState();
        break;
        case 1:
          automaton::diffuse_delay = box.getState();
        break;
        case 2:
          automaton::reloc_delay = box.getState();
        break;
      }
      i++;
    }
  }

} // end namespace framework

/************************************
 *                                  *
 *   Simulation mode entry point.   *
 *     (Called by the Splash)       *
 *                                  *
 ************************************/
int runSimulation(int scenarioParam, bool paused)
{
    scenario = scenarioParam;
    pause = paused;

    std::cout << "[INFO] Starting Simulation mode..." << std::endl;

    framework::GUImode = framework::SIMULATION;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(glfwErrorCallback);

    // --- OPENGL CONTEXT HINTS ---
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // --- CREATE WINDOW HERE ---
    GLFWwindow* window = glfwCreateWindow(1280, 960, "Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- UPDATE gViewport WITH REAL DIMENSIONS ---
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    gViewport[0] = 0;
    gViewport[1] = 0;
    gViewport[2] = width;
    gViewport[3] = height;

    glViewport(0, 0, width, height);

    std::cout << "[INFO] Viewport updated: " 
              << width << " x " << height << std::endl;

    int status = framework::CAWindow::instance().run();

    glfwTerminate();
    return status;
}

/************************************
 *                                  *
 *     Replay mode entry point.     *
 *     (Called by the Splash)       *
 *                                  *
 ************************************/
int runReplay()
{
    std::cout << "[INFO] Starting Replay mode..." << std::endl;

    scenario = -1;
    framework::GUImode = framework::REPLAY;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return EXIT_FAILURE;
    }
    
    // Set OpenGL context hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    int status = framework::CAWindow::instance().run();

    glfwTerminate();
    return status;
}