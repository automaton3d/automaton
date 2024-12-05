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
#include "mygl.h"

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
	  int run(int width, int height);

	  static void buttonCallback(GLFWwindow *window, int button, int action,
	                               int mods);
	  static void errorCallback(int error, const char* description);
	  static void keyCallback(GLFWwindow *window, int key, int scancode,
	                            int action, int mods);
	  static void moveCallback(GLFWwindow *window, double xpos, double ypos);
	  static void scrollCallback(GLFWwindow *window, double xpos, double ypos);
	  static void sizeCallback(GLFWwindow *window, int width, int height);

	private:
	    Animator mAnimator;
	  Camera mCamera;
	  TrackBallInteractor mInteractor;
	  RendererOpenGL1 mRenderer;
	    GLFWwindow *mWindow;
	    static DWORD WINAPI SimulateThread(LPVOID lpParam);
	    volatile bool isThreadReady = false;
	};

	void sound();

} // end namespace framework

#endif // RENDER_WIN
