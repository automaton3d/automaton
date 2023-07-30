//#include "simulation.h"
#include "window.h"
#include "mygl.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

namespace framework
{

// UID

HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk, axes_chk;
HWND single_rad, partial_rad, full_rad, rand_rad;
HWND xy_rad, yz_rad, zx_rad, iso_rad;
HPEN xPen, yPen, zPen, boxPen;
HWND stopButton, suspendButton, centerButton;
bool momentum, wavefront, single, partial, full, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;

// Trackball

bool stop;
bool active = true;
unsigned long timer = 0;

RenderWindowGLFW::RenderWindowGLFW() :  mWindow(0)
{
}

RenderWindowGLFW::~RenderWindowGLFW()
{
}

void RenderWindowGLFW::buttonCallback(GLFWwindow *window, int button, int action, int mods)
{
  switch(action)
  {
    case GLFW_PRESS:
    {
      double xpos, ypos;
      glfwGetCursorPos(window, & xpos, & ypos);
      ypos += 20;
      switch(button)
      {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
          for (Tickbox& checkbox : checkboxes)
          {
            if (xpos >= checkbox.getX() && xpos <= checkbox.getX() + 100 &&
                ypos >= checkbox.getY() && ypos <= checkbox.getY() + 20)
            {
              checkbox.setState(!checkbox.getState());
              break;
            }
          }
          bool ok = false;
          for (Radio& radio : dataset)
          {
            if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 && ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
              ok = true;
          }
          if (ok)
          {
            for (Radio& radio : dataset)
            {
              if (xpos >= radio.getX()-2 && xpos <= radio.getX() + 100 &&
                  ypos >= radio.getY()-5 && ypos <= radio.getY() + 25)
              {
                radio.setSelected(true);
              }
              else
              {
                radio.setSelected(false);
              }
            }
          }
          ok = false;
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
                  int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                  instance().mCamera.setEye(0,0,length);
                  instance().mCamera.setUp(1,0,0);
                  instance().mCamera.update();
                  instance().mInteractor.setCamera(& instance().mCamera);
                }
                else if (viewpoint[2].isSelected())
                {
                  int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                  instance().mCamera.setEye(length,0,0);
                  instance().mCamera.setUp(0,1,0);
                  instance().mCamera.update();
                  instance().mInteractor.setCamera(& instance().mCamera);
                }
                else if (viewpoint[3].isSelected())
                {
                    int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                    instance().mCamera.setEye(0,length,0);
                    instance().mCamera.setUp(1,0,0);
                    instance().mCamera.update();
                    instance().mInteractor.setCamera(& instance().mCamera);
                }
                else if (viewpoint[4].isSelected())
                {
                  instance().mCamera.reset();
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
          instance().mInteractor.setLeftClicked(false);
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
    std::cerr << description << std::endl;
}

RenderWindowGLFW & RenderWindowGLFW::instance()
{
  static RenderWindowGLFW i;
  return i;
}

void RenderWindowGLFW::keyCallback(GLFWwindow *window, int key, int scancode,
                                   int action, int mods)
{
    float length;
    switch(action)
    {
        case GLFW_PRESS:
            switch(key)
            {
                case GLFW_KEY_ESCAPE:
                    // Exit app on ESC key.
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
                    std::cout
                        << "(" << instance().mCamera.getEye().x
                        << "," << instance().mCamera.getEye().y
                        << "," << instance().mCamera.getEye().z << ") "
                        << "(" << instance().mCamera.getCenter().x
                        << "," << instance().mCamera.getCenter().y
                        << "," << instance().mCamera.getCenter().z << ") "
                        << "(" << instance().mCamera.getUp().x
                        << "," << instance().mCamera.getUp().y
                        << "," << instance().mCamera.getUp().z  << ")\n";
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
                    } else {
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
                  sound();
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

void RenderWindowGLFW::moveCallback(GLFWwindow *window, double xpos,
                                    double ypos)
{
    instance().mInteractor.setClickPoint(xpos, ypos);
}

void RenderWindowGLFW::scrollCallback(GLFWwindow *window, double xpos,
                                      double ypos)
{
    instance().mInteractor.setScrollDirection(xpos + ypos > 0 ? true : false);
}

void RenderWindowGLFW::sizeCallback(GLFWwindow *window, int width, int height)
{
    instance().mRenderer.resize(width, height);
    instance().mInteractor.setScreenSize(width, height);
    instance().mAnimator.setScreenSize(width, height);
}

DWORD WINAPI RenderWindowGLFW::SimulateThread(LPVOID lpParam)
{
  automaton::initSimulation();
  static_cast<RenderWindowGLFW*>(lpParam)->isThreadReady = true;
  Sleep(100);
  while (true)
  {
  if(!stop)
  {
    automaton::simulation();
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

int RenderWindowGLFW::run(int width, int height)
{
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

  // Calculate the position to center the window
  int xPos = (mode->width - 300) / 2;
  int yPos = (mode->height - 50) / 2;

  // Create a borderless GLFW window and center it on the screen
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  GLFWwindow* iconWindow = glfwCreateWindow(300, 50, "Loading...", NULL, NULL);
  glfwSetWindowPos(iconWindow, xPos, yPos); // Set window position

  // Make the window's context current
  glfwMakeContextCurrent(iconWindow);

  DWORD dwThreadId;
  HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, this, 0, &dwThreadId);
  CloseHandle(hSimulateThread);

  // Set up variables for the ellipsis
  int ellipsisCount = 2;
  int maxEllipsisCount = 12;
  double lastEllipsisChangeTime = glfwGetTime();
  double ellipsisChangeInterval = 0.5; // Change the ellipsis every 0.5 seconds

  // Set up OpenGL for basic text rendering
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width, 0, height, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glColor3f(1.0f, 1.0f, 1.0f);
  while (!isThreadReady)
  {
    // Poll events
    glfwPollEvents();

    // Clear the buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the loading message and ellipsis
    std::string loadingMessage = "Loading";
    for (int i = 0; i < ellipsisCount; ++i)
    {
      loadingMessage += ".";
    }

    // Draw the loading message at the center of the screen
    framework::drawString(loadingMessage, 500, height / 2);

    // Change the ellipsis count if enough time has passed
    if (glfwGetTime() - lastEllipsisChangeTime >= ellipsisChangeInterval)
    {
      ellipsisCount = (ellipsisCount % maxEllipsisCount) + 1;
      lastEllipsisChangeTime = glfwGetTime();
    }

    // Swap buffers
    glfwSwapBuffers(iconWindow);
  }
  glfwSetErrorCallback(& RenderWindowGLFW::errorCallback);
  mWindow = glfwCreateWindow(width, height, "Cellular automaton", NULL, NULL);
  if (!mWindow)
  {
    glfwTerminate();
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

  while (!glfwWindowShouldClose(mWindow))
  {
    mAnimator.animate();
    mInteractor.update();
    mRenderer.render();
    glfwSwapBuffers(mWindow);
    glfwPollEvents();
  }
  glfwDestroyWindow(mWindow);
  glfwTerminate();
  return EXIT_SUCCESS;
}

} // end namespace framework

/**
 * Entry point.
 */
int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    if (!videoMode)
    {
      glfwTerminate();
      return -1;
    }
  return framework::RenderWindowGLFW::instance().run(videoMode->width, videoMode->height);
}
