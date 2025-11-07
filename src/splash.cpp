/*
 * splash.cpp
 */

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

namespace splash
{
  int lattice_size = 21;
  int numLayers = 10;
  int selection = 0;
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

  // UI Elements
  Dropdown sizeDropdown(-0.85f, -0.25f, 0.4f, 0.08f, sizeOptions);
  Dropdown layerDropdown(-0.85f, -0.40f, 0.4f, 0.08f, layerOptions);

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

  float scenarioWidth = 0.6f;
  float rightMargin = 0.15f;
  float scenarioX = 1.0f - scenarioWidth - rightMargin;

  Dropdown scenarioDropdown(scenarioX,
                            sizeDropdown.y,
                            scenarioWidth,
                            0.08f,
                            scenarioOptions);
  // Calculate position in NDC, then convert for bottom-left origin
  float ndcX = scenarioDropdown.x;
  float ndcY = scenarioDropdown.y - 0.12f;

  // Convert NDC to pixel (bottom-left origin)
  int tickX = (int)((ndcX + 1.0f) * WINDOW_WIDTH / 2.0f);
  int tickY = (int)((ndcY + 1.0f) * WINDOW_HEIGHT / 2.0f);

  Tickbox startPausedBox(tickX, tickY, "Start paused");

  XButton btn1 = { -0.5f, -0.65f, 1.0f, 0.12f, "Simulation" };
  XButton btn2 = { -0.5f, -0.82f, 1.0f, 0.12f, "Statistics" };
  XButton helpLink = { -0.15f, -0.90f, 0.3f, 0.05f, "Help" };

  framework::Logo *logo_splash = nullptr;

  bool insideButton(int mx, int my, int w, int h, const XButton& b)
  {
      float x = (float)mx / w * 2.0f - 1.0f;
      float y = 1.0f - (float)my / h * 2.0f;
      return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
  }

  void drawButton(const XButton& b, bool isDefault = false)
  {
      glColor3f(0.1f, 0.1f, 0.1f);
      float offset = 0.02f;
      glBegin(GL_QUADS);
          glVertex2f(b.x + offset, b.y - offset);
          glVertex2f(b.x + b.w + offset, b.y - offset);
          glVertex2f(b.x + b.w + offset, b.y + b.h - offset);
          glVertex2f(b.x + offset, b.y + b.h - offset);
      glEnd();

      glColor3f(isDefault ? 0.3f : 0.2f, isDefault ? 0.7f : 0.6f, isDefault ? 0.9f : 0.8f);
      glBegin(GL_QUADS);
          glVertex2f(b.x, b.y);
          glVertex2f(b.x + b.w, b.y);
          glVertex2f(b.x + b.w, b.y + b.h);
          glVertex2f(b.x, b.y + b.h);
      glEnd();

      glColor3f(1.0f, 1.0f, 1.0f);
      const char* c = b.label;
      int textWidth = 0;
      while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c++);
      float tx = b.x + b.w/2 - (textWidth * 1.0f / WINDOW_WIDTH);
      float ty = b.y + b.h/2 - 0.015f;
      glRasterPos2f(tx, ty);
      c = b.label;
      while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c++);
  }

  void drawHyperlink(const XButton& b)
  {
      glColor3f(helpHover ? 0.1f : 0.0f, helpHover ? 0.4f : 0.0f, 1.0f);
      const char* c = b.label;
      int textWidth = 0;
      while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
      float tx = b.x + (b.w - (textWidth * 2.0f / WINDOW_WIDTH)) / 2;
      float ty = b.y + 0.015f;
      glRasterPos2f(tx, ty);
      c = b.label;
      while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c++);
      glLineWidth(1.0f);
      glBegin(GL_LINES);
          glVertex2f(tx, b.y + 0.005f);
          glVertex2f(tx + (textWidth * 2.0f / WINDOW_WIDTH), b.y + 0.005f);
      glEnd();
  }

  void drawLabel(const char* text, float x, float y)
  {
      glColor3f(0.0f, 0.0f, 0.0f);
      glRasterPos2f(x, y);
      for (const char* c = text; *c; ++c)
          glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
  }

  void drawControls()
  {
    drawLabel("Size", sizeDropdown.x, sizeDropdown.y + sizeDropdown.height + 0.01f);
    drawLabel("Layers", layerDropdown.x, layerDropdown.y + layerDropdown.height + 0.01f);
    drawLabel("Scenario", scenarioDropdown.x, scenarioDropdown.y + scenarioDropdown.height + 0.01f);

    bool sizeOpen     = sizeDropdown.isOpen;
    bool layerOpen    = layerDropdown.isOpen;
    bool scenarioOpen = scenarioDropdown.isOpen;

    // Draw all headers first
    sizeDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
    layerDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
    scenarioDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw buttons below dropdowns
    drawButton(btn1, true);
    drawButton(btn2, false);

    // Re-draw the open dropdown last to bring it to front
    if (sizeOpen)
        sizeDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
    else if (layerOpen)
        layerDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
    else if (scenarioOpen)
        scenarioDropdown.draw(WINDOW_WIDTH, WINDOW_HEIGHT);
    //
    // Draw the "Start paused" tickbox
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.0f, 1.0f, 1.0f);
    startPausedBox.draw();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
  }

  void drawTitle()
  {
    const char* title = "It from bit: a concrete attempt";
    glColor3f(0.4f, 0.7f, 1.0f);
    int textWidth = 0;
    const char* c = title;
    while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c++);
    float tx = -(textWidth * 2.0f / WINDOW_WIDTH) / 2.0f;
    float ty = 0.85f;
    for (float dx = -0.001f; dx <= 0.001f; dx += 0.001f)
      for (float dy = -0.001f; dy <= 0.001f; dy += 0.001f)
      {
        glRasterPos2f(tx + dx, ty + dy);
        c = title;
        while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c++);
      }
  }

  void display()
  {
    glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawTitle();
    drawHyperlink(helpLink);
    drawControls();
    if (logo_splash->valid())
    	logo_splash->draw(0.0f, 0.35f, 0.8f); // Centered, half-screen size
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

    // Handle "Start paused" checkbox click
    int mx = x;
    int my = WINDOW_HEIGHT - y; // convert to bottom-left origin
    if (startPausedBox.onMouseButton(mx, my, true))
    {
        glutPostRedisplay();
        return; // prevent other UI from also reacting to this click
    }

    bool clickedOnUI = isClickOnUI(x, y, WINDOW_WIDTH, WINDOW_HEIGHT);
    // Handle dropdown selections
    bool sizeSelected     = sizeDropdown.handleClick(x, y, WINDOW_WIDTH, WINDOW_HEIGHT);
    bool layerSelected    = layerDropdown.handleClick(x, y, WINDOW_WIDTH, WINDOW_HEIGHT);
    bool scenarioSelected = scenarioDropdown.handleClick(x, y, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Update values if selected
    if (sizeSelected)
        lattice_size = std::stoi(sizeDropdown.getSelectedItem());
    if (layerSelected)
        numLayers = std::stoi(layerDropdown.getSelectedItem());
    if (scenarioSelected)
      std::cout << "Scenario selected: " << scenarioDropdown.getSelectedItem() << std::endl;
    // Close other dropdowns when one is opened
    if (sizeDropdown.isOpen && !layerDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT)
        && !scenarioDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        layerDropdown.close(), scenarioDropdown.close();
    if (layerDropdown.isOpen && !sizeDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT)
        && !scenarioDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        sizeDropdown.close(), scenarioDropdown.close();
    if (scenarioDropdown.isOpen && !sizeDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT)
        && !layerDropdown.containsHeader(x, y, WINDOW_WIDTH, WINDOW_HEIGHT))
        sizeDropdown.close(), layerDropdown.close();
    // Only trigger buttons if not clicking on UI
    if (!clickedOnUI)
    {
      if (insideButton(x, y, WINDOW_WIDTH, WINDOW_HEIGHT, btn1))
      {
        automaton::calculateParameters(lattice_size, numLayers);
        if (!automaton::tryAllocate(lattice_size, numLayers))
          MessageBox(NULL, "Memory allocation failed. Try a smaller lattice size or fewer layers.", "Allocation Error", MB_OK | MB_ICONWARNING);
        else { selection = 1; glutLeaveMainLoop(); }
      }
      else if (insideButton(x, y, WINDOW_WIDTH, WINDOW_HEIGHT, btn2))
      {
        automaton::calculateParameters(lattice_size, numLayers);
        if (!automaton::tryAllocate(lattice_size, numLayers))
          MessageBox(NULL, "Memory allocation failed. Try a smaller lattice size or fewer layers.", "Allocation Error", MB_OK | MB_ICONWARNING);
        else { selection = 2; glutLeaveMainLoop(); }
      }
      else if (insideButton(x, y, WINDOW_WIDTH, WINDOW_HEIGHT, helpLink))
      {
        ShellExecuteA(NULL, "open", "https://github.com/automaton3d/automaton/blob/master/help.md", NULL, NULL, SW_SHOWNORMAL);
      }

      // Close dropdowns if clicked outside UI
      sizeDropdown.close();
      layerDropdown.close();
      scenarioDropdown.close();
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
        selection = 1;
        glutLeaveMainLoop();
      }
    }
  }

  void reshape(int w, int h)
  {
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  }

  void closeFunc() { shouldExit = true; }

  void passiveMotion(int x, int y)
  {
    helpHover = insideButton(x, y, WINDOW_WIDTH, WINDOW_HEIGHT, helpLink);
    glutPostRedisplay();
  }

}

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
  if (splash::selection == 1)
      return runSimulation(splash::scenarioDropdown.selectedIndex);
  if (splash::selection == 2)
      return runStatistics();
  return 0;
}
