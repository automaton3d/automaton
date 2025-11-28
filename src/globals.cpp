/*
 * globals.cpp
 */

#include "globals.h"
#include "rect_renderer.h"
#include "text_renderer.h"
#include <iostream>

unsigned long long timer = 0;

// Simulation state
int scenario = -1;
std::vector<unsigned int> voxels;

// Shaders and uniforms
GLuint colorProgram2D = 0;
GLuint textureProgram2D = 0;
GLint colorMvpLoc2D = -1;
GLint colorColorLoc2D = -1;
GLint textureMvpLoc = -1;
GLint textureSamplerLoc = -1;
GLuint textProgram = 0;
TextRenderer* textRenderer = nullptr;

// Viewport and matrices
GLint gViewport[4] = {0, 0, 0, 0};
glm::mat4 modelview = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

// Flags
bool active = false;
bool pause = false;
bool enable = true;

// UI elements
std::vector<Radio> viewpoint;
std::vector<Radio> projectionDirs;
std::vector<Radio> tomoDirs;
RectRenderer* rect = nullptr;
