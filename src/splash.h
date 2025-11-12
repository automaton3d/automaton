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
// Increased height to accommodate the new "Replay" button and panel layout
constexpr int WINDOW_HEIGHT = 700;

void drawRaisedPanel(float x, float y, float w, float h);

// ✅ Forward declarations for simulation entry points
int runSimulation(int scenario, bool paused);
int runReplay();
int runStatistics();

#endif /* SPLASH_H_ */
