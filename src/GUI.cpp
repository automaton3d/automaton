/*
 * GUI.cpp
 *
 * Implements the GUIrenderer orchestration logic.
 */

#include "GUI.h"
#include <GL/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include "hslider.h"
#include "replay_progress.h"
#include "logo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "GUI.h"

namespace framework
{
    bool MULTICUBE_MODE = false;
    int GUImode = SIMULATION;   // or = 0 if you prefer

    unsigned tomo_x = 0;
    unsigned tomo_y = 0;
    unsigned tomo_z = 0;
    Dropdown* fileMenu = nullptr;
    Dropdown* helpMenu = nullptr;

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
       fileMenu = new Dropdown(10, 1, 120, 30, fileOptions);
       helpMenu = new Dropdown(140, 1, 150, 30, helpOptions);
   }

   void GUIrenderer::handleMenuSelection()
   {
       if (!fileMenu || !helpMenu) return;

       // Check File menu
       if (fileMenu->getSelectedIndex() >= 0)
       {
           std::string selected = fileMenu->getSelectedItem();

           if (selected == "Save replay")
           {
               // TODO: Implement save replay functionality
               std::cout << "Save replay selected" << std::endl;
           }
           else if (selected == "Load replay")
           {
               // TODO: Implement load replay functionality
               std::cout << "Load replay selected" << std::endl;
           }
           else if (selected == "Exit")
           {
               exit(0);
           }
       }

       // Check Help menu
       if (helpMenu->getSelectedIndex() >= 0)
       {
           std::string selected = helpMenu->getSelectedItem();

           if (selected == "GUI help")
           {
               extern bool showHelp;
               showHelp = !showHelp;
           }
           else if (selected == "Documentation")
           {
               // TODO: Open documentation
               std::cout << "Documentation selected" << std::endl;
           }
           else if (selected == "About")
           {
               // TODO: Show about dialog
               std::cout << "About selected" << std::endl;
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

} // namespace framework
