/*
 * window.h
 */

#ifndef RENDER_WIN
#define RENDER_WIN

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <GUI.h>
#include <iostream>
#include "model/simulation.h"
#include "animator.h"

namespace automaton
{
  extern bool convol_delay;
  extern bool diffuse_delay;
  extern bool reloc_delay;
}

namespace framework
{
  /**
   * Only one window instance is supported, but this could be extended by
   * using a registry class to scale the static callback handlers.
   */
  class CAWindow
  {
  public:
      CAWindow();
      ~CAWindow();

    static CAWindow & instance();
    int run();

    static void buttonCallback(GLFWwindow *window, int button, int action,
                                 int mods);
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow *window, int key, int scancode,
                              int action, int mods);
    static void moveCallback(GLFWwindow *window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow *window, double xpos, double ypos);
    static void sizeCallback(GLFWwindow *window, int width, int height);

    void onDelayToggled(Tickbox* toggled);
    bool pendingExit = false;
    GLFWwindow* getWindow() const { return mWindow; }

  private:
    Animator mAnimator;
    Camera mCamera;
    TrackBallInteractor mInteractor;
    GUIrenderer mRenderer;
    GLFWwindow *mWindow;
    static DWORD WINAPI SimulateThread(LPVOID lpParam);
    volatile bool isThreadReady = false;
    void updateProjection();

  };

  void sound();
  void renderCenterBox(const char* text);

} // end namespace framework

#endif // RENDER_WIN
