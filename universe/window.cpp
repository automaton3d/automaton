/*
 * window.cpp
 *
 * processes IO, runs the simulation, and renders graphics for it.
 *
 * This is the top file of the program.
 */

#include <iostream>
#include <vector>
#include "window.h"
#include <thread>
#include <chrono>
#include "GUIrenderer.h"
#include "model/simulation.h"
#include "slider.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


namespace framework
{
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
  extern VerticalSlider slider;

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

  /*
   * Processes the mouse buttons.
   */
  void RenderWindowGLFW::buttonCallback(GLFWwindow *window, int button, int action, int mods)
  {
	int width, height;
    glfwGetWindowSize(window, &width, &height);
    switch(action)
    {
      case GLFW_PRESS:
      {
       double xpos, ypos;
       glfwGetCursorPos(window, & xpos, & ypos);
       ypos += 20;
       // Check if the mouse click is within the slider's thumb area
       switch(button)
       {
         case GLFW_MOUSE_BUTTON_LEFT:
         {
           // Handle the vertical slider
           slider.onMouseClick(button, action, xpos, ypos, height);
           // Handle the lattice features
           for (Tickbox& checkbox : checkboxes)
           {
             if (xpos >= checkbox.getX() && xpos <= checkbox.getX() + 100 && ypos >= checkbox.getY() && ypos <= checkbox.getY() + 40)
             {
               checkbox.setState(!checkbox.getState());
               break;
             }
           }
           // Handle the layer list
           list.poll(xpos, ypos);
           // Handle light frame delays
           for (Tickbox &checkbox : delays)
           {
             if (xpos >= checkbox.getX() && xpos <= checkbox.getX() + 100 && ypos >= checkbox.getY() && ypos <= checkbox.getY() + 40)
             {
               // Toggle the clicked checkbox
               checkbox.setState(!checkbox.getState());
               // Optional: notify that this specific checkbox changed
               instance().onDelayToggled(&checkbox);
             }
           }
           // Handle view points
           bool ok = false;
           for (Radio& radio : viewpoint)
           {
             if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 && ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
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
                 if (viewpoint[0].isSelected())
                 {
                   // Isometric view
                   int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                   instance().mCamera.setEye(length, length, length);
                   instance().mCamera.setUp(0, 1, 0);
                   instance().mCamera.update();
                   instance().mInteractor.setCamera(& instance().mCamera);
                 }
                 else if (viewpoint[1].isSelected())
                 {
                   // XY view
                   int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                   instance().mCamera.setEye(0,0,length);
                   instance().mCamera.setUp(1,0,0);
                   instance().mCamera.update();
                   instance().mInteractor.setCamera(& instance().mCamera);
                 }
                 else if (viewpoint[2].isSelected())
                 {
                   // YZ view
                   int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                   instance().mCamera.setEye(length,0,0);
                   instance().mCamera.setUp(0,1,0);
                   instance().mCamera.update();
                   instance().mInteractor.setCamera(& instance().mCamera);
                 }
                 else if (viewpoint[3].isSelected())
                 {
                   // ZX view
                   int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                   instance().mCamera.setEye(0,length,0);
                   instance().mCamera.setUp(1,0,0);
                   instance().mCamera.update();
                   instance().mInteractor.setCamera(& instance().mCamera);
                 }
               }
               else
               {
                 radio.setSelected(false);
               }
             }
           }
           instance().mInteractor.setLeftClicked(true);
           instance().mInteractor.setClickPoint(xpos, ypos);
           break;
         }
       case GLFW_MOUSE_BUTTON_MIDDLE:
         instance().mInteractor.setMiddleClicked(true);
         break;
       case GLFW_MOUSE_BUTTON_RIGHT:
         instance().mInteractor.setRightClicked(true);
         break;
     }
     instance().mInteractor.setClickPoint(xpos, ypos);
     break;
   }
   case GLFW_RELEASE:
   {
    switch(button)
    {
     case GLFW_MOUSE_BUTTON_LEFT:
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      ypos += 20;
      instance().mInteractor.setLeftClicked(false);
      slider.onMouseClick(button, action, xpos, ypos, height);
      break;
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

 /*
  * Processes the keyboard.
  */
 void RenderWindowGLFW::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
 {
   float length;
   switch(action)
   {
     case GLFW_PRESS:
       switch(key)
       {
         case GLFW_KEY_ESCAPE:
           // Exit app on ESC key.
           if (enable)
             glfwSetWindowShouldClose(window, GL_TRUE);
           break;
         case GLFW_KEY_LEFT_CONTROL:
         case GLFW_KEY_RIGHT_CONTROL:
             instance().mInteractor.setSpeed(5.f);
             break;
         case GLFW_KEY_LEFT_SHIFT:
         case GLFW_KEY_RIGHT_SHIFT:
             instance().mInteractor.setSpeed(.1f);
             break;
         case GLFW_KEY_F1:
             instance().mAnimator.setAnimation(Animator::ORBIT);
             break;
         case GLFW_KEY_C:
             cout
			   << "\nCamera view:"
                 << "(" << instance().mCamera.getEye().x
                 << "," << instance().mCamera.getEye().y
                 << "," << instance().mCamera.getEye().z << ") "
                 << "(" << instance().mCamera.getCenter().x
                 << "," << instance().mCamera.getCenter().y
                 << "," << instance().mCamera.getCenter().z << ") "
                 << "(" << instance().mCamera.getUp().x
                 << "," << instance().mCamera.getUp().y
                 << "," << instance().mCamera.getUp().z  << ")\n\n";
             break;
         case GLFW_KEY_R:
             // Reset the view.
             instance().mCamera.reset();
             instance().mInteractor.setCamera(& instance().mCamera);
             break;
         case GLFW_KEY_T:
             // Toogle motion type.
             if (instance().mInteractor.getMotionRightClick() ==
                     TrackBallInteractor::FIRSTPERSON) {
                 instance().mInteractor.setMotionRightClick(
                         TrackBallInteractor::PAN);
             }
             else
             {
               instance().mInteractor.setMotionRightClick(
                         TrackBallInteractor::FIRSTPERSON);
             }
             break;
         case GLFW_KEY_X:
             // Snap view to axis.
             length = glm::length(instance().mCamera.getEye() -
                                  instance().mCamera.getCenter());
             instance().mCamera.setEye(length,0,0);
             instance().mCamera.setUp(0,1,0);
             instance().mCamera.update();
             instance().mInteractor.setCamera(& instance().mCamera);
             break;
         case GLFW_KEY_Y:
             length = glm::length(instance().mCamera.getEye() -
                                  instance().mCamera.getCenter());
             instance().mCamera.setEye(0,length,0);
             instance().mCamera.setUp(1,0,0);
             instance().mCamera.update();
             instance().mInteractor.setCamera(& instance().mCamera);
             break;
         case GLFW_KEY_Z:
           length = glm::length(instance().mCamera.getEye() -
                                  instance().mCamera.getCenter());
           instance().mCamera.setEye(0,0,length);
           instance().mCamera.setUp(1,0,0);
           instance().mCamera.update();
           instance().mInteractor.setCamera(& instance().mCamera);
           break;
         case GLFW_KEY_O:
           // Isometric view
           length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
           instance().mCamera.setEye(length, length, length);
           instance().mCamera.setUp(0, 1, 0);
           instance().mCamera.update();
           instance().mInteractor.setCamera(& instance().mCamera);
           break;
         case GLFW_KEY_M:
           // Test jingle
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
         default: break;
             }
             break;
         case GLFW_RELEASE:
           switch(key)
           {
             case GLFW_KEY_LEFT_CONTROL:
             case GLFW_KEY_RIGHT_CONTROL:
             case GLFW_KEY_LEFT_SHIFT:
             case GLFW_KEY_RIGHT_SHIFT:
               instance().mInteractor.setSpeed(1.f);
               break;
           }
           break;
         default: break;
     }
 }

  void RenderWindowGLFW::moveCallback(GLFWwindow* window, double xpos, double ypos)
  {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Convert ypos to OpenGL coordinates
    ypos = height - ypos;

    // Update the slider's thumb position if it's being dragged
    slider.onMouseDrag(xpos, ypos, height);

    // Update the trackball
    instance().mInteractor.setClickPoint(xpos, ypos);
  }

  void RenderWindowGLFW::scrollCallback(GLFWwindow *window, double xpos, double ypos)
  {
    instance().mInteractor.setScrollDirection(xpos + ypos > 0 ? true : false);
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
        timer++;
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
    instance().mCamera.setEye(length, length, length);
    instance().mCamera.setUp(0, 1, 0);
    instance().mCamera.update();
    instance().mInteractor.setCamera(& instance().mCamera);
    // Launch the simulation thread
    DWORD dwThreadId;
    HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, this, 0, &dwThreadId);
    CloseHandle(hSimulateThread);
    // Hide the cursor
//    glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
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

} // end namespace framework

/************************************
 *                                  *
 *          Entry point.            *
 *      (Called by the OS)          *
 *                                  *
 ************************************/

#ifdef GRAPH // Create the graphical version

  int main(int argc, char *argv[])
  {
    glutInit(&argc, argv);
    if (!glfwInit())
      return EXIT_FAILURE;
    Beep(1000, 80);
    return framework::RenderWindowGLFW::instance().run();
 }

#else  // Create the text only version

  int main(int argc, char *argv[])
  {
    printf("SIDE=%d, PERIOD=%d,\n", SIDE, automaton::RMAX);
    fflush(stdout);
 //   automaton::initSimulation();
    while (true)
    {
      automaton::displayLattice();
      automaton::simulation(); Sleep(100);
    }
  }
#endif
