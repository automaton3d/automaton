/*
 * globals.h
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Forward declarations
class RectRenderer;
class TextRenderer;

#include "radio.h"

// --- Timer ---
extern unsigned long long timer;

// --- Simulation State ---
extern int scenario;
extern std::vector<unsigned int> voxels;

// === Shared OpenGL Resources ===
extern GLuint colorProgram2D;
extern GLuint textureProgram2D;
extern GLint colorMvpLoc2D;
extern GLint colorColorLoc2D;
extern GLint textureMvpLoc;
extern GLint textureSamplerLoc;

// === Viewport and Matrices ===
extern GLint gViewport[4];
extern glm::mat4 modelview;
extern glm::mat4 projection;

// === Text Rendering ===
extern TextRenderer* textRenderer;
extern GLuint textProgram;

// === UI State Flags ===
extern bool active;
extern bool pause;
extern bool enable;

// === Global UI Elements ===
extern std::vector<Radio> viewpoint;
extern std::vector<Radio> projectionDirs;
extern std::vector<Radio> tomoDirs;
extern RectRenderer* rect;

// Utilidade: calcula (gViewport[3] - n)
inline GLint bottomOffset(GLint n)
{
    return gViewport[3] - n;
}

#endif // GLOBALS_H