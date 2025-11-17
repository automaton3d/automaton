/*
 * GUI.cpp
 *
 * Implements the GUIrenderer orchestration logic.
 */

#include "GUI.h"
#include <GL/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include "hslider.h"
#include "replay_progress.h"
#include "logo.h"
#include "cawindow.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "GUI.h"

namespace framework
{
  extern bool showHelp;

  bool MULTICUBE_MODE = false;
  int GUImode = SIMULATION;

  unsigned tomo_x = 0;
  unsigned tomo_y = 0;
  unsigned tomo_z = 0;
  Dropdown* fileMenu = nullptr;
  Dropdown* helpMenu = nullptr;
  bool showAboutDialog = false;

  using namespace std;
  using namespace automaton;

  // Global viewport info
  GLint gViewport[4] = {0, 0, 1920, 1080}; // default values, overwritten on resize

  // Core GUI elements
  vector<Tickbox> data3D;
  vector<Tickbox> delays;
  vector<Radio> views;
  vector<Radio> projection;
  std::unique_ptr<LayerList> layerList;
  HSlider hslider(0, 0, 0, 0, 0);
  VSlider vslider(1890, 93, 10.0f, 607.0f, 30.0f);
  Tickbox *tomo = new Tickbox(50, 840, "Enable");
  vector<Radio> tomoDirs;
  ProgressBar *progress = nullptr;
  ReplayProgressBar *replayProgress = nullptr;

  // Auxiliary
  unsigned long tbegin = 0;
  int barWidths[3] = {0, 0, 0};
  bool helpHover = false;
  bool showScenarioHelp = false;
  Tickbox* scenarioHelpToggle = nullptr;

  void initializeWidgets(); // defined in GUI_InitWidgets.cpp
  void requestExit();
  void showAboutWindow();
  void saveReplay();
  void loadReplay();

  // --------------------------------------------
  // GUIrenderer implementation
  // --------------------------------------------

  GUIrenderer::GUIrenderer() : Renderer() {}

  // Add destructor cleanup
   GUIrenderer::~GUIrenderer()
   {
       delete fileMenu;
       delete helpMenu;
   }

  void GUIrenderer::init()
  {
      initializeWidgets();
      initMenuDropdowns();
  }

  void GUIrenderer::initMenuDropdowns()
   {
       std::vector<std::string> fileOptions = {
           "Save replay",
           "Load replay",
           "Exit"
       };

       std::vector<std::string> helpOptions = {
           "GUI help",
           "Documentation",
           "About"
       };
       fileMenu = new Dropdown(10, 0, 80, 30, fileOptions, "File");
       helpMenu = new Dropdown(100, 0, 80, 30, helpOptions, "Help");
   }


  void requestExit();
  void showAboutWindow();

  void GUIrenderer::handleMenuSelection()
  {
      if (!fileMenu || !helpMenu) return;

      // Process File menu only on fresh selection
      if (fileMenu->wasJustSelected())
      {
          std::string selected = fileMenu->getSelectedItem();
          fileMenu->clearSelection();  // Clear AFTER reading the selected item

          if (selected == "Save replay")
          {
        	  saveReplay();  // ✅ Call the function
          }
          else if (selected == "Load replay")
          {
        	  loadReplay();  // ✅ Call the function
          }
          else if (selected == "Exit")
          {
        	  requestExit();
          }
      }

      // Process Help menu only on fresh selection
      if (helpMenu->wasJustSelected())
      {
          std::string selected = helpMenu->getSelectedItem();
          helpMenu->clearSelection();  // Clear AFTER reading the selected item

          if (selected == "GUI help")
          {
              // Toggle once on actual selection; no frame-by-frame repeated toggles
              showHelp = !showHelp;
          }
          else if (selected == "Documentation")
          {
              std::cout << "Documentation selected" << std::endl;
          }
          else if (selected == "About")
          {
        	  showAboutWindow();
          }
      }
  }

  void GUIrenderer::render()
  {
      renderClear();
      renderObjects();
      renderUI();

  #ifdef DEBUG
      extern bool showDebugClick;
      extern double debugClickX;
      extern double debugClickY;
      if (showDebugClick)
      {
          glColor3f(1.0f, 0.0f, 0.0f);
          glPointSize(6.0f);
          glBegin(GL_POINTS);
          glVertex2f(debugClickX, debugClickY);
          glEnd();
      }
  #endif
  }

  void GUIrenderer::renderClear()
  {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClearDepth(1.0f);
  }

  void GUIrenderer::resize(int width, int height)
  {
      if (height == 0) height = 1;
      GLfloat ratio = width / (GLfloat) height;

      glViewport(0, 0, width, height);

      gViewport[0] = 0;
      gViewport[1] = 0;
      gViewport[2] = width;
      gViewport[3] = height;

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      if (projection[0].isSelected())
      {
          float orthoSize = 0.6f;
          mProjection_ = glm::ortho(
              -orthoSize * ratio, orthoSize * ratio,
              -orthoSize, orthoSize,
              0.01f, 100.0f
          );
      }
      else
      {
          mProjection_ = glm::perspective(glm::radians(45.0f), ratio, .01f, 100.f);
      }
  }

  // ✅ NEW: Shared exit confirmation function
  void requestExit()
  {
      std::thread([]() {
          int result = MessageBoxW(
              NULL,
              L"Are you sure you want to exit?",
              L"Exit Confirmation",
              MB_ICONQUESTION | MB_YESNO | MB_SYSTEMMODAL
          );
          if (result == IDYES)
          {
              glfwSetWindowShouldClose(framework::CAWindow::instance().getWindow(), GL_TRUE);
          }
      }).detach();
  }

  // ✅ NEW: Function to show About dialog
  void showAboutWindow()
  {
      const wchar_t* message =
          L"Cellular Automaton Visualizer\n\n"
          L"Version 1.0\n\n"
          L"A 3D simulation and visualization tool for\n"
          L"cellular automaton patterns.\n\n"
          L"Features:\n"
          L"• Real-time 3D visualization\n"
          L"• Frame recording and replay\n"
          L"• Tomography slicing\n"
          L"• Multiple viewing modes\n\n"
          L"© 2024 - Built with OpenGL & GLFW";

      MessageBoxW(
          NULL,
          message,
          L"About Cellular Automaton",
          MB_OK | MB_ICONINFORMATION
      );
  }

} // namespace framework
