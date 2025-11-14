/*
 * splash.cpp
 */

#include <button.h>
#include <windows.h>
#include <GL/freeglut.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include "dropdown.h"
#include "splash.h"
#include "model/simulation.h"
#include "logo.h"
#include "tickbox.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Draw a raised rectangular panel in pixel coordinates
void drawRaisedPanel(float x, float y, float w, float h)
{
    float x1 = x;
    float y1 = y;
    float x2 = x + w;
    float y2 = y + h;
    float bevel = 2.0f; // bevel in pixels

    // Dark shadow (bottom and right edges)
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
        // Bottom edge
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y1 - bevel);
        glVertex2f(x1, y1 - bevel);
        // Right edge
        glVertex2f(x2, y1);
        glVertex2f(x2 + bevel, y1);
        glVertex2f(x2 + bevel, y2);
        glVertex2f(x2, y2);
    glEnd();

    // Light highlight (top and left edges)
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        // Top edge
        glVertex2f(x1, y2);
        glVertex2f(x2, y2);
        glVertex2f(x2, y2 + bevel);
        glVertex2f(x1, y2 + bevel);
        // Left edge
        glVertex2f(x1, y1);
        glVertex2f(x1, y2);
        glVertex2f(x1 - bevel, y2);
        glVertex2f(x1 - bevel, y1);
    glEnd();

    // Inner fill
    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
    glEnd();
}

namespace automaton
{
  extern int scenario;
}

namespace splash
{
  int lattice_size = 21;
  int numLayers = 10;
  int selection = -1;
  bool shouldExit = false;
  bool helpHover = false;

  // Dropdown options
  std::vector<std::string> sizeOptions = {
      "5", "7", "9", "11", "13", "15", "17", "19", "21", "23", "25", "27", "29", "31",
      "35", "37", "39", "41", "43", "45", "47", "49", "51", "53", "55", "57", "59",
      "61", "63", "65", "67", "69", "71", "73", "75", "77", "79", "81", "83", "85", "87", "89"
  };

  std::vector<std::string> layerOptions = {
      "10", "12", "14", "16", "18", "20", "24", "28", "32", "38", "44",
      "52", "60", "70", "82", "96", "112", "130", "150", "174",
      "200", "230", "264", "300", "320", "340", "360", "364"
  };
  std::vector<std::string> scenarioOptions =
  {
    "Wrapping test",
    "Relocate test",
    "Orphan test",
    "Contraction test",
    "Hunting test",
    "Reissue test",
    "Dispersion test",
    "Full simulation"
  };

  framework::Logo *logo_splash = nullptr;
  Dropdown sizeDropdown(50, 340, 120, 30, sizeOptions);
  Dropdown layerDropdown(50, 270, 120, 30, layerOptions);
  Tickbox startPausedBox(200, 240, "Start paused");
  Dropdown scenarioDropdown(200, 340, 200, 30, scenarioOptions);
  Button simBtn(200, 190, 200, 40, "Simulation");
  Button statBtn(200, 130, 200, 40, "Statistics");
  Button replayBtn(200, 60, 200, 40, "Replay");
  Button helpLink((WINDOW_WIDTH - 100) / 2, 20, 100, 25, "Help");

  void drawLabel(const char* text, float x, float y)
  {
      glColor3f(0.0f, 0.0f, 0.0f);
      glRasterPos2f(x, y);
      for (const char* c = text; *c; ++c)
          glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }

  void drawControls()
  {
	  drawRaisedPanel(40, 115, WINDOW_WIDTH - 80, 290);
	  drawRaisedPanel(185, 180, 232, 90);
      // Labels
      drawLabel("Size", sizeDropdown.x, 380);
      drawLabel("Layers", layerDropdown.x, 310);
      drawLabel("Scenario", scenarioDropdown.x, 380);
      // Tickbox
      startPausedBox.draw();
      // Buttons
      simBtn.draw(true);
      replayBtn.draw(true);
      statBtn.draw(true);
      // Dropdowns
      layerDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
      sizeDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
      scenarioDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
      // Help
      helpLink.drawAsHyperlink(helpHover);

  }

  void drawTitle()
  {
      const char* title = "It from bit: a concrete attempt";
      glColor3f(0.4f, 0.7f, 1.0f);

      // Compute text width in pixels
      int textWidth = 0;
      for (const char* c = title; *c; ++c)
          textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);

      // Center horizontally in window
      int tx = (WINDOW_WIDTH - textWidth) / 2;
      int ty = WINDOW_HEIGHT - 50; // 50 pixels down from top

      // Draw with a subtle shadow effect
      for (int dx = -1; dx <= 1; ++dx)
          for (int dy = -1; dy <= 1; ++dy)
          {
              glRasterPos2i(tx + dx, ty + dy);
              for (const char* c = title; *c; ++c)
                  glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
          }
  }

  void display()
  {
      glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      drawTitle();

      // Logo centered below title
      float scale = 0.21f;
      int logoW = static_cast<int>(logo_splash->width()  * scale);
      int logoH = static_cast<int>(logo_splash->height() * scale);
      int logoX = (WINDOW_WIDTH - logoW) / 2;
      int titleBaseline = WINDOW_HEIGHT - 50;
      int logoY = titleBaseline - logoH - 20;
      logo_splash->draw(logoX, logoY, scale);

      // Widgets block (dropdowns, tickbox, buttons) just above help link
      drawControls();

      // Help link at bottom center
      helpLink.drawAsHyperlink(helpHover);

      glutSwapBuffers();
  }

  bool isClickOnUI(int x, int y, int windowWidth, int windowHeight)
  {
    return sizeDropdown.containsHeader(x, y, windowWidth, windowHeight) ||
           layerDropdown.containsHeader(x, y, windowWidth, windowHeight) ||
           scenarioDropdown.containsHeader(x, y, windowWidth, windowHeight) ||
           sizeDropdown.containsDropdown(x, y, windowWidth, windowHeight) ||
           layerDropdown.containsDropdown(x, y, windowWidth, windowHeight) ||
           scenarioDropdown.containsDropdown(x, y, windowWidth, windowHeight);
  }

  void mouse(int button, int state, int x, int y)
  {
      if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
          return;

      // Convert GLUT’s top-left origin to bottom-left origin
      int mx = x;
      int my = WINDOW_HEIGHT - y;

      if (startPausedBox.onMouseButton(mx, my, true)) {
          glutPostRedisplay();
          return;
      }

      bool sizeSelected     = sizeDropdown.handleClick(mx, my, WINDOW_WIDTH, WINDOW_HEIGHT);
      bool layerSelected    = layerDropdown.handleClick(mx, my, WINDOW_WIDTH, WINDOW_HEIGHT);
      bool scenarioSelected = scenarioDropdown.handleClick(mx, my, WINDOW_WIDTH, WINDOW_HEIGHT);

      if (sizeSelected) lattice_size = std::stoi(sizeDropdown.getSelectedItem());
      if (layerSelected) numLayers = std::stoi(layerDropdown.getSelectedItem());
      if (scenarioSelected) std::cout << "Scenario: " << scenarioDropdown.getSelectedItem() << std::endl;

      // Buttons
      if (simBtn.contains(mx, my)) {
          automaton::calculateParameters(lattice_size, numLayers);
          if (!automaton::tryAllocate(lattice_size, numLayers)) {
              MessageBox(NULL,
                  "Memory allocation failed. Try a smaller lattice size or fewer layers.",
                  "Allocation Error", MB_OK | MB_ICONWARNING);
          } else {
              selection = 0;
              glutLeaveMainLoop();
          }
      }
      else if (replayBtn.contains(mx, my)) {
          automaton::calculateParameters(lattice_size, numLayers);
          if (!automaton::tryAllocate(lattice_size, numLayers)) {
              MessageBox(NULL,
                  "Memory allocation failed. Try a smaller lattice size or fewer layers.",
                  "Allocation Error", MB_OK | MB_ICONWARNING);
          } else {
              selection = 1;
              glutLeaveMainLoop();
          }
      }
      else if (statBtn.contains(mx, my)) {
          automaton::calculateParameters(lattice_size, numLayers);
          if (!automaton::tryAllocate(lattice_size, numLayers)) {
              MessageBox(NULL,
                  "Memory allocation failed. Try a smaller lattice size or fewer layers.",
                  "Allocation Error", MB_OK | MB_ICONWARNING);
          } else {
              automaton::scenario = 7; // full simulation for statistics
              selection = 2;
              glutLeaveMainLoop();
          }
      }
      else if (helpLink.contains(mx, my)) {
          ShellExecuteA(NULL, "open", "https://github.com/automaton3d/automaton/blob/master/help.md", NULL, NULL, SW_SHOWNORMAL);
      }

      glutPostRedisplay();
  }

  void mouseWheel(int button, int dir, int x, int y)
  {
    if (sizeDropdown.isOpen && sizeDropdown.containsDropdown(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        sizeDropdown.scroll(dir > 0 ? 1 : -1);
    else if (layerDropdown.isOpen && layerDropdown.containsDropdown(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        layerDropdown.scroll(dir > 0 ? 1 : -1);
    else if (scenarioDropdown.isOpen && scenarioDropdown.containsDropdown(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        scenarioDropdown.scroll(dir > 0 ? 1 : -1);
    glutPostRedisplay();
  }

  void keyboard(unsigned char key, int, int)
  {
    if (key == 13) // Enter
    {
      automaton::calculateParameters(lattice_size, numLayers);
      if (!automaton::tryAllocate(lattice_size, numLayers))
      {
        MessageBox(NULL, "Memory allocation failed. Try a smaller lattice size or fewer layers.", "Error", MB_OK | MB_ICONERROR);
      }
      else
      {
        selection = 0;
        glutLeaveMainLoop();
      }
    }
  }

  void reshape(int w, int h)
  {
      glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);  // pixel coordinates
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
  }

  void closeFunc() {
      if (selection == 0 || selection == 1 || selection == 2) {
          // valid selection already made — do not override
          return;
      }
      selection = -1; // mark as exit
  }

  void passiveMotion(int x, int y)
  {
    helpHover = helpLink.contains(x, y);
    glutPostRedisplay();
  }
} // end namespace splash

void reshape(int w, int h)
{
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);  // NDC coordinates
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition(
      (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,
      (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2
  );
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutCreateWindow("Toy Universe");
  // Set up projection matrices for NDC
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  splash::sizeDropdown.selectByValue("21");
  splash::layerDropdown.selectByValue("10");
  splash::lattice_size = 21;
  splash::numLayers = 10;
  glutReshapeFunc(splash::reshape);
  glutDisplayFunc(splash::display);
  glutMouseFunc(splash::mouse);
  glutKeyboardFunc(splash::keyboard);
  glutCloseFunc(splash::closeFunc);
  glutPassiveMotionFunc(splash::passiveMotion);
  #ifdef __FREEGLUT_EXT_H__
  glutMouseWheelFunc(splash::mouseWheel);
  #endif
  splash::logo_splash = new framework::Logo("logo_bar.png");
  float border[3] = {0.0f, 0.0f, 0.0f};
  float label[3]  = {0.1f, 0.1f, 0.1f};
  float fillOn[3] = {0.0f, 0.7f, 0.0f};
  float fillOff[3]= {0.7f, 0.7f, 0.7f};
  splash::startPausedBox.setColor(border, label, fillOn, fillOff);

  glutMainLoop();

  // Decide what to do after leaving the splash
  switch (splash::selection) {
      case -1:
          // User closed the splash window → exit quietly
          return 0;

      case 0:
          // Simulation button pressed
          return runSimulation(
              splash::scenarioDropdown.getSelectedIndex(),
              splash::startPausedBox.getState()
          );

      case 1:
          // Replay button pressed
          return runReplay();

      case 2:
          // Statistics button pressed
          return runStatistics();

      default:
          // No valid selection → exit quietly
          return 0;
  }
}
