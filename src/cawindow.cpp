/*
 * window.cpp
 *
 * processes IO, runs the simulation, and renders graphics for it.
 *
 * This is the top file of the program.
 */

#include "GUI.h"
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <cawindow.h>
#include <thread>
#include <chrono>
#include <commdlg.h>
#include "model/simulation.h"
#include "hslider.h"
#include "vslider.h"
#include "logo.h"
#include <recorder.h>
#include "text.h"
#include "replay_progress.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>         // for glm::make_mat4
#include "projection.h"
#include "GUI.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

namespace automaton
{
  extern int scenario;
}

namespace framework
{
  void requestExit();
  using namespace automaton;

  #ifdef DEBUG
  double debugClickX = -1;
  double debugClickY = -1;
  bool showDebugClick = false;
  #endif

  using namespace std;
  GLFWwindow* loadingWindow = nullptr;
  bool pause = false;
  void saveReplay();
  void loadReplay();

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
  extern ReplayProgressBar *replayProgress;
  extern GLint gViewport[4];
  extern int vis_dx;
  extern int vis_dy;
  extern int vis_dz;

  // Logo

  Logo *logo = nullptr;

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

  // dot dragg
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

          // ✅ NEW: Update replay progress bar
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
    glOrtho(0, mode->width, mode->height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Execute all initialization steps

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
      drawString(loadingMessage, 500, mode->height / 2, 8);
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

    // Close the loading window

    glfwDestroyWindow(loadingWindow);
    return 0;
  }

  /**
   * Opens a Save File dialog and returns the chosen filename
   * Returns empty string if user cancels
   */
  std::string getSaveFileName()
  {
      OPENFILENAMEA ofn;
      char szFile[260] = {0};

      // ✅ Get the native Windows handle from GLFW window
      HWND hwnd = glfwGetWin32Window(CAWindow::instance().getWindow());

      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = hwnd;  // ✅ Set the owner window
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = "Replay Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0";
      ofn.nFilterIndex = 1;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;
      ofn.lpstrInitialDir = NULL;
      ofn.lpstrTitle = "Save Replay As";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
      ofn.lpstrDefExt = "dat";

      if (GetSaveFileNameA(&ofn) == TRUE)
      {
          return std::string(ofn.lpstrFile);
      }
      return "";
  }

  /**
   * Opens an Open File dialog and returns the chosen filename
   * Returns empty string if user cancels
   */
  std::string getOpenFileName()
  {
      OPENFILENAMEA ofn;
      char szFile[260] = {0};

      // ✅ Get the native Windows handle from GLFW window
      HWND hwnd = glfwGetWin32Window(CAWindow::instance().getWindow());

      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = hwnd;  // ✅ Set the owner window
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = "Replay Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0";
      ofn.nFilterIndex = 1;
      ofn.lpstrFileTitle = NULL;
      ofn.nMaxFileTitle = 0;
      ofn.lpstrInitialDir = NULL;
      ofn.lpstrTitle = "Open Replay";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

      if (GetOpenFileNameA(&ofn) == TRUE)
      {
          return std::string(ofn.lpstrFile);
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
                      automaton::scenario = recorder.savedScenario;
                      loadPopup = true;
                  }
                  catch (const std::exception& e) {
                      toastMessage = "Failed to load replay: " + std::string(e.what());
                      toastStartTime = glfwGetTime();
                      toastActive = true;
                  }
              }
          }
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
    int width = gViewport[2];
    int height = gViewport[3];
    if (height == 0) height = 1;
    GLfloat ratio = width / (GLfloat) height;
    if (projection[0].isSelected())
    {
      // Orthographic projection
      mRenderer_.setProjection(glm::perspective(   // ← USE setProjection()
          glm::radians(45.0f),
          ratio,
          0.01f,
          100.0f
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

  void CAWindow::buttonCallback(GLFWwindow *window, int button, int action, int mods)
  {
      if (showExitDialog) return;

      int width, height;
      glfwGetWindowSize(window, &width, &height);
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);

      switch (action)
      {
      case GLFW_PRESS:
          switch (button)
          {
          case GLFW_MOUSE_BUTTON_LEFT:
          {
  #ifdef DEBUG
              debugClickX = xpos;
              debugClickY = ypos;
              showDebugClick = true;
  #endif

              // --- NEW: IMPROVED MENU CLICK HANDLING ---
              int menuWidth = gViewport[2];
              int menuHeight = gViewport[3];
              bool fileMenuWasOpen = fileMenu && fileMenu->isOpen_;
              bool helpMenuWasOpen = helpMenu && helpMenu->isOpen_;
              bool itemSelected = false;

              // Try to handle click on File menu
              if (fileMenu) {
                  if (fileMenu->handleClick((int)xpos, (int)ypos, menuWidth, menuHeight)) {
                      itemSelected = true;
                  }
                  if (itemSelected) {
                      if (helpMenu) helpMenu->close();
                      return;
                  }
              }

              // Try to handle click on Help menu
              if (helpMenu) {
                  if (helpMenu->handleClick((int)xpos, (int)ypos, menuWidth, menuHeight)) {
                      itemSelected = true;
                  }
                  if (itemSelected) {
                      if (fileMenu) fileMenu->close();
                      return;
                  }
              }

              // Click-outside-to-close logic
              bool clickOnMenuArea = (fileMenu && fileMenu->isMouseOver((int)xpos, (int)ypos, width, height)) ||
                                     (helpMenu && helpMenu->isMouseOver((int)xpos, (int)ypos, width, height));

              if (fileMenuWasOpen || helpMenuWasOpen) {
                  if (!clickOnMenuArea) {
                      if (fileMenu) fileMenu->close();
                      if (helpMenu) helpMenu->close();
                  }
              }
              // --- END NEW MENU LOGIC ---

              // === OLD: HANDLE SLIDERS ===
              vslider.onMouseButton(button, action, xpos, ypos, height);
              hslider.onMouseButton(button, action, xpos, ypos, height);
              if (hslider.isDragging() || vslider.isDragging())
                  return;

              // === OLD: HANDLE TICKBOXES ===
              for (Tickbox& cb : data3D)
                  if (cb.onMouseButton(xpos, ypos, true))
                      return;

              for (Tickbox& cb : delays)
              {
                  if (cb.onMouseButton(xpos, ypos, true))
                  {
                      instance().onDelayToggled(&cb);
                      return;
                  }
              }

              scenarioHelpToggle->onMouseButton(xpos, ypos, true);

              // === OLD: HANDLE VIEW RADIOS ===
              bool viewClicked = false;
              for (size_t i = 0; i < views.size(); ++i)
              {
                  if (views[i].clicked(xpos, ypos))
                  {
                      for (Radio& r : views) r.setSelected(false);
                      views[i].setSelected(true);
                      viewClicked = true;

                      float length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
                      if (i == 0) { // Isometric
                          instance().mCamera_.setEye(glm::vec3(length, length, length));
                          instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                      } else if (i == 1) { // XY
                          instance().mCamera_.setEye(glm::vec3(0, 0, length));
                          instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                      } else if (i == 2) { // YZ
                          instance().mCamera_.setEye(glm::vec3(length, 0, 0));
                          instance().mCamera_.setUp(glm::vec3(0, 1, 0));
                      } else if (i == 3) { // ZX
                          instance().mCamera_.setEye(glm::vec3(0, length, 0));
                          instance().mCamera_.setUp(glm::vec3(1, 0, 0));
                      }

                      instance().mCamera_.update();
                      instance().mInteractor_.setCamera(&instance().mCamera_);
                      break;
                  }
              }
              if (viewClicked) return;

              // === OLD: HANDLE LAYER LIST ===
              layerList->poll(xpos, ypos);

              // === OLD: HANDLE PROJECTION RADIOS ===
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

              // === OLD: HANDLE TOMOGRAPHY ===
              if (tomo->onMouseButton(xpos, ypos, true))
              {
                  for (Radio& radio : tomoDirs)
                      radio.setSelected(false);
                  if (tomo->getState())
                      tomoDirs[0].setSelected(true);
              }
              else if (tomo->getState())
              {
                  for (Radio& radio : tomoDirs)
                  {
                      if (radio.clicked(xpos, ypos))
                      {
                          for (Radio& r : tomoDirs) r.setSelected(false);
                          radio.setSelected(true);
                          break;
                      }
                  }
              }

              // === OLD: HANDLE HELP HYPERLINK ===
              const char* linkText = "Help";
              int textWidth = 0;
              const char* c = linkText;
              while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
              int linkX = (width - textWidth) / 2;
              int linkY = height - 30;
              int linkHeight = 15;

              if (xpos >= linkX && xpos <= linkX + textWidth &&
                  ypos >= linkY - linkHeight && ypos <= linkY + 5)
              {
                  ShellExecuteA(NULL, "open",
                      "https://github.com/automaton3d/automaton/blob/master/help.md",
                      NULL, NULL, SW_SHOWNORMAL);
                  return;
              }

              // === NEW: IMPROVED AXIS THUMB DETECTION ===
              if ((pause || GUImode == REPLAY))
              {
              if (!gAxisProjValid) {
                  // No cached projection; fall back to 3D interactor
                  if (!thumb.active && isIn3DZone(xpos, ypos, width, height)) {
                      instance().mInteractor_.setLeftClicked(true);
                      instance().mInteractor_.setClickPoint(xpos, ypos);
                  }
                  break;
              }

              float clickX = (float)xpos;
              float clickY = (float)(height - ypos); // bottom-origin

              auto distAndT = [](float px, float py, const AxisProjection& ap, float &tOut) {
                  float dx = ap.x1 - ap.x0, dy = ap.y1 - ap.y0;
                  float denom = dx*dx + dy*dy;
                  if (denom <= 0.0f) { tOut = 0.0f; return std::hypot(px - ap.x0, py - ap.y0); }
                  float t = ((px - ap.x0)*dx + (py - ap.y0)*dy) / denom;
                  t = glm::clamp(t, 0.0f, 1.0f);
                  tOut = t;
                  float cx = ap.x0 + t*dx, cy = ap.y0 + t*dy;
                  return std::hypot(px - cx, py - cy);
              };

              float tX=0.0f, tY=0.0f, tZ=0.0f;
              float dX = distAndT(clickX, clickY, gAxisProj[0], tX);
              float dY = distAndT(clickX, clickY, gAxisProj[1], tY);
              float dZ = distAndT(clickX, clickY, gAxisProj[2], tZ);

              const float thresholdPx = 12.0f;
              int chosenAxis = -1;
              float t = 0.0f;
              float minDist = thresholdPx;

              if (dX < minDist) { minDist = dX; chosenAxis = 0; t = tX; }
              if (dY < minDist) { minDist = dY; chosenAxis = 1; t = tY; }
              if (dZ < minDist) { minDist = dZ; chosenAxis = 2; t = tZ; }

              if (chosenAxis != -1) {
                  thumb.active   = true;
                  thumb.axis     = chosenAxis;
                  thumb.position = t * axisLength;
                  thumb.initialPosition = t * axisLength;
                  thumb.dragging = true;


                  thumb.startOffset[0] = vis_dx;
                  thumb.startOffset[1] = vis_dy;
                  thumb.startOffset[2] = vis_dz;

                  return;
              }
              }
              // === END NEW AXIS LOGIC ===

              // === ACTIVATE 3D INTERACTOR ===
              if (!thumb.active && isIn3DZone(xpos, ypos, width, height)) {
                  instance().mInteractor_.setLeftClicked(true);
                  instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;
          }

          case GLFW_MOUSE_BUTTON_MIDDLE:
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                  instance().mInteractor_.setMiddleClicked(true);
                  instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;

          case GLFW_MOUSE_BUTTON_RIGHT:
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                  instance().mInteractor_.setRightClicked(true);
                  instance().mInteractor_.setClickPoint(xpos, ypos);
              }
              break;
          }
          break;

      case GLFW_RELEASE:
          switch (button)
          {
          case GLFW_MOUSE_BUTTON_LEFT:
          {
              // OLD: Preserve slider-aware release logic
              bool wasSliderDragging = hslider.isDragging() || vslider.isDragging();
              hslider.onMouseButton(button, action, xpos, ypos, height);
              vslider.onMouseButton(button, action, xpos, ypos, height);
              if (!wasSliderDragging)
                  instance().mInteractor_.setLeftClicked(false);
              thumb.active = false;
              thumb.dragging = false;
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

      default: break;
      }
  }

  void CAWindow::moveCallback(GLFWwindow* window, double xpos, double ypos)
  {
      int width, height;
      glfwGetWindowSize(window, &width, &height);

      // === UPDATE DROPDOWN HOVER ===
      if (framework::fileMenu) {
          framework::fileMenu->updateHover(xpos, ypos, width, height);
      }
      if (framework::helpMenu) {
        framework::helpMenu->updateHover(xpos, ypos, width, height);
      }
      // === END HOVER UPDATE ===

      // --- MENU AUTO-CLOSE LOGIC ---
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
      {
          if (framework::fileMenu && framework::fileMenu->isOpen_)
          {
              if (!framework::fileMenu->isMouseOver((int)xpos, (int)ypos, width, height))
                  framework::fileMenu->close();
          }
          if (framework::helpMenu && framework::helpMenu->isOpen_)
          {
              if (!framework::helpMenu->isMouseOver((int)xpos, (int)ypos, width, height))
                  framework::helpMenu->close();
          }
      }
      // --- END NEW MENU LOGIC ---

      // === HANDLE SLIDER DRAGGING ===
      hslider.onMouseDrag((int)xpos, (int)ypos, height);
      vslider.onMouseDrag((int)xpos, (int)ypos, height);
      hslider.onMouseMove((int)xpos, ypos);

      // === HOVER DETECTION FOR HELP HYPERLINK ===
      const char* linkText = "Help";
      int textWidth = 0;
      const char* c = linkText;
      while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
      int linkX = (width - textWidth) / 2;
      int linkY = height - 30;
      int linkHeight = 15;
      helpHover = (xpos >= linkX && xpos <= linkX + textWidth &&
                   ypos >= linkY - linkHeight && ypos <= linkY + 5);

      // === TRACKBALL UPDATE IF IN 3D ZONE ===
      if (!hslider.isDragging() && !vslider.isDragging() &&
        isIn3DZone(xpos, ypos, width, height))
      {
        instance().mInteractor_.setClickPoint(xpos, ypos);
      }

      // === AXIS THUMB DRAGGING ===
      if ((pause || GUImode == REPLAY) && thumb.active && thumb.dragging)
      {
          if (!gAxisProjValid) return;

          float x0 = gAxisProj[thumb.axis].x0;
          float y0 = gAxisProj[thumb.axis].y0;
          float x1 = gAxisProj[thumb.axis].x1;
          float y1 = gAxisProj[thumb.axis].y1;

          float mx = static_cast<float>(xpos);
          float my = static_cast<float>(height - ypos);

          auto relPos = [](float px, float py, float ax, float ay, float bx, float by) {
              float dx = bx - ax, dy = by - ay;
              float denom = dx*dx + dy*dy;
              if (denom <= 0.0f) return 0.0f;
              float t = ((px - ax) * dx + (py - ay) * dy) / denom;
              return glm::clamp(t, 0.0f, 1.0f);
          };

          float t = relPos(mx, my, x0, y0, x1, y1);
          thumb.position = t * axisLength;


          // Calculate the change from initial grab position
          float deltaPosition = thumb.position - thumb.initialPosition;
          int deltaOffset = static_cast<int>(round((deltaPosition / axisLength) * automaton::EL));

          // Apply delta relative to starting offsets
          if (thumb.axis == 0) vis_dx = thumb.startOffset[0] + deltaOffset;
          if (thumb.axis == 1) vis_dy = thumb.startOffset[1] + deltaOffset;
          if (thumb.axis == 2) vis_dz = thumb.startOffset[2] + deltaOffset;
      }
  }

  /*
   * Processes scroll wheel - IMPROVED VERSION
   */
  void CAWindow::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
  {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Check if scrolling over an open dropdown menu
    if (framework::fileMenu && framework::fileMenu->isOpen_)
    {
        if (framework::fileMenu->containsDropdown((int)mouseX, (int)mouseY, width, height))
        {
            framework::fileMenu->scroll(yoffset > 0 ? -1 : 1);
            return;
        }
    }

    if (framework::helpMenu && framework::helpMenu->isOpen_)
    {
        if (framework::helpMenu->containsDropdown((int)mouseX, (int)mouseY, width, height))
        {
            framework::helpMenu->scroll(yoffset > 0 ? -1 : 1);
            return;
        }
    }

    // Continue with existing scroll handling
    if (!hslider.isDragging() && !vslider.isDragging() &&
        isIn3DZone(mouseX, mouseY, width, height))
    {
      instance().mInteractor_.setScrollDirection(xoffset + yoffset > 0 ? true : false);
    }
  }

  void CAWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
      // Debounce timer to prevent double-trigger from GLFW key repeat
      static double lastKeyTime = 0.0;
      double now = glfwGetTime();
      if (now - lastKeyTime < 0.2)
          return;
      lastKeyTime = now;
      // === NORMAL KEY HANDLING ===
      if (action == GLFW_PRESS)
      {
        float length;
        switch (key)
        {
          case GLFW_KEY_ESCAPE:
          {
            // ✅ Use shared function
            framework::requestExit();
            break;
          }
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
              if (GUImode == SIMULATION && !replayFrames) {
                  if (!recordFrames) {
                      // Starting recording - ensure it's enabled
                      recorder.recordingEnabled_ = true;
                  }
                  recordFrames = !recordFrames;
              }
              break;

          case GLFW_KEY_F6:
              if (GUImode == REPLAY)
              {
                  replayFrames = !replayFrames;
                  replayIndex = 0;
                  recordFrames = false;

                  // ✅ Reset progress bar
                  if (framework::replayProgress)
                      framework::replayProgress->update(0, recorder.frames.size());
              }
              break;

          case GLFW_KEY_F7:
              saveReplay();  // ✅ Use the new function
              break;

          case GLFW_KEY_F8:
              loadReplay();  // ✅ Use the new function
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
              // ✅ Reset offsets when resuming simulation
              if (!pause && GUImode == SIMULATION) {
                  vis_dx = 0;
                  vis_dy = 0;
                  vis_dz = 0;
              }
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


  void CAWindow::sizeCallback(GLFWwindow *window, int width, int height)
  {
      instance().mRenderer_.resize(width, height);
      instance().mInteractor_.setScreenSize(width, height);
      instance().mAnimator_.setScreenSize(width, height);

      // ✅ Update replay progress bar position
      if (framework::replayProgress)
          framework::replayProgress->setPosition(width, height, 100);
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
          if (updateReplay())
          {
            framework::timer++;
            Sleep(250);
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
    // Make sure to register it in your initialization
    mInteractor_.setCamera(& mCamera_);
    mRenderer_.setCamera(& mCamera_);
    mAnimator_.setInteractor(& mInteractor_);
    mRenderer_.init();
    logo = new Logo("logo.png");
    sizeCallback(mWindow_, width, height); // Set initial size.
    // Isometric view
    int length = glm::length(instance().mCamera_.getEye() - instance().mCamera_.getCenter());
    instance().mCamera_.setEye(glm::vec3(length, length, length));
    instance().mCamera_.setUp(glm::vec3(0, 1, 0));
    instance().mCamera_.update();
    instance().mInteractor_.setCamera(& instance().mCamera_);
    mRenderer_.clearVoxels();
    // Launch the simulation thread
    DWORD dwThreadId;
    HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, this, 0, &dwThreadId);
    CloseHandle(hSimulateThread);
    // Enter the visualization loop
    while (!glfwWindowShouldClose(mWindow_))
    {
      if (pendingExit)
      {
        pendingExit = false;
        showExitDialog = true;
      }
      mRenderer_.render();
      glDisable(GL_DEPTH_TEST);
      glDepthMask(GL_FALSE);
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
          drawString(toastMessage.c_str(), boxLeft + 20, boxBottom + 15, 8);

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
      glEnable(GL_DEPTH_TEST);
      glDepthMask(GL_TRUE);
      glfwSwapBuffers(mWindow_);
      mRenderer_.handleMenuSelection();
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
 *   Simulation mode entry point.   *
 *     (Called by the Splash)       *
 *                                  *
 ************************************/
int runSimulation(int scenario, bool paused)
{
  automaton::scenario = scenario;
  framework::pause = paused;
  Beep(1000, 80);
  framework::GUImode = framework::SIMULATION;
  glfwInit();
  char arg0[] = "test";
  char *argv[] = { arg0, NULL };
  int argc = 1;
  glutInit(&argc, argv);
  bool status = framework::CAWindow::instance().run();
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
  Beep(1000, 80);
  automaton::scenario = -1;
  framework::GUImode = framework::REPLAY;
  glfwInit();
  char arg0[] = "test";
  char *argv[] = { arg0, NULL };
  int argc = 1;
  glutInit(&argc, argv);
  bool status = framework::CAWindow::instance().run();
  glfwTerminate();
  return status;
}
