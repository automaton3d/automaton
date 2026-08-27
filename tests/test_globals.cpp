// test_globals.cpp
#include "test_globals.h"

// ============================================================================
// Definições Únicas das Variáveis Globais de Teste
// ============================================================================

GLFWwindow* window = nullptr;

GLuint textProgram = 0;

TextRenderer* textRenderer = nullptr;

int gViewport[4] = {0, 0, 800, 600};

bool gAppShouldExit = false;
