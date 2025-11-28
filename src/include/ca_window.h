/*
 * ca_window.h
 */

#ifndef CA_WINDOW_H
#define CA_WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <GUI.h>
#include "model/simulation.h"
#include "animator.h"
#include "camera.h"
#include "trackball.h"
#include "vslider.h"
#include "hslider.h"
#include "tickbox.h"
#include "radio.h"
#include "replay_progress.h"
#include "recorder.h"
#include "text_renderer.h"
#include "globals.h"

namespace automaton
{
  extern bool convol_delay;
  extern bool diffuse_delay;
  extern bool reloc_delay;
}

namespace framework
{
  extern VSlider vslider;
  extern HSlider hslider;
  extern Tickbox *tomo;
  extern std::vector<Radio> tomoDirs;
  extern bool showHelp;
  extern Tickbox* scenarioHelpToggle;
  extern ReplayProgressBar *replayProgress;
  extern int vis_dx;
  extern int vis_dy;
  extern int vis_dz;
  extern FrameRecorder recorder;
  extern bool recordFrames;
  extern size_t replayIndex;
  extern bool showExitDialog;
  extern const float axisLength;
  extern bool showAboutDialog;
  
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

      // GLFW callbacks
      static void buttonCallback(GLFWwindow *window, int button, int action, int mods);
      static void errorCallback(int error, const char* description);
      static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
      static void moveCallback(GLFWwindow *window, double xpos, double ypos);
      static void passiveMotionCallback(int x, int y);
      static void scrollCallback(GLFWwindow *window, double xpos, double ypos);
      static void sizeCallback(GLFWwindow *window, int width, int height);

      void onDelayToggled(Tickbox* toggled);

      bool pendingExit = false;
      bool stopSimThread = false;
      GLFWwindow* getWindow() const { return mWindow_; }

  private:
      Animator mAnimator_;
      Camera mCamera_;
      TrackBallInteractor mInteractor_;
      GUIrenderer mRenderer_;
      GLFWwindow *mWindow_;
      static void SimulateThread();
      volatile bool isThreadReady_ = false;

      void updateProjection();
  };

  void sound();
  void renderCenterBox(const char* text);
  void requestExit();
  void saveReplay();
  void loadReplay();
  bool isIn3DZone(double xpos, double ypos, int windowWidth, int windowHeight);

} // end namespace framework

int runSimulation(int scenario, bool paused);
int runReplay();
int runStatistics();

#endif // CA_WINDOW_H