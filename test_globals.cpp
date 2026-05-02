// test_globals.cpp
#include "test_globals.h"

// ============================================================================
// Definições Únicas das Variáveis Globais de Teste
// ============================================================================

GLFWwindow* window = nullptr;

GLuint textProgram = 0;
GLuint colorProgram2D = 0;

GLint colorMvpLoc2D = 0;
GLint colorColorLoc2D = 0;

TextRenderer* textRenderer = nullptr;

int gViewport[4] = {0, 0, 800, 600};

bool gAppShouldExit = false;

// NOTE:
// Você deve garantir que o arquivo 'dropdown.cpp' e 'text_renderer.cpp' 
// NÃO INCLUAM 'globals.h', mas sim 'test_globals.h' para este teste. 
// No entanto, isso é difícil, então faremos uma correção no main.cpp (veja o próximo passo).