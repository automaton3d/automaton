/*
 * splash.h
 */

#ifndef SPLASH_H_
#define SPLASH_H_

#include <GL/freeglut.h>
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include "model/simulation.h"
#include "stb_image.h"

// ✅ Window dimensions
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 650;

// ✅ Button structure for clickable UI elements
struct XButton
{
  float x, y, w, h;
  const char* label;
};

// ✅ Forward declarations for simulation entry points
int runSimulation(int scenario);
int runStatistics();

#endif /* SPLASH_H_ */
