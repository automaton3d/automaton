// test_globals.h
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "text_renderer.h"

// Variáveis Globais MINIMALMENTE necessárias para o teste do Dropdown/TextRenderer
// (Estas substituem as externs de globals.h para este teste específico)

extern GLFWwindow* window;

extern GLuint textProgram;
extern GLuint colorProgram2D;

extern GLint colorMvpLoc2D;
extern GLint colorColorLoc2D;

extern TextRenderer* textRenderer;

extern int gViewport[4];

// Variável de controle (opcional, mas bom ter)
extern bool gAppShouldExit;