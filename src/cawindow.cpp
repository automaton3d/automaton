/*
 * window.cpp
 *
 * processes IO, runs the simulation, and renders graphics for it.
 *
 * This is the top file of the program.
 */

#include <GUI.h>
#include <text.h>
#include <iostream>
#include <windows.h>
#include <windows.h>
#include <shellapi.h>
#include <vector>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <cawindow.h>
#include <thread>
#include <chrono>
#include "model/simulation.h"
#include "hslider.h"
#include "vslider.h"
#include "logo.h"
#include "recorder.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

namespace automaton
{
  extern int scenario;
}

namespace framework
{

  #ifdef DEBUG
  double debugClickX = -1;
  double debugClickY = -1;
  bool showDebugClick = false;
  #endif

  using namespace std;
  GLFWwindow* loadingWindow = nullptr;
  bool pause = false;

  // UID

  HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk, axes_chk;
  HWND single_rad, partial_rad, full_rad, rand_rad;
  HWND xy_rad, yz_rad, zx_rad, iso_rad;
  HPEN xPen, yPen, zPen, boxPen;
  HWND stopButton, suspendButton, centerButton;
  bool momentum, wavefront, single, partial, full, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;

  // Trackball

  bool active = true;
  unsigned long long timer = 0;
  extern VSlider vslider;
  extern HSlider hslider;
  extern Tickbox *tomo;
  extern vector<Radio> tomoDirs;
  extern bool showHelp;
  extern Tickbox* scenarioHelpToggle;
  extern unsigned tomo_x, tomo_y, tomo_z;

  // Logo

  Logo *logo = nullptr;
  bool logoCreated = false;

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

  // In cawindow.cpp, modify updateReplay() function:

  void updateReplay()
  {
    using namespace automaton;

    if (replayIndex < recorder.frames.size())
    {
      const Frame& currentFrame = recorder.frames[replayIndex];

      // If it's a keyframe, apply directly
      // If it's a delta frame, we need to apply it on top of current state
      if (currentFrame.isKeyframe) {
        // Clear and rebuild from keyframe
        std::fill(voxels, voxels + EL * EL * EL, RGB(0, 0, 0));
      }

      currentFrame.apply(lattice_curr);

      // Update voxel buffer for rendering
      int selectedLayer = list->getSelected();

      // Clear voxels for non-keyframes (delta updates)
      if (!currentFrame.isKeyframe) {
        std::fill(voxels, voxels + EL * EL * EL, RGB(0, 0, 0));
      }

      // Render all active cells in current layer
      for (unsigned x = 0; x < EL; ++x) {
        for (unsigned y = 0; y < EL; ++y) {
          for (unsigned z = 0; z < EL; ++z) {
            const Cell& cell = getCell(lattice_curr, x, y, z, selectedLayer);

            bool visible = true;
            if (tomo && tomo->getState()) {
              if (tomoDirs[0].isSelected())      visible = (z == tomo_z);
              else if (tomoDirs[1].isSelected()) visible = (x == tomo_x);
              else if (tomoDirs[2].isSelected()) visible = (y == tomo_y);
            }
            if (!visible) continue;

            unsigned index3D = x * EL * EL + y * EL + z;
            if (index3D >= EL * EL * EL) continue;

            // Determine color based on cell state
            if (cell.t == cell.d) {
              if (cell.a == W_USED) voxels[index3D] = RGB(255, 0, 0);
              else if (cell.d == 0) voxels[index3D] = RGB(0, 255, 0);
              else voxels[index3D] = RGB(255, 255, 255);
            }

            if (data3D[2].getState() && cell.sB) {
              voxels[index3D] = RGB(0, 255, 255);  // Cyan (spin)
            }
            if (data3D[1].getState() && cell.pB) {
              voxels[index3D] = RGB(255, 255, 0);  // Yellow (momentum)
            }
          }
        }
      }

      ++replayIndex;
      if (replayIndex >= recorder.frames.size()) {
        replayIndex = 0;
        replayTimer -= recorder.frames.size();
      } else {
        ++replayTimer;
      }
    }
  }

  /**
   * Function to show the loading window
   */
  int showLoadingWindow()
  {
    // Randomize
    srand(0);//time(NULL));
    //
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor)
    {
      glfwTerminate();
      return 1;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode)
    {
      glfwTerminate();
      return 1;
    }
    // Calculate the position to center the window
    int xPos = (mode->width - 300) / 2;
    int yPos = (mode->height - 50) / 2;
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    loadingWindow = glfwCreateWindow(300, 50, "Loading...", nullptr, nullptr);
    glfwSetWindowPos(loadingWindow, xPos, yPos); // Set window position
    if (!loadingWindow)
    {
      std::cerr << "Failed to create loading window!" << std::endl;
      glfwTerminate();
      exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(loadingWindow);
    glfwShowWindow(loadingWindow);
    // Set up variables for the ellipsis
    int ellipsisCount = 2;
    int maxEllipsisCount = 12;
    double lastEllipsisChangeTime = glfwGetTime();
    double ellipsisChangeInterval = 0.5; // Change the ellipsis every 0.5 seconds
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, mode->width, 0, mode->height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    int step = 0;
    while (!glfwWindowShouldClose(loadingWindow))
    {
      glfwPollEvents();
      glClear(GL_COLOR_BUFFER_BIT);
      // Render a simple loading indicator (could be text, animation, etc.)
      string loadingMessage = "Loading";
      for (int i = 0; i < ellipsisCount; ++i)
      {
        loadingMessage += ".";
      }
      // Draw the loading message at the center of the screen
      framework::drawString8(loadingMessage, 500, mode->height / 2);
      // Change the ellipsis count if enough time has passed
      if (glfwGetTime() - lastEllipsisChangeTime >= ellipsisChangeInterval)
      {
        ellipsisCount = (ellipsisCount % maxEllipsisCount) + 1;
        lastEllipsisChangeTime = glfwGetTime();
      }
      // Call all initialization routines.
      glfwSwapBuffers(loadingWindow);
      if (automaton::initSimulation(step++))
        break;
      Sleep(200);
    }
    glfwDestroyWindow(loadingWindow);
    return 0;
  }

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
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];
    if (height == 0) height = 1;
    GLfloat ratio = width / (GLfloat) height;
    if (projection[0].isSelected())
    {
      // Orthographic projection
      float orthoSize = 0.6f; // Adjust this value to control zoom
      mRenderer_.setProjection(glm::ortho(         // ← USE setProjection()
        -orthoSize * ratio, orthoSize * ratio,
        -orthoSize, orthoSize,
        0.01f, 100.0f
      ));
    }
    else if (projection[1].isSelected())
    {
      // Perspective projection
      mRenderer_.setProjection(glm::perspective(   // ← USE setProjection()
        glm::radians(45.0f),
        ratio,
        0.01f,
        100.0f
      ));
    }
  }

  /*
   * Processes the mouse buttons.
   */
  const int BOTTOM_UI_ZONE = 120;  // Reserve bottom 120 pixels for UI (horizontal slider area)
  const int LEFT_UI_ZONE = 200;    // Reserve left 200 pixels for UI (vertical slider, checkboxes)

  /*
   * Helper function to check if click is in 3D interaction zone
   */
  bool isIn3DZone(double xpos, double ypos, int windowWidth, int windowHeight)
  {
    // Convert ypos to bottom-origin coordinates
    double bottomY = windowHeight - ypos;
    // Check if we're outside the UI zones
    return (bottomY > BOTTOM_UI_ZONE && xpos > LEFT_UI_ZONE);
  }

  /*
   * Processes the mouse buttons - IMPROVED VERSION
   */
  void CAWindow::buttonCallback(GLFWwindow *window, int button, int action, int mods)
  {
	if (showExitDialog)
	  return;  // ⛔ Ignore mouse input while dialog is active
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    // Convert to OpenGL coordinates consistently (y=0 at bottom)
    switch(action)
    {
        case GLFW_PRESS:
        {
          switch(button)
          {
            case GLFW_MOUSE_BUTTON_LEFT:
            {
              #ifdef DEBUG
              // Store the click position for debug rendering
              debugClickX = xpos;
              debugClickY = ypos;
              showDebugClick = true;
              #endif
              // ==========================================
              // STEP 1: Handle sliders FIRST and check immediately
              // ==========================================
              vslider.onMouseButton(button, action, xpos, ypos, height);
              hslider.onMouseButton(button, action, xpos, ypos, height);
              // If either slider started dragging, don't process anything else TODO
              if (hslider.isDragging() || vslider.isDragging())
                return;
              // ==========================================
              // STEP 2: Handle other UI elements
              // ==========================================
              // Handle checkboxes

              int height;
              glfwGetWindowSize(window, nullptr, &height);
              // Tickboxes
              for (Tickbox& cb : data3D)
                  if (cb.onMouseButton(xpos, ypos, true))
                      return;
              // Handle light frame delays
              for (Tickbox& cb : delays)
              {
                  if (cb.onMouseButton(xpos, ypos, true))
                  {
                      instance().onDelayToggled(&cb);
                      return;
                  }
              }
              // Poke the help checkbox
              scenarioHelpToggle->onMouseButton(xpos, ypos, true);
              // === RADIOS: views ===
              bool viewClicked = false;
              for (size_t i = 0; i < views.size(); ++i)
              {
                  if (views[i].clicked(xpos, ypos))
                  {
                      // Deselect all
                      for (Radio& r : views) r.setSelected(false);
                      views[i].setSelected(true);
                      viewClicked = true;

                      // Update camera
                      float length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                      if (i == 0) // Isometric
                      {
                          instance().mCamera_.setEye(glm::vec3(length, length, length));
                          instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                      }
                      else if (i == 1) // XY
                      {
                          instance().mCamera_.setEye(glm::vec3(0, 0, length));
                          instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                      }
                      else if (i == 2) // YZ
                      {
                          instance().mCamera_.setEye(glm::vec3(length, 0, 0));
                          instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                      }
                      else if (i == 3) // ZX
                      {
                          instance().mCamera_.setEye(glm::vec3(0, length, 0));
                          instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                      }

                      instance().mCamera_.update();
                      instance().mInteractor_.setCamera(&instance().mCamera_);
                      break;  // Only one can be selected
                  }
              }
              if (viewClicked) return;              // Handle the layer list
              list->poll(xpos, ypos);
              bool projClicked = false;
              for (size_t i = 0; i < projection.size(); ++i)
              {
                  if (projection[i].clicked(xpos, ypos))
                  {
                      for (Radio& r : projection) r.setSelected(false);
                      projection[i].setSelected(true);
                      projClicked = true;
                      instance().updateProjection();
                      break;
                  }
              }
              if (projClicked) return;
              // Handle tomography
              if (tomo->onMouseButton(xpos, ypos, true))
              {
                for (Radio& radio : tomoDirs)
                  radio.setSelected(false);
                // Optionally reset directions when turning off tomography
                if (tomo->getState())
                {
                  tomoDirs[0].setSelected(true);
                }
              }
              else if (tomo->getState())
              {
                // Only handle direction selection if tomography mode is active
                for (Radio& radio : tomoDirs)
                {
                  if (radio.clicked(xpos, ypos))
                  {
                    // Select this direction and deselect others
                    for (Radio& r : tomoDirs)
                      r.setSelected(false);
                    radio.setSelected(true);
                    // TODO: call any tomography-related update logic here, e.g.:
                    // instance().onTomoDirectionChanged(&radio);
                    break;
                  }
                }
              }
              // Handle help hyperlink
              const char* linkText = "Help";
              int textWidth = 0;
              const char* c = linkText;
              while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
              int linkX = (width - textWidth) / 2;
              int linkY = height - 30;
              int linkHeight = 15;

              if (xpos >= linkX && xpos <= linkX + textWidth &&
                  ypos >= linkY - linkHeight && ypos-30 <= linkY + 5)
              {
                ShellExecuteA(
            	   NULL,                           // HWND hwnd
            	   "open",                         // LPCSTR lpOperation
            	   "https://github.com/automaton3d/automaton/blob/master/help.md", // LPCSTR lpFile
            	   NULL,                           // LPCSTR lpParameters
            	   NULL,                           // LPCSTR lpDirectory
            	   SW_SHOWNORMAL                   // INT nShowCmd
            	);
                return;  // Don't activate 3D interactor
              }
              // ==========================================
              // STEP 3: Only activate 3D interactor if in valid zone
              // ==========================================
              if (isIn3DZone(xpos, ypos, width, height))
              {
                instance().mInteractor_.setLeftClicked(true);
                instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;
            }
            case GLFW_MOUSE_BUTTON_MIDDLE:
              // Only activate if in 3D zone and no slider dragging
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                instance().mInteractor_.setMiddleClicked(true);
                instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;
            case GLFW_MOUSE_BUTTON_RIGHT:
              // Only activate if in 3D zone and no slider dragging
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                instance().mInteractor_.setRightClicked(true);
                instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;
          }
          break;
        }
        case GLFW_RELEASE:
        {
          switch(button)
          {
            case GLFW_MOUSE_BUTTON_LEFT:
            {
              // Store state before release
              bool wasSliderDragging = hslider.isDragging() || vslider.isDragging();
              // Release sliders
              hslider.onMouseButton(button, action, xpos, ypos, height);
              vslider.onMouseButton(button, action, xpos, ypos, height);
              // Only release 3D interactor if slider wasn't being dragged
              if (!wasSliderDragging)
              {
                instance().mInteractor_.setLeftClicked(false);
              }
              break;
            }
            case GLFW_MOUSE_BUTTON_MIDDLE:
              instance().mInteractor_.setMiddleClicked(false);
              break;
            case GLFW_MOUSE_BUTTON_RIGHT:
              instance().mInteractor_.setRightClicked(false);
              break;
          }
          break;
        }
        default: break;
      }
    }
  void CAWindow::errorCallback(int error, const char* description)
  {
    cerr << description << endl;
  }

  CAWindow & CAWindow::instance()
  {
    static CAWindow i;
    return i;
  }

  void CAWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
      // Debounce timer to prevent double-trigger from GLFW key repeat
      static double lastKeyTime = 0.0;
      double now = glfwGetTime();
      if (now - lastKeyTime < 0.2)
          return;
      lastKeyTime = now;

      // === EXIT POPUP LOGIC ===
      if (showExitDialog)
      {
          if (action == GLFW_PRESS)
          {
              if (key == GLFW_KEY_Y)
              {
                  // Confirm exit
                  glfwSetWindowShouldClose(window, GL_TRUE);
                  showExitDialog = false;
              }
              else if (key == GLFW_KEY_N || key == GLFW_KEY_ESCAPE)
              {
                  // Cancel exit
                  showExitDialog = false;
              }
          }
          return; // ⛔ Block all other key input when popup is active
      }

      // === NORMAL KEY HANDLING ===
      if (action == GLFW_PRESS)
      {
          float length;
          switch (key)
          {
              case GLFW_KEY_ESCAPE:
                  // Show the popup immediately
                  showExitDialog = true;
                  instance().pendingExit = false;
                  break;

              case GLFW_KEY_LEFT_CONTROL:
              case GLFW_KEY_RIGHT_CONTROL:
                  instance().mInteractor_.setSpeed(5.f);
                  break;

              case GLFW_KEY_LEFT_SHIFT:
              case GLFW_KEY_RIGHT_SHIFT:
                  instance().mInteractor_.setSpeed(0.1f);
                  break;

              case GLFW_KEY_F2:
                  instance().mAnimator_.setAnimation(Animator::ORBIT);
                  break;

              case GLFW_KEY_F5:
                  if (!replayFrames)
                      recordFrames = !recordFrames;
                  break;

              case GLFW_KEY_F6:
                  replayFrames = !replayFrames;
                  replayIndex = 0;
                  recordFrames = false;
                  break;

              case GLFW_KEY_F7:
                  if (!recordFrames && !replayFrames)
                  {
                      recorder.saveToFile("frames.dat");
                      savePopup = true;
                  }
                  break;

              case GLFW_KEY_F8:
                  if (!recordFrames && !replayFrames)
                  {
                      recorder.loadFromFile("frames.dat");
                      timer = recorder.savedTimer;
                      automaton::scenario = recorder.savedScenario;
                      loadPopup = true;
                  }
                  break;

              case GLFW_KEY_C:
                  std::cout << "\nCamera view:"
                            << " (" << instance().mCamera_.getEye().x
                            << "," << instance().mCamera_.getEye().y
                            << "," << instance().mCamera_.getEye().z << ") "
                            << "(" << instance().mCamera_.getCenter().x
                            << "," << instance().mCamera_.getCenter().y
                            << "," << instance().mCamera_.getCenter().z << ") "
                            << "(" << instance().mCamera_.getUp().x
                            << "," << instance().mCamera_.getUp().y
                            << "," << instance().mCamera_.getUp().z << ")\n\n";
                  break;

              case GLFW_KEY_R:
                  instance().mCamera_.reset();
                  instance().mInteractor_.setCamera(&instance().mCamera_);
                  break;

              case GLFW_KEY_T:
                  if (instance().mInteractor_.getMotionRightClick() == TrackBallInteractor::FIRSTPERSON)
                      instance().mInteractor_.setMotionRightClick(TrackBallInteractor::PAN);
                  else
                      instance().mInteractor_.setMotionRightClick(TrackBallInteractor::FIRSTPERSON);
                  break;

              case GLFW_KEY_X:
                  length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                  instance().mCamera_.setEye(glm::vec3(length, 0, 0));
                  instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                  instance().mCamera_.update();
                  instance().mInteractor_.setCamera(&instance().mCamera_);
                  break;

              case GLFW_KEY_Y:
                  length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                  instance().mCamera_.setEye(glm::vec3(0, length, 0));
                  instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                  instance().mCamera_.update();
                  instance().mInteractor_.setCamera(&instance().mCamera_);
                  break;

              case GLFW_KEY_Z:
                  length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                  instance().mCamera_.setEye(glm::vec3(0, 0, length));
                  instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                  instance().mCamera_.update();
                  instance().mInteractor_.setCamera(&instance().mCamera_);
                  break;

              case GLFW_KEY_O:
                  length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                  instance().mCamera_.setEye(glm::vec3(length, length, length));
                  instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                  instance().mCamera_.update();
                  instance().mInteractor_.setCamera(&instance().mCamera_);
                  break;

              case GLFW_KEY_M:
                  sound(false);
                  break;

              case GLFW_KEY_P:
                  pause = !pause;
                  break;

              case GLFW_KEY_F1:
                  showHelp = !showHelp;
                  break;

              default:
                  break;
          }
      }
      else if (action == GLFW_RELEASE)
      {
          switch (key)
          {
              case GLFW_KEY_LEFT_CONTROL:
              case GLFW_KEY_RIGHT_CONTROL:
              case GLFW_KEY_LEFT_SHIFT:
              case GLFW_KEY_RIGHT_SHIFT:
                  instance().mInteractor_.setSpeed(1.f);
                  break;

              default:
                  break;
          }
      }
  }

  /*
   * Processes mouse movement - IMPROVED VERSION
   */
  void CAWindow::moveCallback(GLFWwindow* window, double xpos, double ypos)
  {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    // Always update sliders if they're being dragged
    hslider.onMouseDrag((int)xpos, (int)ypos, height);
    vslider.onMouseDrag((int)xpos, (int)ypos, height);
    hslider.onMouseMove((int)xpos, ypos);
    // Check if mouse is over the Help hyperlink
    const char* linkText = "Help";
    int textWidth = 0;
    const char* c = linkText;
    while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
     int linkX = (width - textWidth) / 2;
       int linkY = height - 30;
       int linkHeight = 15;
       helpHover = (xpos >= linkX && xpos <= linkX + textWidth &&
                    ypos >= linkY - linkHeight && ypos <= linkY + 5);

       // Only update trackball if NOT dragging sliders AND in 3D zone
       if (!hslider.isDragging() && !vslider.isDragging() &&
           isIn3DZone(xpos, ypos, width, height))
       {
         instance().mInteractor_.setClickPoint(xpos, ypos);
       }
     }

  /*
   * Processes scroll wheel - IMPROVED VERSION
   */
  void CAWindow::scrollCallback(GLFWwindow *window, double xpos, double ypos)
  {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    // Only allow scroll zoom if in 3D zone and no sliders being dragged
    if (!hslider.isDragging() && !vslider.isDragging() &&
        isIn3DZone(mouseX, mouseY, width, height))
    {
      instance().mInteractor_.setScrollDirection(xpos + ypos > 0 ? true : false);
    }
  }

  void CAWindow::sizeCallback(GLFWwindow *window, int width, int height)
  {
    instance().mRenderer_.resize(width, height);
    instance().mInteractor_.setScreenSize(width, height);
    instance().mAnimator_.setScreenSize(width, height);
  }

  /*
   * Runs the simulation.
   * (independent of the GUI thread)
   */
  DWORD WINAPI CAWindow::SimulateThread(LPVOID lpParam)
  {
    puts("Simulation thread launched...");
    static_cast<CAWindow*>(lpParam)->isThreadReady_ = true;
    Sleep(100);
    // Prepare the mirror grid before starting
    automaton::swap_lattices();
    while (true)
    {
      if (!pause)
      {
        if (replayFrames)
        {
          updateReplay();
        }
        else
        {
          automaton::simulation();
          if (recordFrames)
            recorder.recordFrame(automaton::lattice_curr, timer, automaton::scenario);
          automaton::updateBuffer();
          framework::timer++;
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

    return 0;
  }

  /*
   * Runs the graphics.
   * This is the GUI loop, but the independent simulation loop thread is launched here.
   */
  int CAWindow::run()
  {
  // Launch the loading window
    showLoadingWindow();
    // Get the primary monitor's dimensions
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor)
    {
      glfwTerminate();
        return 1;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode)
    {
      glfwTerminate();
        return 1;
    }
    int width = mode->width;
  int height = mode->height;
    // Create a borderless GLFW window and center it on the screen
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // Set up OpenGL for basic text rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    puts("GUI thread launched...");
    glfwSetErrorCallback(& CAWindow::errorCallback);
    mWindow_ = glfwCreateWindow(width, height, "Cellular automaton", NULL, NULL);
    if (!mWindow_)
    {
      glfwTerminate();
      perror("GUI window failed.");
      return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(mWindow_);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(mWindow_, & CAWindow::moveCallback);
    glfwSetKeyCallback(mWindow_, & CAWindow::keyCallback);
    glfwSetMouseButtonCallback(mWindow_, & CAWindow::buttonCallback);
    glfwSetScrollCallback(mWindow_, & CAWindow::scrollCallback);
    glfwSetWindowSizeCallback(mWindow_, &CAWindow::sizeCallback);
    mInteractor_.setCamera(& mCamera_);
    mRenderer_.setCamera(& mCamera_);
    mAnimator_.setInteractor(& mInteractor_);
    mRenderer_.init();
    sizeCallback(mWindow_, width, height); // Set initial size.
    // Isometric view
    int length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
    instance().mCamera_.setEye(glm::vec3(length, length, length));
    instance().mCamera_.setUp(glm::vec3(0, 1, 0));
    instance().mCamera_.update();
    instance().mInteractor_.setCamera(& instance().mCamera_);
    // Launch the simulation thread
    DWORD dwThreadId;
    HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, this, 0, &dwThreadId);
    CloseHandle(hSimulateThread);
    // Enter the visualization loop
    while (!glfwWindowShouldClose(mWindow_))
    {
      if (!logoCreated)
      {
      logo = new framework::Logo("logo.png");  // Context is now current
      logoCreated = true;
      }
      if (pendingExit)
      {
        pendingExit = false;
        showExitDialog = true;
      }
      mRenderer_.render();
      if (toastActive)
      {
        double now = glfwGetTime();
        if (now - toastStartTime < 3.0)  // Show for 3 seconds
        {
          glMatrixMode(GL_PROJECTION);
          glPushMatrix();
          glLoadIdentity();
          glOrtho(0, width, 0, height, -1, 1);
          glMatrixMode(GL_MODELVIEW);
          glPushMatrix();
          glLoadIdentity();
          glDisable(GL_DEPTH_TEST);

          // Toast layout
          const int boxWidth = 250;
          const int boxHeight = 40;

          int boxLeft   = (width - boxWidth) / 2;
          int boxRight  = boxLeft + boxWidth;
          int boxBottom = (height - boxHeight) / 2;
          int boxTop    = boxBottom + boxHeight;

          // Toast background
          glColor3f(0.3f, 0.3f, 0.3f);  // Green
          glBegin(GL_QUADS);
            glVertex2f(boxLeft, boxBottom);
            glVertex2f(boxRight, boxBottom);
            glVertex2f(boxRight, boxTop);
            glVertex2f(boxLeft, boxTop);
          glEnd();

          // Toast text
          glColor3f(1.0f, 1.0f, 0);
          drawString8(toastMessage.c_str(), boxLeft + 20, boxBottom + 15);

          glEnable(GL_DEPTH_TEST);
          glPopMatrix();
          glMatrixMode(GL_PROJECTION);
          glPopMatrix();
        }
        else
        {
          toastActive = false;
        }
      }
      glfwSwapBuffers(mWindow_);
      mInteractor_.update();
      if (!pause)
      {
      mAnimator_.animate();
      }
      else
      {
      this_thread::sleep_for(std::chrono::milliseconds(16));
      }
      glfwPollEvents();
      if (savePopup)
      {
        toastMessage = "Frames saved successfully.";
        toastStartTime = glfwGetTime();
        toastActive = true;
        savePopup = false;
      }
      if (loadPopup)
      {
        toastMessage = "Frames loaded successfully.";
        toastStartTime = glfwGetTime();
        toastActive = true;
        loadPopup = false;
      }
    }
    puts("GUI thread ended.");
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
    // Optionally update renderer, animation speed, etc.
  }

} // end namespace framework


/************************************
 *                                  *
 *     Simulation entry point.      *
 *     (Called by the Splash)       *
 *                                  *
 ************************************/
int runSimulation(int scenario, bool paused)
{
  automaton::scenario = scenario;
  framework::pause = paused;
  Beep(1000, 80);
  glfwInit();
  char arg0[] = "test";
  char *argv[] = { arg0, NULL };
  int argc = 1;
  glutInit(&argc, argv);
  bool status = framework::CAWindow::instance().run();
  glfwTerminate();
  return status;
}
