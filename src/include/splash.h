/*
 * splash.h (merged core + legacy)
 */

#ifndef SPLASH_H_
#define SPLASH_H_

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <GLFW/glfw3.h>
#include "model/simulation.h"
#include "stb_image.h"

// ✅ Window dimensions
constexpr int WINDOW_WIDTH  = 600;
constexpr int WINDOW_HEIGHT = 700; // taller to fit Replay button etc.

// ✅ Forward declarations for simulation entry points
int runSimulation(int scenario, bool paused);
int runReplay();
int runStatistics();

// ✅ Utility functions
void drawRaisedPanel(float x, float y, float w, float h);
void drawTitle(int w, int h);
void display(GLFWwindow* window);

// ✅ Callbacks
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void closeCallback(GLFWwindow* window);
void passiveMotionCallback(GLFWwindow* window, double xpos, double ypos);

#endif /* SPLASH_H_ */
