/*
 * stats.cpp (adaptado para GLAD/GLFW, multiplataforma)
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include "model/simulation.h"
#include "stats.h"
#include "text_renderer.h"
#include "globals.h"
#include "shader_utils.h"

extern unsigned long long timer;

namespace automaton {
  extern unsigned EL;
  bool initSimulation(int step);
  extern std::vector<Cell> lattice_curr;
}

namespace stats {

#define MAXTICKS 10000

int windowWidth = 600;
int windowHeight = 480;

// Controle de thread
volatile bool simulationRunning = false;
volatile bool pauseSimulation = false;
volatile bool stopSimulation = false;
std::thread simulationThread;
std::chrono::steady_clock::time_point tbegin;

// Buffer de console
std::vector<std::string> consoleLines;
std::mutex consoleMutex;
const int MAX_CONSOLE_LINES = 50;

// Funcao de log
void consolePrintf(const char* format, ...) {
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  {
    std::lock_guard<std::mutex> lock(consoleMutex);
    consoleLines.push_back(buffer);
    if (consoleLines.size() > MAX_CONSOLE_LINES)
      consoleLines.erase(consoleLines.begin());
  }

  std::cout << buffer;
}

// Loop da simulacao em thread separada
void SimulationLoop() {
  consolePrintf("Launching statistics simulation thread...\n");
  for (int step = 0; automaton::initSimulation(step); step++);
  automaton::swap_lattices();
  tbegin = std::chrono::steady_clock::now();
  simulationRunning = true;

  while (!stopSimulation) {
    if (!pauseSimulation) {
      automaton::simulation();
      collectData();
      timer++;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
  }
  simulationRunning = false;
  consolePrintf("Simulation thread ended.\n");
}

// Renderizacao
void display(GLFWwindow* window, TextRenderer& renderer) {
  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Tempo decorrido
  if (simulationRunning) {
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - tbegin).count();
    std::stringstream ss;
    ss << "Elapsed " << (millis / 1000.0) << "s";
    renderer.RenderText(ss.str(), 20, 30, 0.8f,
                        glm::vec3(1.0f, 1.0f, 0.0f),
                        windowWidth, windowHeight);
  }

  // Titulo
  renderer.RenderText("Toy universe statistics", 20, 60, 1.0f,
                      glm::vec3(0.7f, 0.8f, 1.0f),
                      windowWidth, windowHeight);

  // Status
  std::stringstream status;
  status << "Simulation Status: " << (pauseSimulation ? "Paused" : "Running");
  renderer.RenderText(status.str(), 20, 100, 0.7f,
                      glm::vec3(1.0f, 1.0f, 1.0f),
                      windowWidth, windowHeight);

  // Timer
  std::stringstream steps;
  steps << "Steps: " << timer;
  renderer.RenderText(steps.str(), 20, 130, 0.7f,
                      glm::vec3(1.0f, 1.0f, 1.0f),
                      windowWidth, windowHeight);

  // Console
  {
    std::lock_guard<std::mutex> lock(consoleMutex);
    int y = 160;
    for (auto& line : consoleLines) {
      renderer.RenderText(line, 20, y, 0.6f,
                          glm::vec3(0.0f, 1.0f, 0.0f),
                          windowWidth, windowHeight);
      y += 20;
    }
  }

  glfwSwapBuffers(window);
}

// Entrada de teclado
void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        consolePrintf("Stopping simulation...\n");
        stopSimulation = true;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
      case GLFW_KEY_SPACE:
        pauseSimulation = !pauseSimulation;
        consolePrintf(pauseSimulation ? "Simulation paused\n" : "Simulation resumed\n");
        break;
      case GLFW_KEY_T:
        consolePrintf("Test message at step %llu\n", timer);
        break;
    }
  }
}

int run() {
  if (!glfwInit()) {
    std::cerr << "Failed to init GLFW\n";
    return -1;
  }

  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Statistics", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to init GLAD\n";
    return -1;
  }

  glfwSetKeyCallback(window, keyCallback);

  // Inicializar TextRenderer
  // NOTE: You need to compile and load your text shader first
  unsigned int shaderID = framework::compileTextShader();
  TextRenderer renderer;
  if (!renderer.init("fonts/arial.ttf", 18, shaderID)) {
    std::cerr << "Failed to initialize TextRenderer\n";
    // Continue anyway or return -1
  }

  // Lancar thread de simulacao
  simulationThread = std::thread(SimulationLoop);

  while (!glfwWindowShouldClose(window)) {
    display(window, renderer);
    glfwPollEvents();
  }

  stopSimulation = true;
  if (simulationThread.joinable())
    simulationThread.join();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

// Coleta de dados
void collectData() {
  unsigned index3D = 0;
  for (unsigned x = 0; x < automaton::EL; x++)
    for (unsigned y = 0; y < automaton::EL; y++)
      for (unsigned z = 0; z < automaton::EL; z++) {
        automaton::Cell &cell = automaton::getCell(automaton::lattice_curr, x, y, z, 0);
        if (cell.t == cell.d) {
          // Process awakened cells here if needed
        }
        index3D++;
      }
}

} // namespace stats

// Wrapper global
int runStatistics() {
  return stats::run();
}