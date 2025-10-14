/*
 * splash.cpp
 */
#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>
#include <shellapi.h>
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
Button helpLink = { -0.15f, -0.90f, 0.3f, 0.05f, "Help" }; // Hyperlink button, centered

// ----------------------------------------------------------
// Size selection controls
const int sizes[] = {3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31};
const int numSizes = sizeof(sizes) / sizeof(sizes[0]);
int lattice_size = 11;
int sizeIndex = 4;
bool sizeDropdownOpen = false;
Button sizeDropBox = { -0.85f, -0.25f, 0.4f, 0.08f, "11" };
// New scroll variables for Size Dropdown
float sizeScrollOffset = 0.0f; // Stores the vertical scroll offset (0.0 means no scroll)
const float MAX_DROPDOWN_HEIGHT = 0.5f; // Max visual height of the dropdown list

// ----------------------------------------------------------
// Layer selection controls - even numbers 2 to 364
const int layers[] = {
  2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
  40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78,
  80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118,
  120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
  160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198,
  200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238,
  240, 242, 244, 246, 248, 250, 252, 254, 256, 258, 260, 262, 264, 266, 268, 270, 272, 274, 276, 278,
  280, 282, 284, 286, 288, 290, 292, 294, 296, 298, 300, 302, 304, 306, 308, 310, 312, 314, 316, 318,
  320, 322, 324, 326, 328, 330, 332, 334, 336, 338, 340, 342, 344, 346, 348, 350, 352, 354, 356, 358,
  360, 362, 364
};
const int numLayersAvailable = sizeof(layers) / sizeof(layers[0]);
int numLayers = 4;
const int maxLayers = 364;
bool useStandard = false;
bool dropdownOpen = false;
Button dropBox = { -0.85f, -0.40f, 0.4f, 0.08f, "4" };
Button chkStandard = { 0.15f, -0.40f, 0.08f, 0.08f, "" };

// New scroll variables for Layers Dropdown
float layersScrollOffset = 0.0f;
const float MAX_LAYERS_DROPDOWN_HEIGHT = MAX_DROPDOWN_HEIGHT; // Use same height as Size

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
// Draw hyperlink text
void drawHyperlink(const Button& b)
{
  glColor3f(0.0f, 0.0f, 1.0f);  // Blue text
  const char* c = b.label;
  int textWidth = 0;
  while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c++);
  float tx = b.x + (b.w - (textWidth * 2.0f / WINDOW_WIDTH)) / 2;  // Center text
  float ty = b.y + 0.015f;
  glRasterPos2f(tx, ty);
  c = b.label;
  while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c++);

  // Draw underline
  glColor3f(0.0f, 0.0f, 1.0f);
  glLineWidth(1.0f);
  glBegin(GL_LINES);
    glVertex2f(b.x + (b.w - (textWidth * 2.0f / WINDOW_WIDTH)) / 2, b.y + 0.005f);
    glVertex2f(b.x + (b.w + (textWidth * 2.0f / WINDOW_WIDTH)) / 2, b.y + 0.005f);
  glEnd();
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
// Expanded size dropdown - NOW WITH CLIPPING AND SCROLLING
void drawExpandedSizeDropdown(const Button& b)
{
  float itemHeight = b.h;
  float spacing = 0.015f;

  // --- Scroll calculation and Scissoring setup ---
  float totalListHeight = numSizes * itemHeight + (numSizes - 1) * spacing;
  float visibleHeight = totalListHeight > MAX_DROPDOWN_HEIGHT ? MAX_DROPDOWN_HEIGHT : totalListHeight;
  float maxScrollOffset = totalListHeight > MAX_DROPDOWN_HEIGHT ? totalListHeight - MAX_DROPDOWN_HEIGHT : 0.0f;

  if (sizeScrollOffset < 0.0f) sizeScrollOffset = 0.0f;
  if (sizeScrollOffset > maxScrollOffset) sizeScrollOffset = maxScrollOffset;
  // --- End Scroll Setup ---

  // Calculate the y-positions for visible items
  std::vector<float> itemYPositions;
  for (int i = 0; i < numSizes; i++)
  {
    // Apply scroll offset to the base position
    float yItem = b.y - (i + 1) * itemHeight - i * spacing + sizeScrollOffset;
    itemYPositions.push_back(yItem);
  }

  if (itemYPositions.empty()) return;

  // --- Define Clipping Area ---
  float borderTop = b.y - 0.005f;
  float borderBottom = borderTop - visibleHeight - 0.01f; // Account for small border padding
  float borderLeft = b.x - 0.015f;
  float borderRight = b.x + b.w + 0.015f;

  // --- Set Scissor Test to clip the list ---
  glEnable(GL_SCISSOR_TEST);
  // Convert normalized coordinates [-1, 1] to screen coordinates [0, w]
  int scissorX = (int)((borderLeft + 1.0f) * WINDOW_WIDTH / 2.0f);
  int scissorY = (int)((borderBottom + 1.0f) * WINDOW_HEIGHT / 2.0f); // BOTTOM Y
  int scissorW = (int)((borderRight - borderLeft) * WINDOW_WIDTH / 2.0f);
  int scissorH = (int)((borderTop - borderBottom) * WINDOW_HEIGHT / 2.0f); // HEIGHT
  glScissor(scissorX, scissorY, scissorW, scissorH);
  // --- End Scissor Setup ---

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Background box (Drawn large enough to cover the total list height, clipped by glScissor)
  glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(borderLeft, b.y - totalListHeight - spacing);
    glVertex2f(borderRight, b.y - totalListHeight - spacing);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Draw list items
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
  // --- FIX: Disable Scissor Test before drawing the border ---
  glDisable(GL_SCISSOR_TEST);

  // Draw the border *after* disabling Scissor Test to ensure it's not clipped
  glColor3f(0.2f, 0.2f, 0.2f);
  glLineWidth(2.5f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();
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
// Expanded layers dropdown - NOW WITH CLIPPING AND SCROLLING
void drawExpandedLayersDropdown(const Button& b)
{
  float itemHeight = b.h;
  float spacing = 0.015f;

  // --- Scroll calculation and Scissoring setup ---
  float totalListHeight = numLayersAvailable * itemHeight + (numLayersAvailable - 1) * spacing;
  float visibleHeight = totalListHeight > MAX_LAYERS_DROPDOWN_HEIGHT ? MAX_LAYERS_DROPDOWN_HEIGHT : totalListHeight;
  float maxScrollOffset = totalListHeight > MAX_LAYERS_DROPDOWN_HEIGHT ? totalListHeight - MAX_LAYERS_DROPDOWN_HEIGHT : 0.0f;

  if (layersScrollOffset < 0.0f) layersScrollOffset = 0.0f;
  if (layersScrollOffset > maxScrollOffset) layersScrollOffset = maxScrollOffset;
  // --- End Scroll Setup ---

  // Calculate the y-positions for all items, applying scroll offset
  std::vector<float> itemYPositions;
  for (int i = 0; i < numLayersAvailable; i++)
  {
    // Apply scroll offset to the base position
    float yItem = b.y - (i + 1) * itemHeight - i * spacing + layersScrollOffset;
    itemYPositions.push_back(yItem);
  }

  if (itemYPositions.empty()) return;

  // --- Define Clipping Area ---
  float borderTop = b.y - 0.005f;
  float borderBottom = borderTop - visibleHeight - 0.01f;
  float borderLeft = b.x - 0.015f;
  float borderRight = b.x + b.w + 0.015f;

  // --- Set Scissor Test to clip the list ---
  glEnable(GL_SCISSOR_TEST);
  int scissorX = (int)((borderLeft + 1.0f) * WINDOW_WIDTH / 2.0f);
  int scissorY = (int)((borderBottom + 1.0f) * WINDOW_HEIGHT / 2.0f);
  int scissorW = (int)((borderRight - borderLeft) * WINDOW_WIDTH / 2.0f);
  int scissorH = (int)((borderTop - borderBottom) * WINDOW_HEIGHT / 2.0f);
  glScissor(scissorX, scissorY, scissorW, scissorH);
  // --- End Scissor Setup ---

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Background box (Clipped by glScissor)
  glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
  glBegin(GL_QUADS);
    glVertex2f(borderLeft, b.y - totalListHeight - spacing);
    glVertex2f(borderRight, b.y - totalListHeight - spacing);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();

  // Draw list items
  for (size_t idx = 0; idx < itemYPositions.size(); idx++)
  {
    float yItem = itemYPositions[idx];
    int layerValue = layers[idx];
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
  glDisable(GL_SCISSOR_TEST);

  // Draw the border *after* disabling Scissor Test
  glColor3f(0.2f, 0.2f, 0.2f);
  glLineWidth(2.5f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(borderLeft, borderBottom);
    glVertex2f(borderRight, borderBottom);
    glVertex2f(borderRight, borderTop);
    glVertex2f(borderLeft, borderTop);
  glEnd();
}

// ----------------------------------------------------------
// Draw controls (main boxes only)
void drawControls()
{
  glColor3f(0, 0, 0);
  glRasterPos2f(-0.85f, -0.15f);
  const char* sizeLbl = "Size:";
  while (*sizeLbl) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *sizeLbl++);
  drawSizeDropdown(sizeDropBox);

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

  drawTitle();
  drawLogo();
  drawControls();
  drawButton(btn1, true);
  drawButton(btn2, false);
  drawHyperlink(helpLink);  // Draw hyperlink last

  // Draw expanded dropdowns last so they are on top of other elements
  if (dropdownOpen && !useStandard)
  {
    drawExpandedLayersDropdown(dropBox);
  }

  // Size dropdown is drawn after layers dropdown to ensure it's on top if they overlap
  if (sizeDropdownOpen)
  {
    drawExpandedSizeDropdown(sizeDropBox);
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
    else if (insideButton(x, y, w, h, helpLink)) {
      ShellExecuteA(NULL, "open", "https://github.com/automaton3d/automaton/blob/master/help.md", NULL, NULL, SW_SHOWNORMAL);
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
    // --- Size Dropdown Selection/Out-of-Bounds Logic (UPDATED) ---
    else if (sizeDropdownOpen)
    {
      float xN = (float)x / w * 2.0f - 1.0f;
      float yN = 1.0f - (float)y / h * 2.0f;
      float itemHeight = sizeDropBox.h;
      float spacing = 0.015f;

      // Calculate total height and max scroll
      float totalListHeight = numSizes * itemHeight + (numSizes - 1) * spacing;
      float maxScrollOffset = totalListHeight > MAX_DROPDOWN_HEIGHT ? totalListHeight - MAX_DROPDOWN_HEIGHT : 0.0f;

      // Check if click is outside the visible dropdown area (using MAX_DROPDOWN_HEIGHT for clipping)
      if (xN < sizeDropBox.x - 0.015f || xN > sizeDropBox.x + sizeDropBox.w + 0.015f ||
              yN > sizeDropBox.y + 0.005f || yN < sizeDropBox.y - MAX_DROPDOWN_HEIGHT - 0.015f)
      {
        sizeDropdownOpen = false; // Clicked outside the bounds
        return; // CORRECTED: Use return to exit the function, resolving the 'break' error
      }

      // Iterate all items, applying scroll offset to their position
      for (int i = 0; i < numSizes; i++)
      {
        float yItemUnscrolled = sizeDropBox.y - (i + 1) * itemHeight - i * spacing;
        float yItem = yItemUnscrolled + sizeScrollOffset;
        if (xN >= sizeDropBox.x && xN <= sizeDropBox.x + sizeDropBox.w &&
            yN >= yItem && yN <= yItem + itemHeight)
        {
          lattice_size = sizes[i];
          sizeIndex = i;
          sizeDropdownOpen = false;
          // Reset scroll offset on selection
          sizeScrollOffset = 0.0f;
          break;
        }
      }
    }
    // --- Layers Dropdown Activation ---
    else if (insideButton(x, y, w, h, dropBox) && !useStandard)
    {
      dropdownOpen = !dropdownOpen;
      sizeDropdownOpen = false;
    }
    // --- Layers Dropdown Selection/Out-of-Bounds Logic (NEW SCROLL LOGIC) ---
    else if (dropdownOpen && !useStandard)
    {
      float xN = (float)x / w * 2.0f - 1.0f;
      float yN = 1.0f - (float)y / h * 2.0f;
      float itemHeight = dropBox.h;
      float spacing = 0.015f;

      // Calculate total height and max scroll for Layers
      float totalListHeight = numLayersAvailable * itemHeight + (numLayersAvailable - 1) * spacing;
      float maxScrollOffset = totalListHeight > MAX_LAYERS_DROPDOWN_HEIGHT ? totalListHeight - MAX_LAYERS_DROPDOWN_HEIGHT : 0.0f;

      // Check if click is outside the visible dropdown area
      if (xN < dropBox.x - 0.015f || xN > dropBox.x + dropBox.w + 0.015f ||
          yN > dropBox.y + 0.005f || yN < dropBox.y - MAX_LAYERS_DROPDOWN_HEIGHT - 0.015f)
      {
        dropdownOpen = false; // Clicked outside the bounds
        return;
      }

      // Iterate all items, applying scroll offset to their position
      for (int i = 0; i < numLayersAvailable; i++)
      {
        float yItemUnscrolled = dropBox.y - (i + 1) * itemHeight - i * spacing;
        float yItem = yItemUnscrolled + layersScrollOffset;

        if (xN >= dropBox.x && xN <= dropBox.x + dropBox.w &&
            yN >= yItem && yN <= yItem + itemHeight)
        {
          numLayers = layers[i];
          dropdownOpen = false;
          // Reset scroll offset on selection
          layersScrollOffset = 0.0f;
          break;
        }
      }
    }
    // --- Close All ---
    else {
      sizeDropdownOpen = false;
      dropdownOpen = false;
    }
  }
}

// ----------------------------------------------------------
// Mouse wheel handling (Requires FreeGLUT) - UPDATED FOR LAYERS
void mouseWheel(int button, int dir, int x, int y)
{
  int w = WINDOW_WIDTH;
  int h = WINDOW_HEIGHT;
  float scrollSpeed = 0.05f;
  float xN = (float)x / w * 2.0f - 1.0f;
  float yN = 1.0f - (float)y / h * 2.0f;

  if (sizeDropdownOpen)
  {
    // Check if mouse is over the dropdown area
    if (xN >= sizeDropBox.x - 0.015f && xN <= sizeDropBox.x + sizeDropBox.w + 0.015f &&
        yN <= sizeDropBox.y + 0.005f && yN >= sizeDropBox.y - MAX_DROPDOWN_HEIGHT - 0.015f)
    {
      // dir > 0 for scroll up, dir < 0 for scroll down
      sizeScrollOffset += dir * scrollSpeed;
      // Clamp the scroll offset
      float itemHeight = sizeDropBox.h;
      float spacing = 0.015f;
      float totalListHeight = numSizes * itemHeight + (numSizes - 1) * spacing;
      float maxScrollOffset = totalListHeight > MAX_DROPDOWN_HEIGHT ? totalListHeight - MAX_DROPDOWN_HEIGHT : 0.0f;
      if (sizeScrollOffset < 0.0f) sizeScrollOffset = 0.0f;
      if (sizeScrollOffset > maxScrollOffset) sizeScrollOffset = maxScrollOffset;
    }
  }
  // --- Layers Scroll Logic ---
  else if (dropdownOpen && !useStandard)
  {
    // Check if mouse is over the dropdown area
    if (xN >= dropBox.x - 0.015f && xN <= dropBox.x + dropBox.w + 0.015f &&
        yN <= dropBox.y + 0.005f && yN >= dropBox.y - MAX_LAYERS_DROPDOWN_HEIGHT - 0.015f)
    {
      layersScrollOffset += dir * scrollSpeed;
      // Clamp the scroll offset
      float itemHeight = dropBox.h;
      float spacing = 0.015f;
      float totalListHeight = numLayersAvailable * itemHeight + (numLayersAvailable - 1) * spacing;
      float maxScrollOffset = totalListHeight > MAX_LAYERS_DROPDOWN_HEIGHT ? totalListHeight - MAX_LAYERS_DROPDOWN_HEIGHT : 0.0f;
      if (layersScrollOffset < 0.0f) layersScrollOffset = 0.0f;
      if (layersScrollOffset > maxScrollOffset) layersScrollOffset = maxScrollOffset;
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
  // This line is needed for mouse wheel support (FreeGLUT)
  #ifdef __FREEGLUT_EXT_H__
  glutMouseWheelFunc(mouseWheel);
  #endif
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
