/*
* stats_glut.cpp
*/
#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdarg>
#include "model/simulation.h"
#include "stats.h"

// Forward declarations from your simulation
namespace automaton
{
  extern unsigned EL;
  void swap_lattices();
  void simulation();
  bool initSimulation(int step);
  extern std::vector<Cell> lattice_curr;
}

namespace framework
{
  extern unsigned long long timer;
}

namespace stats
{

#define MAXTICKS 10000

int windowWidth = 600;
int windowHeight = 480;

// Thread control variables
volatile bool simulationRunning = false;
volatile bool pauseSimulation = false;
volatile bool stopSimulation = false;
HANDLE simulationThread = NULL;
unsigned long long tbegin = 0;

// Console output buffer
std::vector<std::string> consoleLines;
CRITICAL_SECTION consoleMutex;
const int MAX_CONSOLE_LINES = 50;

// Custom printf function that adds to console
void consolePrintf(const char* format, ...)
{
  char buffer[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  EnterCriticalSection(&consoleMutex);
  // Split by newlines
  std::string str(buffer);
  std::stringstream ss(str);
  std::string line;
  while (std::getline(ss, line))
  {
    if (!line.empty())
    {
      consoleLines.push_back(line);
      // Keep only last MAX_CONSOLE_LINES
      if (consoleLines.size() > MAX_CONSOLE_LINES)
      {
        consoleLines.erase(consoleLines.begin());
      }
    }
  }
  LeaveCriticalSection(&consoleMutex);
  // Also print to stdout
  std::cout << buffer;
}

// Simulation thread function
DWORD WINAPI SimulationThread(LPVOID lpParam)
{
  consolePrintf("Statistics simulation thread launched...\n");
  // Initialize simulation
  for (int step = 0; automaton::initSimulation(step); step++);
  consolePrintf("Simulation initialized, starting loop...\n");
  // Prepare the mirror grid before starting
  automaton::swap_lattices();
  tbegin = GetTickCount64();
  simulationRunning = true;
  unsigned long long lastReportTime = 0;
  const unsigned long long REPORT_INTERVAL = 1000; // Report every 1000 steps
  while (!stopSimulation)
  {
    if (!pauseSimulation)
    {
      automaton::simulation();
      collectData();
      framework::timer++;
      // Periodic status report
      if (framework::timer - lastReportTime >= REPORT_INTERVAL)
      {
        consolePrintf("Light: %llu  Tick: %llu\n", framework::timer / automaton::FRAME, framework::timer);
        lastReportTime = framework::timer;
      }
    }
    else
    {
      Sleep(80);
    }
  }
  simulationRunning = false;
  consolePrintf("Simulation thread ended.\n");
  return 0;
}

void drawConsole()
{
  // Calculate console area (middle section, above progress bar)
  float bottomMargin = 15.0f / windowHeight * 2.0f;  // bottom gap (~15 px)
  float progressBarHeight = 30.0f / windowHeight * 2.0f;
  float gapBetween = 10.0f / windowHeight * 2.0f;  // gap between progress bar and console
  float pixelShift   = 20.0f / windowHeight * 2.0f;  // upward offset (~20 px)
  float consoleHeight = 2.0f / 3.0f;
  float consoleBottom = -1.0f + bottomMargin + progressBarHeight + gapBetween;
  float consoleTop    = consoleBottom + consoleHeight + pixelShift;
  float consoleWidth  = 0.995f; // nearly full width
  // --- Draw console background ---
  glColor3f(0.0f, 0.0f, 0.0f); // black background
  glBegin(GL_QUADS);
    glVertex2f(-consoleWidth, consoleBottom);
    glVertex2f( consoleWidth, consoleBottom);
    glVertex2f( consoleWidth, consoleTop);
    glVertex2f(-consoleWidth, consoleTop);
  glEnd();
  // --- Draw console border ---
  glColor3f(0.3f, 0.3f, 0.3f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(-consoleWidth, consoleBottom);
    glVertex2f( consoleWidth, consoleBottom);
    glVertex2f( consoleWidth, consoleTop);
    glVertex2f(-consoleWidth, consoleTop);
  glEnd();
  // --- Draw console text ---
  EnterCriticalSection(&consoleMutex);
  glColor3f(0.0f, 1.0f, 0.0f); // green text
  int charHeight = 15; // height of GLUT_BITMAP_8_BY_13 with spacing
  int consoleHeightPixels = (int)((consoleTop - consoleBottom) * 0.5f * windowHeight);
  int maxVisibleLines = (consoleHeightPixels - 10) / charHeight; // Reduced top margin
  int startIdx = 0;
  if (consoleLines.size() > (size_t)maxVisibleLines)
    startIdx = consoleLines.size() - maxVisibleLines;
  float lineHeight = (consoleTop - consoleBottom - 0.05f) / (maxVisibleLines > 0 ? maxVisibleLines : 1); // Reduced margin
  float verticalCenterShift = 6.0f / windowHeight * 2.0f; // Shift down 3 pixels
  float textY = consoleTop - 0.04f - verticalCenterShift; // Start closer to top, shifted down
  for (int i = startIdx; i < (int)consoleLines.size(); ++i)
  {
    glRasterPos2f(-consoleWidth + 0.01f, textY);
    const char* text = consoleLines[i].c_str();
    while (*text)
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text++);
    textY -= lineHeight;
  }
  LeaveCriticalSection(&consoleMutex);
}

void drawProgressBar()
{
  // Progress bar at the bottom with margin
  float bottomMargin = 15.0f / windowHeight * 2.0f;  // bottom gap (~15 px)
  float barHeight = 30.0f / windowHeight * 2.0f;
  float barBottom = -1.0f + bottomMargin;
  float barTop = barBottom + barHeight;
  float barWidth = 0.995f;

  // Background
  glColor3f(0.2f, 0.2f, 0.2f);
  glBegin(GL_QUADS);
    glVertex2f(-barWidth, barBottom);
    glVertex2f( barWidth, barBottom);
    glVertex2f( barWidth, barTop);
    glVertex2f(-barWidth, barTop);
  glEnd();

  // Progress fill
  float progress = (float)(framework::timer % MAXTICKS) / MAXTICKS;
  float fillWidth = barWidth * 2.0f * progress;
  glColor3f(0.2f, 0.6f, 0.2f); // Green progress
  glBegin(GL_QUADS);
    glVertex2f(-barWidth, barBottom);
    glVertex2f(-barWidth + fillWidth, barBottom);
    glVertex2f(-barWidth + fillWidth, barTop);
    glVertex2f(-barWidth, barTop);
  glEnd();

  // Border
  glColor3f(0.5f, 0.5f, 0.5f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(-barWidth, barBottom);
    glVertex2f( barWidth, barBottom);
    glVertex2f( barWidth, barTop);
    glVertex2f(-barWidth, barTop);
  glEnd();

  // Progress text
  glColor3f(1.0f, 1.0f, 1.0f);
  char progressText[64];
  sprintf(progressText, "%llu / %d", framework::timer % MAXTICKS, MAXTICKS);
  float textVerticalShift = 10.0f / windowHeight * 2.0f;
  glRasterPos2f(-0.08f, barBottom + 0.008f + textVerticalShift);
  const char* ptr = progressText;
  while (*ptr)
  {
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *ptr);
    ptr++;
  }
}

void display()
{
  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw elapsed time at upper left corner
  if (tbegin > 0)
  {
    unsigned long millis = GetTickCount64() - tbegin;
    char s[100];
    sprintf(s, "Elapsed %.1fs", millis / 1000.0);
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow color
    glRasterPos2f(-0.95f, 0.92f);
    const char* ptr = s;
    while (*ptr)
    {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *ptr);
      ptr++;
    }
  }

  float pixelShift = 60.0f / windowHeight * 2.0f;
  float newTop = 0.95f - pixelShift;
  float newBottom = 0.25f - pixelShift;
  float upShift20 = 50.0f / windowHeight * 2.0f;
  // --- Draw Title "Toy universe" (font 24, white blue) ---
  glColor3f(0.7f, 0.8f, 1.0f); // Light blue for "white blue" effect
  glRasterPos2f(-0.90f, (newTop - 0.05f) + upShift20);
  const char* mainTitle = "Toy universe";
  int titlePixelWidth = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)mainTitle);
  float normalizedTitleWidth = (float)titlePixelWidth / windowWidth * 2.0f;
  float centerX = 0.0f;
  float newX = centerX - (normalizedTitleWidth / 2.0f);
  glRasterPos2f(newX, (newTop - 0.05f) + upShift20);
  while (*mainTitle)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *mainTitle);
    mainTitle++;
  }
  // Draw main stats area (top 1/3, full width)
  glColor3f(0.3f, 0.3f, 0.3f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(-0.99f, newBottom); // Modified Y
    glVertex2f( 0.99f, newBottom); // Modified Y
    glVertex2f( 0.99f, newTop);    // Modified Y
    glVertex2f(-0.99f, newTop);    // Modified Y
  glEnd();
  // Draw status text in main area
  glColor3f(1.0f, 1.0f, 1.0f);
  // Status - Shift this text down by the same amount
  glRasterPos2f(-0.90f, 0.75f - pixelShift); // Modified Y
  const char* statusText = "Simulation Status: ";
  while (*statusText)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *statusText);
    statusText++;
  }
  glColor3f(1.0f, 1.0f, 1.0f);
  glRasterPos2f(-0.90f, 0.55f - pixelShift); // Modified Y
  const char* timerLabel = "Steps: ";
  while (*timerLabel)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *timerLabel);
    timerLabel++;
  }
  char timerStr[64];
  sprintf(timerStr, "%llu", framework::timer);
  const char* timerPtr = timerStr;
  while (*timerPtr)
  {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *timerPtr);
    timerPtr++;
  }
  drawConsole();
  drawProgressBar();
  glutSwapBuffers();
}

void reshape(int w, int h)
{
  windowWidth = w;
  windowHeight = h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Orthographic projection
  if (w >= h)
    glOrtho(-1.0*w/h, 1.0*w/h, -1.0, 1.0, -1.0, 1.0);
  else
    glOrtho(-1.0, 1.0, -1.0*h/w, 1.0*h/w, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void idle()
{
  glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
  switch(key)
  {
    case 27: // ESC key
      consolePrintf("Stopping simulation...\n");
      stopSimulation = true;

      // Wait for thread to finish
      if (simulationThread != NULL)
      {
        WaitForSingleObject(simulationThread, 2000); // Wait up to 2 seconds
        CloseHandle(simulationThread);
      }
      exit(0);
      break;

    case ' ': // Space bar - pause/resume
      pauseSimulation = !pauseSimulation;
      consolePrintf(pauseSimulation ? "Simulation paused\n" : "Simulation resumed\n");
      break;

    case 't':
    case 'T':
      // Test console output
      consolePrintf("Test message at step %llu\n", framework::timer);
      break;
  }
}

void closeFunc()
{
  stopSimulation = true;
  if (simulationThread != NULL)
  {
    WaitForSingleObject(simulationThread, 2000);
    CloseHandle(simulationThread);
  }
}

int run()
{
  // Initialize critical section
  InitializeCriticalSection(&consoleMutex);

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("Statistics");

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutCloseFunc(closeFunc);

  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

  // Launch the simulation thread
  consolePrintf("Launching simulation thread...\n");
  DWORD dwThreadId;
  simulationThread = CreateThread(NULL, 0, SimulationThread, NULL, 0, &dwThreadId);

  if (simulationThread == NULL)
  {
    std::cerr << "Failed to create simulation thread!" << std::endl;
    DeleteCriticalSection(&consoleMutex);
    return -1;
  }
  glutMainLoop(); // blocks here until window is closed
  // Cleanup
  consolePrintf("Window closed, stopping simulation...\n");
  stopSimulation = true;
  if (simulationThread != NULL)
  {
    WaitForSingleObject(simulationThread, 2000);
    CloseHandle(simulationThread);
  }
  // Delete critical section
  DeleteCriticalSection(&consoleMutex);
  return 0;
}

/**
 * This collects data to build the statistical graphs.
 */
void collectData()
{
  // Iterate over the 3D grid to update the voxel data
  unsigned index3D = 0;
  for (unsigned x = 0; x < automaton::EL; x++)
  {
    for (unsigned y = 0; y < automaton::EL; y++)
    {
      for (unsigned z = 0; z < automaton::EL; z++)
      {
        automaton::Cell &cell = automaton::getCell(automaton::lattice_curr, x, y, z, 0);
        if (cell.t == cell.d)
        {
        }
        index3D++;
      }
    }
  }
}

} // namespace stats

// Global wrapper for splash.cpp linkage
int runStatistics()
{
  char arg0[] = "test";
  char *argv[] = { arg0, NULL };
  int argc = 1;
  glutInit(&argc, argv);
  return stats::run();
}
