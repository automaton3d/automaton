/*
 * window.cpp
 *
 * processes IO, runs the simulation, and renders graphics for it.
 *
 * This is the top file of the program.
 */

#include <iostream>
#include <windows.h>
#include <vector>
#include "window.h"
#include <thread>
#include <chrono>
#include "GUIrenderer.h"
#include "GLutils.h"
#include "model/simulation.h"
#include "hslider.h"
#include "vslider.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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
  bool enable = true;

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

  RenderWindowGLFW::RenderWindowGLFW() :  mWindow(0)
  {
  }

  RenderWindowGLFW::~RenderWindowGLFW()
  {
  }

  /**
   * Updates the projection matrix based on radio selection
   */
  /**
   * Updates the projection matrix based on radio selection
   */
  void RenderWindowGLFW::updateProjection()
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
      mRenderer.setProjection(glm::ortho(         // ← USE setProjection()
        -orthoSize * ratio, orthoSize * ratio,
        -orthoSize, orthoSize,
        0.01f, 100.0f
      ));
    }
    else if (projection[1].isSelected())
    {
      // Perspective projection
      mRenderer.setProjection(glm::perspective(   // ← USE setProjection()
        glm::radians(45.0f),
        ratio,
        0.01f,
        100.0f
      ));
    }
  }
///////////////////

  /*
   * Processes the mouse buttons.
   */
  // Boundary constants to prevent 3D interaction conflicts
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
    void RenderWindowGLFW::buttonCallback(GLFWwindow *window, int button, int action, int mods)
    {
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
              for (Tickbox& checkbox : checkboxes)
              {
                if (checkbox.clicked(xpos, ypos, height))
           	    {
           	      checkbox.setState(!checkbox.getState());
           	      return;
           	    }
              }
              // Handle the layer list
              list->poll(xpos, ypos);
              // Handle light frame delays
              for (Tickbox &checkbox : delays)
              {
                if (xpos >= checkbox.getX() && xpos <= checkbox.getX() + 100 &&
                    ypos >= checkbox.getY() && ypos <= checkbox.getY() + 40)
                {
                  checkbox.setState(!checkbox.getState());
                  instance().onDelayToggled(&checkbox);
                  return;  // Don't activate 3D interactor
                }
              }
              // Handle view points
              bool ok = false;
              for (Radio& radio : viewpoint)
              {
                if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 &&
                    ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
                  ok = true;
              }
              if (ok)
              {
                for (Radio& radio : viewpoint)
                {
                  if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 &&
                      ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
                  {
                    radio.setSelected(true);
                    int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                    if (viewpoint[0].isSelected())
                    {
                      // Isometric view
                      instance().mCamera.setEye(glm::vec3(length, length, length));
                      instance().mCamera.setUp(glm::vec3(0, 1, 0));
                    }
                    else if (viewpoint[1].isSelected())
                    {
                      // XY view
                      instance().mCamera.setEye(glm::vec3(0,0,length));
                      instance().mCamera.setUp(glm::vec3(1,0,0));
                    }
                    else if (viewpoint[2].isSelected())
                    {
                      // YZ view
                      instance().mCamera.setEye(glm::vec3(length,0,0));
                      instance().mCamera.setUp(glm::vec3(0,1,0));
                    }
                    else if (viewpoint[3].isSelected())
                    {
                      // ZX view
                      instance().mCamera.setEye(glm::vec3(0,length,0));
                      instance().mCamera.setUp(glm::vec3(1,0,0));
                    }

                    instance().mCamera.update();
                    instance().mInteractor.setCamera(&instance().mCamera);
                  }
                  else
                  {
                    radio.setSelected(false);
                  }
                }
                return;  // Don't activate 3D interactor
              }
              // Handle projection types
              bool projOk = false;
              for (Radio& radio : projection)
              {
                if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 &&
                    ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
                  projOk = true;
              }
              if (projOk)
              {
                for (Radio& radio : projection)
                {
                  if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 &&
                      ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
                  {
                    radio.setSelected(true);
                    instance().updateProjection();
                  }
                  else
                  {
                    radio.setSelected(false);
                  }
                }
                return;  // Don't activate 3D interactor
              }
              // Handle tomography
              if (tomo->clicked(xpos, ypos, height))
              {
                // Toggle tomography mode
                tomo->flipState();
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
                  if (radio.clicked(xpos, ypos, height))
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
                ShellExecuteA(NULL, "open",
                             "https://github.com/automaton3d/automaton/blob/master/help.md",
                             NULL, NULL, SW_SHOWNORMAL);
                return;  // Don't activate 3D interactor
              }
              // ==========================================
              // STEP 3: Only activate 3D interactor if in valid zone
              // ==========================================
              if (isIn3DZone(xpos, ypos, width, height))
              {
                instance().mInteractor.setLeftClicked(true);
                instance().mInteractor.setClickPoint(xpos, ypos);
              }
              break;
            }
            case GLFW_MOUSE_BUTTON_MIDDLE:
              // Only activate if in 3D zone and no slider dragging
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                instance().mInteractor.setMiddleClicked(true);
                instance().mInteractor.setClickPoint(xpos, ypos);
              }
              break;
            case GLFW_MOUSE_BUTTON_RIGHT:
              // Only activate if in 3D zone and no slider dragging
              if (isIn3DZone(xpos, ypos, width, height) &&
                  !hslider.isDragging() && !vslider.isDragging())
              {
                instance().mInteractor.setRightClicked(true);
                instance().mInteractor.setClickPoint(xpos, ypos);
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
                instance().mInteractor.setLeftClicked(false);
              }
              break;
            }
            case GLFW_MOUSE_BUTTON_MIDDLE:
              instance().mInteractor.setMiddleClicked(false);
              break;
            case GLFW_MOUSE_BUTTON_RIGHT:
              instance().mInteractor.setRightClicked(false);
              break;
          }
          break;
        }
        default: break;
      }
    }
   void RenderWindowGLFW::errorCallback(int error, const char* description)
   {
     cerr << description << endl;
   }

   RenderWindowGLFW & RenderWindowGLFW::instance()
   {
     static RenderWindowGLFW i;
     return i;
   }

   void RenderWindowGLFW::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
   {
     float length;

     switch (action)
     {
       case GLFW_PRESS:
         switch (key)
         {
           case GLFW_KEY_ESCAPE:
             if (enable)
               glfwSetWindowShouldClose(window, GL_TRUE);
             break;

           case GLFW_KEY_LEFT_CONTROL:
           case GLFW_KEY_RIGHT_CONTROL:
             instance().mInteractor.setSpeed(5.f);
             break;

           case GLFW_KEY_LEFT_SHIFT:
           case GLFW_KEY_RIGHT_SHIFT:
             instance().mInteractor.setSpeed(0.1f);
             break;

           case GLFW_KEY_F2:
             instance().mAnimator.setAnimation(Animator::ORBIT);
             break;

           case GLFW_KEY_C:
             std::cout << "\nCamera view:"
                       << " (" << instance().mCamera.getEye().x
                       << "," << instance().mCamera.getEye().y
                       << "," << instance().mCamera.getEye().z << ") "
                       << "(" << instance().mCamera.getCenter().x
                       << "," << instance().mCamera.getCenter().y
                       << "," << instance().mCamera.getCenter().z << ") "
                       << "(" << instance().mCamera.getUp().x
                       << "," << instance().mCamera.getUp().y
                       << "," << instance().mCamera.getUp().z << ")\n\n";
             break;

           case GLFW_KEY_R:
             instance().mCamera.reset();
             instance().mInteractor.setCamera(&instance().mCamera);
             break;

           case GLFW_KEY_T:
             if (instance().mInteractor.getMotionRightClick() == TrackBallInteractor::FIRSTPERSON)
               instance().mInteractor.setMotionRightClick(TrackBallInteractor::PAN);
             else
               instance().mInteractor.setMotionRightClick(TrackBallInteractor::FIRSTPERSON);
             break;

           case GLFW_KEY_X:
             length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
             instance().mCamera.setEye(glm::vec3(length, 0, 0));
             instance().mCamera.setUp(glm::vec3(0, 1, 0));
             instance().mCamera.update();
             instance().mInteractor.setCamera(&instance().mCamera);
             break;

           case GLFW_KEY_Y:
             length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
             instance().mCamera.setEye(glm::vec3(0, length, 0));
             instance().mCamera.setUp(glm::vec3(1, 0, 0));
             instance().mCamera.update();
             instance().mInteractor.setCamera(&instance().mCamera);
             break;

           case GLFW_KEY_Z:
             length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
             instance().mCamera.setEye(glm::vec3(0, 0, length));
             instance().mCamera.setUp(glm::vec3(1, 0, 0));
             instance().mCamera.update();
             instance().mInteractor.setCamera(&instance().mCamera);
             break;

           case GLFW_KEY_O:
             length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
             instance().mCamera.setEye(glm::vec3(length, length, length));
             instance().mCamera.setUp(glm::vec3(0, 1, 0));
             instance().mCamera.update();
             instance().mInteractor.setCamera(&instance().mCamera);
             break;

           case GLFW_KEY_M:
             sound(false);
             break;

           case GLFW_KEY_P:
             if (enable)
               pause = !pause;
             break;

           case GLFW_KEY_H:
             if (!pause)
               enable = !enable;
             break;

           case GLFW_KEY_F1:
             showHelp = !showHelp;
             break;

           default:
             break;
         }
         break;

       case GLFW_RELEASE:
         switch (key)
         {
           case GLFW_KEY_LEFT_CONTROL:
           case GLFW_KEY_RIGHT_CONTROL:
           case GLFW_KEY_LEFT_SHIFT:
           case GLFW_KEY_RIGHT_SHIFT:
             instance().mInteractor.setSpeed(1.f);
             break;

           default:
             break;
         }
         break;

       default:
         break;
     }
   }


   /*
      * Processes mouse movement - IMPROVED VERSION
      */
     void RenderWindowGLFW::moveCallback(GLFWwindow* window, double xpos, double ypos)
     {
       int width, height;
       glfwGetWindowSize(window, &width, &height);

       // Always update sliders if they're being dragged
       hslider.onMouseDrag((int)xpos, (int)ypos, height);
       vslider.onMouseDrag((int)xpos, (int)ypos, height);

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
         instance().mInteractor.setClickPoint(xpos, ypos);
       }
     }

     /*
       * Processes scroll wheel - IMPROVED VERSION
       */
      void RenderWindowGLFW::scrollCallback(GLFWwindow *window, double xpos, double ypos)
      {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Only allow scroll zoom if in 3D zone and no sliders being dragged
        if (!hslider.isDragging() && !vslider.isDragging() &&
            isIn3DZone(mouseX, mouseY, width, height))
        {
          instance().mInteractor.setScrollDirection(xpos + ypos > 0 ? true : false);
        }
      }
  void RenderWindowGLFW::sizeCallback(GLFWwindow *window, int width, int height)
  {
    instance().mRenderer.resize(width, height);
    instance().mInteractor.setScreenSize(width, height);
    instance().mAnimator.setScreenSize(width, height);
  }

  /*
   * Runs the simulation.
   * (independent of the GUI thread)
   */
  DWORD WINAPI RenderWindowGLFW::SimulateThread(LPVOID lpParam)
  {
    puts("Simulation thread launched...");
    static_cast<RenderWindowGLFW*>(lpParam)->isThreadReady = true;
    Sleep(100);
    // Prepare the mirror grid before starting
    automaton::swap_lattices();
    while (true)
    {
      if(!pause)
      {
        // Run one step of the simulation
        automaton::simulation();
        // Transfer data to the GUI
        automaton::updateBuffer();
        framework::timer++;
      }
      else
      {
        Sleep(80);
      }
    }
    return 0;
  }

  /*
   * Runs the graphics.
   * This is the GUI loop, but the independent simulation loop thread is launched here.
   */
  int RenderWindowGLFW::run()
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
    glfwSetErrorCallback(& RenderWindowGLFW::errorCallback);
    mWindow = glfwCreateWindow(width, height, "Cellular automaton", NULL, NULL);
    if (!mWindow)
    {
      glfwTerminate();
      perror("GUI window failed.");
      return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(mWindow, & RenderWindowGLFW::moveCallback);
    glfwSetKeyCallback(mWindow, & RenderWindowGLFW::keyCallback);
    glfwSetMouseButtonCallback(mWindow, & RenderWindowGLFW::buttonCallback);
    glfwSetScrollCallback(mWindow, & RenderWindowGLFW::scrollCallback);
    glfwSetWindowSizeCallback(mWindow, &RenderWindowGLFW::sizeCallback);
    mInteractor.setCamera(& mCamera);
    mRenderer.setCamera(& mCamera);
    mAnimator.setInteractor(& mInteractor);
    mRenderer.init();
    sizeCallback(mWindow, width, height); // Set initial size.
    // Isometric view
    int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
    instance().mCamera.setEye(glm::vec3(length, length, length));
    instance().mCamera.setUp(glm::vec3(0, 1, 0));
    instance().mCamera.update();
    instance().mInteractor.setCamera(& instance().mCamera);
    // Launch the simulation thread
    DWORD dwThreadId;
    HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, this, 0, &dwThreadId);
    CloseHandle(hSimulateThread);
    // Enter the visualization loop
    while (!glfwWindowShouldClose(mWindow))
    {
      if (enable)
      {
        mAnimator.animate();
        mInteractor.update();
        mRenderer.render();
        glfwSwapBuffers(mWindow);
      }
      else
      {
        // Sleep for 16ms (~60 FPS) to reduce resource usage
        this_thread::sleep_for(std::chrono::milliseconds(16));
      }
      glfwPollEvents();
    }
    puts("GUI thread ended.");
    glfwDestroyWindow(mWindow);
    glfwTerminate();
    return EXIT_SUCCESS;
  }

  void RenderWindowGLFW::onDelayToggled(Tickbox* toggled)
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
 *      (Called by the Splash)      *
 *                                  *
 ************************************/

#ifdef GRAPH // Create the graphical version

  /**
   * Starts the 'Simulation' option of splash.cpp.
   */
  int runSimulation(unsigned L, unsigned W)
  {
    Beep(1000, 80);
    glfwInit();
    // Initialize
    for (int step = 0; step < 8; step++)
    {
      automaton::initSimulation(step);
    }
    char arg0[] = "test";
    char *argv[] = { arg0, NULL };
    int argc = 1;
    glutInit(&argc, argv);
    bool status = framework::RenderWindowGLFW::instance().run();
    glfwTerminate();
    return status;
  }


#else  // Create the text only version

  int main(int argc, char *argv[])
  {
    printf("SIDE=%d, PERIOD=%d,\n", SIDE, automaton::RMAX);
    fflush(stdout);
    automaton::initSimulation();
    while (true)
    {
      automaton::displayLattice();
      automaton::simulation(); Sleep(100);
    }
  }
#endif
