/*
 * splash.h
 *
 *  Created on: 16 de out. de 2025
 *      Author: Alexandre
 */

#ifndef SPLASH_H_
#define SPLASH_H_

#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <vector>
#include "model/simulation.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Button structure
struct Button
{
  float x, y, w, h;
  const char* label;
};

// Forward declarations
int runSimulation(unsigned EL, unsigned W);
int runStatistics(unsigned EL, unsigned W);

#endif /* SPLASH_H_ */
