/*
 * splash.cpp
 */
#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>
#include <cstdio>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ----------------------------------------------------------
// Button structure
struct Button
{
  float x, y, w, h;
  const char* label;
};

// ----------------------------------------------------------
// Selection buttons (positioned at bottom)
Button btn1 = { -0.5f, -0.65f, 1.0f, 0.12f, "Simulation" };
Button btn2 = { -0.5f, -0.82f, 1.0f, 0.12f, "Statistics" };

// ----------------------------------------------------------
// Size selection controls

const int sizes[] = {3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31};
const int numSizes = sizeof(sizes) / sizeof(sizes[0]);
int lattice_size = 11;  // Default stays 11
int sizeIndex = 4;      // Index of 11 in new array (0-based: 3,5,7,9,11 -> index 4)
bool sizeDropdownOpen = false;
Button sizeDropBox = { -0.85f, -0.25f, 0.4f, 0.08f, "11" };

// ----------------------------------------------------------
// Layer selection controls
int numLayers = 5;
const int maxLayers = 10;
bool useStandard = false;
bool dropdownOpen = false;
Button dropBox = { -0.85f, -0.40f, 0.4f, 0.08f, "5" };
Button chkStandard = { 0.15f, -0.40f, 0.08f, 0.08f, "" };

// ----------------------------------------------------------
// Logo texture
GLuint logoTexture = 0;
int logoWidth = 0, logoHeight = 0;

// ----------------------------------------------------------
// Window dimensions
const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 700;

// ----------------------------------------------------------
volatile int selection = 0;
volatile bool shouldExit = false;

// Forward declarations
int runSimulation(unsigned EL, unsigned W);
int runStatistics(unsigned EL, unsigned W);

// ----------------------------------------------------------
// Helper function for button detection
bool insideButton(int mx, int my, int w, int h, const Button& b)
{
  float x = (float)mx / w * 2.0f - 1.0f;
  float y = 1.0f - (float)my / h * 2.0f;
  return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
}

// ----------------------------------------------------------
// Load PNG image and create texture
bool loadLogo(const char* filename)
{
  int channels;
  unsigned char* data = stbi_load(filename, &logoWidth, &logoHeight, &channels, 4);
  if (!data)
  {
    std::cerr << "Failed to load logo: " << filename << std::endl;
    return false;
  }
  glGenTextures(1, &logoTexture);
  glBindTexture(GL_TEXTURE_2D, logoTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logoWidth, logoHeight,
               0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);
  return true;
}

// ----------------------------------------------------------
// Draw logo (moved down 30 pixels)
void drawLogo()
{
  if (logoTexture == 0) return;
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, logoTexture);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  float size = 0.8f;
  float yPos = 0.33f;  // Moved down ~30 pixels from 0.45f
  glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-size/2, yPos - size/2);
    glTexCoord2f(1, 1); glVertex2f( size/2, yPos - size/2);
    glTexCoord2f(1, 0); glVertex2f( size/2, yPos + size/2);
    glTexCoord2f(0, 0); glVertex2f(-size/2, yPos + size/2);
  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------
// Draw a button
void drawButton(const Button& b, bool isDefault = false)
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

  glColor3f(isDefault ? 0.5f : 0.3f, isDefault ? 0.9f : 0.7f, isDefault ? 1.0f : 0.9f);
  glLineWidth(isDefault ? 3.0f : 2.0f);
  glBegin(GL_LINE_LOOP);
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

// ----------------------------------------------------------
// Checkbox drawing
void drawCheckbox(const Button& b, bool checked, const char* label)
{
  glColor3f(0.9f, 0.9f, 0.9f);
  glBegin(GL_QUADS);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x + b.w, b.y);
    glVertex2f(b.x + b.w, b.y + b.h);
    glVertex2f(b.x, b.y + b.h);
  glEnd();

  if (checked)
  {
    glColor3f(0.2f, 0.6f, 1.0f);
    glBegin(GL_QUADS);
      glVertex2f(b.x + 0.015f, b.y + 0.015f);
      glVertex2f(b.x + b.w - 0.015f, b.y + 0.015f);
      glVertex2f(b.x + b.w - 0.015f, b.y + b.h - 0.015f);
      glVertex2f(b.x + 0.015f, b.y + b.h - 0.015f);
    glEnd();
  }

  glColor3f(0, 0, 0);
  glRasterPos2f(b.x + b.w + 0.02f, b.y + 0.015f);
  const char* ch = label;
  while (*ch) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
}

// ----------------------------------------------------------
// Main dropdown box (no expanded items)
void drawSizeDropdown(const Button& b)
{
  glColor3f(0.2f, 0.2f, 0.2f);
  glBegin(GL_QUADS);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x + b.w, b.y);
    glVertex2f(b.x + b.w, b.y + b.h);
    glVertex2f(b.x, b.y + b.h);
  glEnd();

  glColor3f(0.95f, 0.95f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(b.x + 0.005f, b.y + 0.005f);
    glVertex2f(b.x + b.w - 0.005f, b.y + 0.005f);
    glVertex2f(b.x + b.w - 0.005f, b.y + b.h - 0.005f);
    glVertex2f(b.x + 0.005f, b.y + b.h - 0.005f);
  glEnd();

  glColor3f(0, 0, 0);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x + b.w, b.y);
    glVertex2f(b.x + b.w, b.y + b.h);
    glVertex2f(b.x, b.y + b.h);
  glEnd();

  glColor3f(0, 0, 0);
  glRasterPos2f(b.x + 0.02f, b.y + 0.02f);
  char label[8];
  sprintf(label, "%d", lattice_size);
  const char* ch = label;
  while (*ch) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
}

// ----------------------------------------------------------
// Expanded size dropdown - single outer border wrapping first number
void drawExpandedSizeDropdown(const Button& b)
{
  float itemHeight = b.h;
  float spacing = 0.015f;
  std::vector<float> itemYPositions;

  // Collect all visible item positions
  for (int i = 0; i < numSizes; i++)
  {
    float yItem = b.y - (i + 1) * itemHeight - i * spacing;
    if (yItem + itemHeight < -0.95f) break;
    itemYPositions.push_back(yItem);
  }

  if (itemYPositions.empty()) return;

  // Calculate border to wrap ALL items including first
  float firstItemY = itemYPositions[0];
  float lastItemY = itemYPositions.back();
  float borderTop = firstItemY + itemHeight + 0.005f;  // Above first item
  float borderBottom = lastItemY - 0.005f;  // Below last item
  float borderLeft = b.x - 0.015f;
  float borderRight = b.x + b.w + 0.015f;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Single background
  glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Single outer border
  glColor3f(0.2f, 0.2f, 0.2f);
  glLineWidth(2.5f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Draw items
  for (size_t i = 0; i < itemYPositions.size(); i++)
  {
    float yItem = itemYPositions[i];
    int sizeValue = sizes[i];
    bool isSelected = (sizeValue == lattice_size);

    if (isSelected)
    {
      glColor4f(0.7f, 0.9f, 1.0f, 0.8f);
      glBegin(GL_QUADS);
        glVertex2f(b.x, yItem);
        glVertex2f(b.x + b.w, yItem);
        glVertex2f(b.x + b.w, yItem + itemHeight);
        glVertex2f(b.x, yItem + itemHeight);
      glEnd();
    }

    glColor3f(0, 0, 0);
    char itemLabel[8];
    sprintf(itemLabel, "%d", sizeValue);
    glRasterPos2f(b.x + 0.03f, yItem + 0.015f);
    const char* ch = itemLabel;
    while (*ch) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
  }

  glDisable(GL_BLEND);
}

// ----------------------------------------------------------
// Main layers dropdown box
void drawLayersDropdown(const Button& b)
{
  glColor3f(0.2f, 0.2f, 0.2f);
  glBegin(GL_QUADS);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x + b.w, b.y);
    glVertex2f(b.x + b.w, b.y + b.h);
    glVertex2f(b.x, b.y + b.h);
  glEnd();

  glColor3f(0.95f, 0.95f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(b.x + 0.005f, b.y + 0.005f);
    glVertex2f(b.x + b.w - 0.005f, b.y + 0.005f);
    glVertex2f(b.x + b.w - 0.005f, b.y + b.h - 0.005f);
    glVertex2f(b.x + 0.005f, b.y + b.h - 0.005f);
  glEnd();

  glColor3f(0, 0, 0);
  glLineWidth(2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x + b.w, b.y);
    glVertex2f(b.x + b.w, b.y + b.h);
    glVertex2f(b.x, b.y + b.h);
  glEnd();

  glColor3f(0, 0, 0);
  char label[8];
  sprintf(label, "%d", useStandard ? maxLayers : numLayers);
  glRasterPos2f(b.x + 0.02f, b.y + 0.02f);
  const char* ch = label;
  while (*ch) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
}

// ----------------------------------------------------------
// Expanded layers dropdown - fixed border
void drawExpandedLayersDropdown(const Button& b)
{
  float itemHeight = b.h;
  float spacing = 0.015f;
  std::vector<float> itemYPositions;
  std::vector<int> itemValues;

  // Collect positions
  for (int i = 2; i <= maxLayers; i++)
  {
    float yItem = b.y - (i - 1) * itemHeight - (i - 2) * spacing;
    if (yItem + itemHeight < -0.95f) break;
    itemYPositions.push_back(yItem);
    itemValues.push_back(i);
    if (itemYPositions.size() >= 8) break;
  }

  if (itemYPositions.empty()) return;

  // Border calculation
  float firstItemY = itemYPositions[0];
  float lastItemY = itemYPositions.back();
  float borderTop = firstItemY + itemHeight + 0.005f;
  float borderBottom = lastItemY - 0.005f;
  float borderLeft = b.x - 0.015f;
  float borderRight = b.x + b.w + 0.015f;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Single background
  glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Single outer border
  glColor3f(0.2f, 0.2f, 0.2f);
  glLineWidth(2.5f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Draw items
  for (size_t idx = 0; idx < itemYPositions.size(); idx++)
  {
    float yItem = itemYPositions[idx];
    int layerValue = itemValues[idx];
    bool isSelected = (layerValue == numLayers);

    if (isSelected)
    {
      glColor4f(0.7f, 0.9f, 1.0f, 0.8f);
      glBegin(GL_QUADS);
        glVertex2f(b.x, yItem);
        glVertex2f(b.x + b.w, yItem);
        glVertex2f(b.x + b.w, yItem + itemHeight);
        glVertex2f(b.x, yItem + itemHeight);
      glEnd();
    }

    glColor3f(0, 0, 0);
    char itemLabel[8];
    sprintf(itemLabel, "%d", layerValue);
    glRasterPos2f(b.x + 0.03f, yItem + 0.015f);
    const char* ch = itemLabel;
    while (*ch) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *ch++);
  }

  glDisable(GL_BLEND);
}

// ----------------------------------------------------------
// Draw controls (main boxes only)
void drawControls()
{
  // Size controls
  glColor3f(0, 0, 0);
  glRasterPos2f(-0.85f, -0.15f);
  const char* sizeLbl = "Size:";
  while (*sizeLbl) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *sizeLbl++);
  drawSizeDropdown(sizeDropBox);

  // Layer controls
  glColor3f(0, 0, 0);
  glRasterPos2f(-0.85f, -0.30f);
  const char* layerLbl = "Layers:";
  while (*layerLbl) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *layerLbl++);
  drawLayersDropdown(dropBox);
  drawCheckbox(chkStandard, useStandard, "Use standard (max)");
}

// ----------------------------------------------------------
// Draw title
void drawTitle()
{
  const char* title = "It from bit: a concrete attempt";
  glColor3f(0.4f, 0.7f, 1.0f);
  int textWidth = 0;
  const char* c = title;
  while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c++);
  float tx = -(textWidth * 2.0f / WINDOW_WIDTH) / 2.0f;
  float ty = 0.75f;
  for (float dx = -0.001f; dx <= 0.001f; dx += 0.001f)
    for (float dy = -0.001f; dy <= 0.001f; dy += 0.001f)
    {
      glRasterPos2f(tx + dx, ty + dy);
      c = title;
      while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c++);
    }
}

// ----------------------------------------------------------
// Main display function
void display()
{
  if (shouldExit) return;

  glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw background elements FIRST
  drawTitle();
  drawLogo();
  drawControls();        // Main dropdown boxes only
  drawButton(btn1, true);
  drawButton(btn2, false);

  // Draw EXPANDED DROPDOWNS LAST (on top)
  if (sizeDropdownOpen)
  {
    drawExpandedSizeDropdown(sizeDropBox);
  }

  if (dropdownOpen && !useStandard)
  {
    drawExpandedLayersDropdown(dropBox);
  }

  glutSwapBuffers();
}

// ----------------------------------------------------------
// Mouse handling
void mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    int w = WINDOW_WIDTH;
    int h = WINDOW_HEIGHT;

    if (insideButton(x, y, w, h, btn1)) {
      selection = 1;
      shouldExit = true;
    }
    else if (insideButton(x, y, w, h, btn2)) {
      selection = 2;
      shouldExit = true;
    }
    else if (insideButton(x, y, w, h, chkStandard))
    {
      useStandard = !useStandard;
      if (useStandard) {
        numLayers = maxLayers;
        dropdownOpen = false;
        sizeDropdownOpen = false;
      }
    }
    else if (insideButton(x, y, w, h, sizeDropBox))
    {
      sizeDropdownOpen = !sizeDropdownOpen;
      dropdownOpen = false;
    }
    else if (sizeDropdownOpen)
    {
      float xN = (float)x / w * 2.0f - 1.0f;
      float yN = 1.0f - (float)y / h * 2.0f;
      float itemHeight = sizeDropBox.h;
      float spacing = 0.015f;
      for (int i = 0; i < numSizes; i++)
      {
        float yItem = sizeDropBox.y - (i + 1) * itemHeight - i * spacing;
        if (xN >= sizeDropBox.x && xN <= sizeDropBox.x + sizeDropBox.w &&
            yN >= yItem && yN <= yItem + itemHeight)
        {
          lattice_size = sizes[i];
          sizeIndex = i;
          sizeDropdownOpen = false;
          break;
        }
      }
    }
    else if (insideButton(x, y, w, h, dropBox) && !useStandard)
    {
      dropdownOpen = !dropdownOpen;
      sizeDropdownOpen = false;
    }
    else if (dropdownOpen && !useStandard)
    {
      float xN = (float)x / w * 2.0f - 1.0f;
      float yN = 1.0f - (float)y / h * 2.0f;
      float itemHeight = dropBox.h;
      float spacing = 0.015f;
      for (int i = 2; i <= maxLayers; i++)
      {
        float yItem = dropBox.y - (i - 1) * itemHeight - (i - 2) * spacing;
        if (xN >= dropBox.x && xN <= dropBox.x + dropBox.w &&
            yN >= yItem && yN <= yItem + itemHeight)
        {
          numLayers = i;
          dropdownOpen = false;
          break;
        }
      }
    }
    else {
      sizeDropdownOpen = false;
      dropdownOpen = false;
    }
  }
}

// ----------------------------------------------------------
// Keyboard handling
void keyboard(unsigned char key, int, int)
{
  if (key == 13) { selection = 1; shouldExit = true; }
}

// ----------------------------------------------------------
// Reshape callback
void reshape(int w, int h)
{
  static bool resizing = false;
  if (resizing) return;
  if (w != WINDOW_WIDTH || h != WINDOW_HEIGHT)
  {
    resizing = true;
    glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
    resizing = false;
  }
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

// ----------------------------------------------------------
// Idle callback
void idle()
{
  if (shouldExit)
  {
    glutIdleFunc(NULL);
    glutLeaveMainLoop();
    return;
  }
  glutPostRedisplay();
}

// ----------------------------------------------------------
// Close callback
void closeFunc() { shouldExit = true; }

// ----------------------------------------------------------
// Main function
int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  glutInitWindowPosition((screenWidth - WINDOW_WIDTH) / 2, (screenHeight - WINDOW_HEIGHT) / 2);
  int window = glutCreateWindow("Toy Universe");

  glutReshapeFunc(reshape);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (!loadLogo("logo.png")) std::cerr << "Warning: Could not load logo.png\n";

  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  glutCloseFunc(closeFunc);

  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutMainLoop();

  if (glutGetWindow() > 0) glutDestroyWindow(window);

  if (selection == 1)
  {
    glfwInit();
    int r = runSimulation(lattice_size, numLayers);
    glfwTerminate();
    return r;
  }
  if (selection == 2)
    return runStatistics(lattice_size, numLayers);

  return 0;
}
