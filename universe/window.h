/*
 * window.h
 */

#ifndef RENDER_WIN
#define RENDER_WIN

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <iostream>
#include "model/simulation.h"
#include "animator.h"
#include "GUIrenderer.h"

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
	class RenderWindowGLFW
	{
	public:
	    RenderWindowGLFW();
	    ~RenderWindowGLFW();

	  static RenderWindowGLFW & instance();
	  int run();

	  static void buttonCallback(GLFWwindow *window, int button, int action,
	                               int mods);
	  static void errorCallback(int error, const char* description);
	  static void keyCallback(GLFWwindow *window, int key, int scancode,
	                            int action, int mods);
	  static void moveCallback(GLFWwindow *window, double xpos, double ypos);
	  static void scrollCallback(GLFWwindow *window, double xpos, double ypos);
	  static void sizeCallback(GLFWwindow *window, int width, int height);

	  void onDelayToggled(Tickbox* toggled)
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

	private:
	  Animator mAnimator;
	  Camera mCamera;
	  TrackBallInteractor mInteractor;
	  GUIrenderer mRenderer;
	  GLFWwindow *mWindow;
	  static DWORD WINAPI SimulateThread(LPVOID lpParam);
	  volatile bool isThreadReady = false;
	};

	void sound();
    void renderCenterBox(const char* text);
    int runSimulation();

} // end namespace framework

#endif // RENDER_WIN
