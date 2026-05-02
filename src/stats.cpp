/*
 * stats.cpp - Refactored single-window version with improved thread management
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include "model/simulation.h"
#include "stats.h"
#include "text_renderer.h"
#include "globals.h"
#include "shader.h"

extern unsigned long long timer;
extern TextRenderer* textRenderer;

namespace stats {

// Thread-safe state management
std::atomic<bool> simulationRunning{false};
std::atomic<bool> pauseSimulation{false};
std::atomic<bool> stopSimulation{false};
std::thread simulationThread;
std::chrono::steady_clock::time_point tbegin;

// Console management
std::vector<std::string> consoleLines;
std::mutex consoleMutex;
const int MAX_CONSOLE_LINES = 50;

// Renderer
static TextRenderer* rendererPtr = nullptr;

// Forward declarations
void stop();
bool start(GLFWwindow* window, TextRenderer* sharedRenderer);

// ============================================================================
// Console Logging
// ============================================================================

void consolePrintf(const char* format, ...) {
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  {
    std::lock_guard<std::mutex> lock(consoleMutex);
    consoleLines.push_back(std::string(buffer));
    if (consoleLines.size() > MAX_CONSOLE_LINES) {
      consoleLines.erase(consoleLines.begin());
    }
  }
  std::cout << buffer << std::flush;
}

// ============================================================================
// Simulation Thread
// ============================================================================

void SimulationLoop() {
  consolePrintf("Launching statistics simulation thread...\n");

  // Initialize simulation
  int step = 0;
  while (automaton::initSimulation(step++)) {}
  automaton::swap_lattices();

  tbegin = std::chrono::steady_clock::now();
  simulationRunning = true;

  // Main simulation loop
  while (!stopSimulation) {
    if (!pauseSimulation) {
      automaton::simulation();
      timer++;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
  }

  simulationRunning = false;
  consolePrintf("Statistics simulation thread ended.\n");
}

// ============================================================================
// Rendering
// ============================================================================

void renderStatusBar(int width, int height) {
  if (!rendererPtr) return;

  // Title
  rendererPtr->RenderText(
    "Toy Universe Statistics",
    20, height - 90,
    1.0f,
    glm::vec3(0.7f, 0.8f, 1.0f),
    width, height
  );

  // Elapsed time
  if (simulationRunning) {
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - tbegin
    ).count();

    std::stringstream ss;
    ss << "Elapsed: " << std::fixed << std::setprecision(1)
       << (millis / 1000.0) << " s";

    rendererPtr->RenderText(
      ss.str(),
      20, height - 50,
      0.8f,
      glm::vec3(1.0f, 1.0f, 0.0f),
      width, height
    );
  }

  // Status
  std::stringstream status;
  status << "Status: " << (pauseSimulation ? "PAUSED" : "RUNNING");
  rendererPtr->RenderText(
    status.str(),
    20, height - 130,
    0.7f,
    glm::vec3(1.0f, 1.0f, 1.0f),
    width, height
  );

  // Steps
  std::stringstream steps;
  steps << "Steps: " << timer;
  rendererPtr->RenderText(
    steps.str(),
    20, height - 160,
    0.7f,
    glm::vec3(1.0f, 1.0f, 1.0f),
    width, height
  );
}

void renderConsole(int width, int height) {
  if (!rendererPtr) return;

  std::lock_guard<std::mutex> lock(consoleMutex);

  int y = 40;
  for (auto it = consoleLines.rbegin();
       it != consoleLines.rend() && y < height - 200;
       ++it) {
    rendererPtr->RenderText(
      *it,
      20, y,
      0.55f,
      glm::vec3(0.0f, 1.0f, 0.3f),
      width, height
    );
    y += 18;
  }
}

void display(GLFWwindow* window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Initialize renderer if needed
  if (!rendererPtr && textRenderer) {
    rendererPtr = textRenderer;
  }

  if (!rendererPtr) return;

  renderStatusBar(width, height);
  renderConsole(width, height);
}

// ============================================================================
// Input Handling
// ============================================================================

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

  switch (key) {
    case GLFW_KEY_ESCAPE:
      stop();
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;

    case GLFW_KEY_SPACE:
      pauseSimulation = !pauseSimulation;
      consolePrintf(
        pauseSimulation ? "Simulation paused\n" : "Simulation resumed\n"
      );
      break;

    case GLFW_KEY_T:
      consolePrintf("Test message at step %llu\n", timer);
      break;
  }
}

// ============================================================================
// Lifecycle Management
// ============================================================================

bool isRunning() {
  return simulationThread.joinable() && simulationRunning;
}

bool start(GLFWwindow* window, TextRenderer* sharedRenderer) {
  // Auto-cleanup if already running
  if (simulationThread.joinable()) {
    consolePrintf("Statistics thread already running - stopping first...\n");
    stop();
  }

  // Setup renderer
  rendererPtr = sharedRenderer ? sharedRenderer : textRenderer;
  if (!rendererPtr) {
    std::cerr << "[Stats] ERROR: No TextRenderer available!\n";
    return false;
  }

  // Setup callbacks
  glfwSetKeyCallback(window, keyCallback);

  // Reset state
  consolePrintf("Initializing statistics mode...\n");
  stopSimulation = false;
  pauseSimulation = false;

  // Launch simulation thread
  try {
    simulationThread = std::thread(SimulationLoop);
  } catch (const std::exception& e) {
    std::cerr << "[Stats] ERROR: Failed to start thread: " << e.what() << "\n";
    return false;
  }

  consolePrintf("Statistics mode started successfully.\n");
  return true;
}

void stop() {
  if (!simulationThread.joinable()) {
    return;  // Already stopped
  }

  consolePrintf("Stopping statistics mode...\n");

  stopSimulation = true;

  // Wait for thread to finish with timeout protection
  if (simulationThread.joinable()) {
    simulationThread.join();
  }

  consolePrintf("Statistics mode stopped.\n");
}

void cleanup() {
  stop();
  consoleLines.clear();
  rendererPtr = nullptr;
}

} // namespace stats

// ============================================================================
// Global Entry Points
// ============================================================================

int runStatistics(GLFWwindow* window, TextRenderer* renderer) {
  if (!window || !renderer) {
    std::cerr << "[Stats] ERROR: Invalid window or renderer!\n";
    return -1;
  }

  if (!stats::start(window, renderer)) {
    std::cerr << "[Stats] ERROR: Failed to start statistics mode!\n";
    return -1;
  }

  return 0;
}

void stopStatistics() {
  stats::stop();
}

bool isStatisticsRunning() {
  return stats::isRunning();
}
