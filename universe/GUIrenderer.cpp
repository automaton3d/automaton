/*
 * mygl.cpp
 *
 * Implements the OpenGL rendering routines.
 */

#include "GUIrenderer.h"

#include "model/entropy.h"
#include "GLutils.h"
#include "layers.h"
#include "slider.h"

namespace automaton
{
  extern EntropyCalculator entropyCalc;
}

namespace framework
{
  using namespace std;

  extern unsigned long long timer;
  extern bool pause;
  extern void setOrthographicProjection();
  extern void resetPerspectiveProjection();

  vector<Tickbox> checkboxes;
  vector<Radio> dataset;
  vector<Radio> viewpoint;
  LayerList list;

  VerticalSlider slider(1886, 93, 20.0f, 607.0f, 30.0f);

  random_device rd;
  mt19937 gen(rd());

  bool entropyFlag;
  unsigned long tbegin;
  int barWidths[5];
  bool poincare = false;
  unsigned currentLayer = 0;

  string help[10] =
  {
    "           c: Print camera Eye, Center, Up",
    "           r: Reset view",
    "           t: Toggle right button to do Pan or First-Person",
    "     x, y, z: Snap camera to axis",
    "   Hold Ctrl: Increase speed",
    "  Hold Shift: Reduce speed",
    "  Left-Click: Rotate",
    "Middle-Click: Pan or First-Person",
    " Right-Click: Roll",
    "Scroll-Wheel: Dolly (zoom)"
  };

  string steps[5] =
  {
	"Convolution",
	"Collision",
	"Diffusion",
	"Relocation",
	"Light"
  };

  unsigned lastPos[W_DIM][3];

  using namespace automaton;

  const float GRID_SIZE = 0.5 / SIDE;

  /**
   * Default constructor.
   */
  GUIrenderer::GUIrenderer() : Renderer()
  {
  }

  /**
   * Destructor
   */
  GUIrenderer::~GUIrenderer()
  {
  }

  /**
   * Initializes the GUI.
   */
  void GUIrenderer::init()
  {
    voxels = (COLORREF*) malloc(SIDE3 * sizeof(COLORREF));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    tbegin = GetTickCount64();
    checkboxes.push_back(Tickbox(50, 80, "Wavefront"));  // 0
    checkboxes.push_back(Tickbox(50, 110, "Momentum"));  // 1
    checkboxes.push_back(Tickbox(50, 140, "Plane"));     // 2
    checkboxes.push_back(Tickbox(50, 170, "Entropy"));   // 3
    checkboxes.push_back(Tickbox(50, 200, "Lattice"));   // 4
    checkboxes.push_back(Tickbox(50, 230, "Axes"));      // 5
    checkboxes.push_back(Tickbox(50, 260, "Particles")); // 6
    checkboxes[0].setState(true);
    checkboxes[3].setState(true);
    checkboxes[5].setState(true);
    checkboxes[6].setState(true);
    dataset.push_back(Radio(60, 330, "Single"));
    dataset.push_back(Radio(60, 360, "Partial"));
    dataset.push_back(Radio(60, 390, "Full"));
    dataset.push_back(Radio(60, 420, "Random"));
    dataset[3].setSelected(true);
    viewpoint.push_back(Radio(60, 490, "Isometric"));
    viewpoint.push_back(Radio(60, 520, "XY"));
    viewpoint.push_back(Radio(60, 550, "YZ"));
    viewpoint.push_back(Radio(60, 580, "ZX"));
    viewpoint.push_back(Radio(60, 610, "Reset view"));
    viewpoint[0].setSelected(true);
    // Initialize entropy
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    // Initialize progress bar data
    int barWidth = viewport[2] / 4; // Bar is 1/4 of the screen width
    double totalRatio = (double) FRAME;
    barWidths[0] = (int)(barWidth * (double)CONVOL / totalRatio);           // CONVOL
    barWidths[1] = 1;                          								// COLLISION
    barWidths[2] = (int)(barWidth * (double)(W_DIM - 1) / totalRatio);      // DIFFUSION
    barWidths[3] = (int)(barWidth * (double)(4 * (SIDE - 1)) / totalRatio); // RELOCATION
    barWidths[4] = (int)(barWidth * (double)LIGHT / totalRatio);            // LIGHT
  }

  /**
   * Renders the GUI.
   */
  void GUIrenderer::render()
  {
    renderClear();
    renderObjects();
    renderTextObjects();
    if (entropyFlag)
      renderEntropy();
  }

  /**
   * Clear the GUI.
   */
  void GUIrenderer::renderClear()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
  }

  void GUIrenderer::renderList()
  {

  }

  /*
   * Renders a progress bar showing a complete light step.
   */
  void GUIrenderer::renderProgressBar()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int screenWidth = viewport[2];
    // Progress bar dimensions
    int barWidth = screenWidth / 4;
    int barHeight = 20; // Fixed height
    int barX = (screenWidth - barWidth) / 2; // Center horizontally
    int barY = 100; // Fixed height from bottom
    // Calculate pointer position based on timer % FRAME
    float progress = static_cast<float>(timer % FRAME) / FRAME;
    int pointerX = barX + static_cast<int>(progress * barWidth);
    // Calculate legend parameters
    int x0 = barX + barWidth + 22;
    int w = 20;
    int h = 5;
    int y0 = barY - 35;
    // Draw the sections of the bar with proportional widths
    int accumulatedWidth = 0;
    for (int i = 0; i < 5; i++)
    {
      // Calculate this section's start and end positions
      int sectionStart = accumulatedWidth;
      int sectionEnd = accumulatedWidth + barWidths[i];
      // Set color for each section
      switch (i)
      {
        case 0: glColor3f(0.3f, 0.3f, 0.0f); break;
        case 1: glColor3f(1.0f, 1.0f, 1.0f); break;
        case 2: glColor3f(0.0f, 0.5f, 0.0f); break;
        case 3: glColor3f(0.0f, 0.2f, 0.7f); break;
        case 4: glColor3f(0.5f, 0.0f, 0.0f); break;
      }
      // Draw the section
      glBegin(GL_QUADS);
        glVertex2i(barX + sectionStart, barY);
        glVertex2i(barX + sectionEnd, barY);
        glVertex2i(barX + sectionEnd, barY + barHeight);
        glVertex2i(barX + sectionStart, barY + barHeight);
      glEnd();
      // Draw legend
      glBegin(GL_TRIANGLES);
        glVertex2f(x0, y0);
    	glVertex2f(x0 + w, y0);
    	glVertex2f(x0 + w, y0 + h);
    	glVertex2f(x0, y0);
    	glVertex2f(x0 + w, y0 + h);
    	glVertex2f(x0, y0 + h);
      glEnd();
      glColor3f(0.6f, 0.6f, 0.6f);
      drawString12(steps[i], x0 + w + 10, y0 + 7);
      // Update the accumulated width
      accumulatedWidth += barWidths[i];
      y0 += 20;
    }
    // Draw the progress bar outline
    glColor3f(0.6f, 0.6f, 0.6f); // White outline
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2i(barX-1, barY);
    glVertex2i(barX-1 + barWidth, barY);
    glVertex2i(barX-1 + barWidth, barY + barHeight);
    glVertex2i(barX-1, barY + barHeight);
    glEnd();
    // Draw the pointer
    glColor3f(1.0f, 1.0f, 0.0f); // Red pointer
    glBegin(GL_QUADS);
    glVertex2i(pointerX - 2, barY+2);          // Pointer width: 10
    glVertex2i(pointerX + 2, barY+2);
    glVertex2i(pointerX + 2, barY + barHeight - 2);
    glVertex2i(pointerX - 2, barY + barHeight - 2);
    glEnd();
  }

  /**
   * Opens a pause message box.
   */
  /**
   * Opens a pause message box with centered text and an outline.
   */
  void GUIrenderer::renderCenterBox(const char* text)
  {
      GLint viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);

      int viewportWidth = viewport[2];
      int viewportHeight = viewport[3];

      // Calculate text dimensions dynamically
      int textWidth = strlen(text) * 10;
      int textHeight = 10;

      // Center the text in the viewport
      int textX = (viewportWidth - textWidth) / 2;
      int textY = (viewportHeight + textHeight) / 2;

      // Padding for the rectangle
      int paddingX = 15;
      int paddingY = 10;

      // Rectangle dimensions and position
      int rectX = textX - paddingX;
      int rectY = textY - textHeight - paddingY;
      int rectWidth = textWidth + 2 * paddingX;
      int rectHeight = textHeight + 2 * paddingY;

      // Draw the text
      glColor3f(1.0f, 1.0f, 1.0f); // White color for text
      drawString8(text, textX, textY); // Render the text at the calculated position

      // Draw the rectangle around the text
      glColor3f(1.0f, 1.0f, 1.0f); // White color for rectangle
      glLineWidth(2);
      glBegin(GL_LINE_LOOP);
      glVertex2i(rectX, rectY);
      glVertex2i(rectX + rectWidth, rectY);
      glVertex2i(rectX + rectWidth, rectY + rectHeight);
      glVertex2i(rectX, rectY + rectHeight);
      glEnd();
  }

  /**
   * Renders 2D text objects.
   */
  void GUIrenderer::renderTextObjects()
  {
  setOrthographicProjection();
  glPushMatrix();
  glLoadIdentity();
  glColor3f(1.0f, 1.0f, 1.0f);
  unsigned long millis = GetTickCount64() - tbegin;
  char s[100];
  sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
  drawString8(s, 50, 40);
  sprintf(s, "Light: %llu tick: %llu", timer / automaton::FRAME, timer);
  render2Dstring(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);

  sprintf(s, "SIDE %u", SIDE);
  render2Dstring(1750, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  //
  if (poincare)
  {
    sprintf(s, "POINCARE: %llu", timer);
    render2Dstring(300, 400, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }
  // Get the primary monitor
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  // Get the video mode of the monitor
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  // Draw the help text
  for(int i = 0; i < 10; i++)
    drawString8(help[i], mode->width - 500, 20 * i + mode->height - 260);
  // Add the progress bar rendering
  renderProgressBar();
  // Add a vertical slider
  slider.draw();
  // Handle pause window
  if (pause)
    renderCenterBox(" Paused ");
  //
  glPopMatrix();
  list.update();
  resetPerspectiveProjection();
}

/**
 * Renders the dynamic entropy curve.
 */
void GUIrenderer::renderEntropy()
{
  // Set up orthographic projection for 2D rendering
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int screenWidth = viewport[2];
  int screenHeight = viewport[3];
  // Define graph dimensions relative to screen size
  int graphWidth = screenWidth / 4;  // 25% of the screen width
  int graphHeight = screenHeight / 4; // 25% of the screen height
  int graphX = 60;  // Margin from the left
  int graphY = screenHeight - graphHeight - 770; // Margin from the top
  // Set orthographic projection for 2D graph
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  // Draw graph background
  glColor3f(0.2f, 0.2f, 0.2f); // Dark grey background
  glBegin(GL_QUADS);
  glVertex2i(graphX, graphY);
  glVertex2i(graphX + graphWidth, graphY);
  glVertex2i(graphX + graphWidth, graphY + graphHeight);
  glVertex2i(graphX, graphY + graphHeight);
  glEnd();
  // Draw axes
  glColor3f(1.0f, 1.0f, 1.0f); // White axes
  glBegin(GL_LINES);
  // Horizontal axis
  glVertex2i(graphX, graphY);
  glVertex2i(graphX + graphWidth, graphY);
  // Vertical axis
  glVertex2i(graphX, graphY);
  glVertex2i(graphX, graphY + graphHeight);
  glEnd();
  // Draw axis ticks and labels
  int numTicks = 5;
  glColor3f(1.0f, 1.0f, 1.0f); // White for text and ticks
  Entropy entropy = entropyCalc.getEntropy();
  float minEntropy = entropy.getMinEntropy();
  float maxEntropy = entropy.getMaxEntropy();
  float entropyStep = (maxEntropy - minEntropy) / numTicks;
  for (int i = 0; i <= numTicks; ++i)
  {
    // Horizontal ticks (percentages from 0 to 100)
    int x = graphX + (i * graphWidth / numTicks);
    glBegin(GL_LINES);
    glVertex2i(x, graphY);
    glVertex2i(x, graphY - 5); // Tick length = 5
    glEnd();
    // Label for horizontal axis as percentage
    int percent = i * 100 / numTicks;  // Convert tick index to percentage
    drawString12(std::to_string(percent) + "%", x - 5, graphY - 15); // Adjust x offset for centering text
    // Vertical ticks (entropy)
    float currentEntropy = minEntropy + i * entropyStep;
    int y = graphY + static_cast<int>((i * graphHeight) / numTicks);
    glBegin(GL_LINES);
    glVertex2i(graphX, y);
    glVertex2i(graphX - 5, y); // Tick length = 5
    glEnd();
    // Draw entropy value as the label
    std::string entropyLabel = std::to_string(currentEntropy);
    entropyLabel = entropyLabel.substr(0, entropyLabel.find('.') + 3); // Limit to two decimal places
    drawString12(entropyLabel, graphX - 40, y - 5); // Adjust positioning for centering text
  }
  // Draw entropy function
  glColor3f(1.0f, 0.0f, 0.0f); // Red entropy function
  glBegin(GL_LINE_STRIP);
  for (int x = 0; x < graphWidth; ++x)
  {
    // Normalize x to range [-1, 1]
    float y = entropy.getY(x);
    if (y == 0)
      break;
    float normalizedY = (y - entropy.getMinEntropy()) / (entropy.getMaxEntropy() - entropy.getMinEntropy()) * graphHeight;
    normalizedY = max(0.0f, min(normalizedY, static_cast<float>(graphHeight)));
    glVertex2i(graphX + x, graphY + static_cast<int>(normalizedY));
  }
  glEnd();
  // Draw vertical needle at pointer position
  glColor3f(0.4f, 0.4f, 0.4f); // Green needle
  glBegin(GL_LINES);
  glVertex2i(graphX + entropy.getPointer(), graphY);
  glVertex2i(graphX + entropy.getPointer(), graphY + graphHeight);
  glEnd();
  // Draw axis labels
  glColor3f(0.0f, 1.0f, 0.0f); // Green labels
  drawBoldText("t", graphX + graphWidth / 2 - 10, graphY - 30); // Horizontal axis label
  drawBoldText("H", graphX - 40, graphY + graphHeight / 2);  // Vertical axis label
  // Restore previous matrices
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

/**
 * Draws the points representing the active wavefront cells of
 * the current layer.
 */
void GUIrenderer::renderWavefront()
{
  // Cell spacing.
  const float GRID_SIZE =  0.5 / SIDE;
  // Size of each lattice point.
  glPointSize(2.0f);
  glBegin(GL_POINTS);
  for (int x = 0; x < SIDE; x++)
  {
    for (int y = 0; y < SIDE; y++)
    {
      for (int z = 0; z < SIDE; z++)
      {
        COLORREF color = automaton::voxels[x*SIDE2 + y*SIDE + z];
        if (!color)
          continue;
        // Extrair os componentes R, G, B
        BYTE r = GetRValue(color);
        BYTE g = GetGValue(color);
        BYTE b = GetBValue(color);
        // Converter para valores normalizados entre 0.0 e 1.0
        GLdouble red   = r / 255.0;
        GLdouble green = g / 255.0;
        GLdouble blue  = b / 255.0;
        float alpha = 0.5;
        // Definir a cor no OpenGL
        glColor4d(red, green, blue, alpha);
        float px = (x - SIDE / 2) * GRID_SIZE;
        float py = (y - SIDE / 2) * GRID_SIZE;
        float pz = (z - SIDE / 2) * GRID_SIZE;
        glVertex3f(px, py, pz);
        // DEBUG
        glColor3d(1, 1, 0);
        if (lattice_curr[x][y][z][0].c[0] || lattice_curr[x][y][z][0].c[1] || lattice_curr[x][y][z][0].c[2])
            glVertex3f(px+1, py+1, pz+1);
      }
    }
  }
  glEnd();
  enhanceVoxel();
}

/*
 * Render the center of the bubbles only.
 * All layers (w dimension) contribute.
 */
void GUIrenderer::renderParticles()
{
  // Cell spacing.
  const float GRID_SIZE = 0.5 / SIDE;
  // Size of each lattice point.
  glPointSize(8.0f);
  glBegin(GL_POINTS);
  // Calculate the index for the center element
  for (unsigned w = 0; w < W_DIM; w++)
  {
    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
    float alpha = 0.5;
    // Set the color in OpenGL
    float r = 0.7 + (w & 1)*0.3;
    float g = 0.7 + ((w >> 1) & 1)*0.3;
    float b = 0.7 + ((w >> 2) & 1)*0.3;
    //
    float px = (SIDE - cell.pos[0] - 0.5f) * GRID_SIZE - 0.25f;
    float py = (SIDE - cell.pos[1] - 0.5f) * GRID_SIZE - 0.25f;
    float pz = (SIDE - cell.pos[2] - 0.5f) * GRID_SIZE - 0.25f;
    glColor4d(r, g, b, alpha);
    glVertex3f(px, py, pz);
  }
  glEnd();
  enhanceVoxel();
}

/*
 * Renders 2D active controls of the GUI.
 */
void GUIrenderer::renderGadgets()
{
  glDisable(GL_DEPTH_TEST);
  setOrthographicProjection();
  glPointSize(1);
  glPushMatrix();
  for (Tickbox& checkbox : checkboxes)
  {
    checkbox.draw();
  }
  list.render();
  /*
  for (Radio& layer : layers)
  {
    layer.draw();
  }
  */
  for (Radio& radio : dataset)
  {
    radio.draw();
  }
  for (Radio& radio : viewpoint)
  {
    radio.draw();
  }
  glFlush();
  glPopMatrix();
  resetPerspectiveProjection();
}

/*
 * Renders 2D and 3D objects controlled by the mouse and keyboard.
 */
void GUIrenderer::renderObjects()
{
  glEnable(GL_DEPTH_TEST);
  glPushMatrix();
  glMultMatrixf(glm::value_ptr(mProjection));
  if (mCamera)
  {
    glPushMatrix();
    glMultMatrixf(mCamera->getMatrixFlat());
  }
  // Render the grid
  if (checkboxes[2].getState())
    renderPlane();
  // Render the axes
  if (checkboxes[5].getState())
    renderAxes();
  // TODO ???
  entropyFlag = checkboxes[3].getState();
  // Render the lattice outline
  if (checkboxes[4].getState())
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode.
    renderCube();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore fill mode.
  }
  // Render the particles (centers of bubbles)
  if (checkboxes[6].getState())
    renderParticles();
  // Render the current layer wavefront
  if (checkboxes[0].getState() || checkboxes[1].getState())
    renderWavefront();
  if (mCamera)
  {
    glPopMatrix();
  }
  glPopMatrix();
  // Render 2D active objects
  renderGadgets();
}

/*
 * Renders the cartesian positive axes with labels.
 */
void GUIrenderer::renderAxes()
{
    // Render the axes
    glLineWidth(2);
    glBegin(GL_LINES);

    // X-axis
    glColor3f(0.6f, 0.f, 0.f); // Red for the X-axis
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(0.5f, 0.f, 0.f);

    // Y-axis
    glColor3f(0.f, 0.6f, 0.f); // Green for the Y-axis
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 0.5f, 0.f);

    // Z-axis
    glColor3f(0.f, 0.f, 0.8f); // Blue for the Z-axis
    glVertex3f(0.0f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, 0.5f);

    glEnd();
    glLineWidth(1);
    // Add labels
    glColor3f(1.f, 0.f, 0.f); // Red for "x" label
    render3Dstring(0.512f, -0.02f, 0.0f, GLUT_BITMAP_HELVETICA_18, "x");
    glColor3f(0.f, 1.f, 0.f); // Green for "y" label
    render3Dstring(-0.02f, 0.512f, 0.0f, GLUT_BITMAP_HELVETICA_18, "y");
    glColor3f(0.1f, 0.1f, 1.f); // Blue for "z" label
    render3Dstring(0.0f, -0.02f, 0.512f, GLUT_BITMAP_HELVETICA_18, "z");
}

/*
 * Renders the 3D space outline.
 */
void GUIrenderer::renderCube()
{
  GLfloat alpha = 0.6f;
  glBegin(GL_QUADS);
  glColor4f(0.45f, 0.13f, 0.13f, alpha);
  // Right face
  glVertex3f( 0.25f, -0.25f, -0.25f);
  glVertex3f( 0.25f,  0.25f, -0.25f);
  glVertex3f( 0.25f,  0.25f,  0.25f);
  glVertex3f( 0.25f, -0.25f,  0.25f);
  // Left face
  glVertex3f(-0.25f, -0.25f, -0.25f);
  glVertex3f(-0.25f,  0.25f, -0.25f);
  glVertex3f(-0.25f,  0.25f,  0.25f);
  glVertex3f(-0.25f, -0.25f,  0.25f);
  // Top face
  glVertex3f(-0.25f,  0.25f, -0.25f);
  glVertex3f( 0.25f,  0.25f, -0.25f);
  glVertex3f( 0.25f,  0.25f,  0.25f);
  glVertex3f(-0.25f,  0.25f,  0.25f);
  // Bottom face
  glVertex3f(-0.25f, -0.25f, -0.25f);
  glVertex3f( 0.25f, -0.25f, -0.25f);
  glVertex3f( 0.25f, -0.25f,  0.25f);
  glVertex3f(-0.25f, -0.25f,  0.25f);
  // Front face
  glVertex3f(-0.25f, -0.25f,  0.25f);
  glVertex3f( 0.25f, -0.25f,  0.25f);
  glVertex3f( 0.25f,  0.25f,  0.25f);
  glVertex3f(-0.25f,  0.25f,  0.25f);
  // Back face
  glVertex3f(-0.25f, -0.25f, -0.25f);
  glVertex3f( 0.25f, -0.25f, -0.25f);
  glVertex3f( 0.25f,  0.25f, -0.25f);
  glVertex3f(-0.25f,  0.25f, -0.25f);
  glEnd();
}

  /*
   * Renders a reference plane to help in the visualization.
   */
  void GUIrenderer::renderPlane()
  {
    float p, d = .1, mn = -1.f, mx = 1.f, eps = -1e-4;
    int i, n = 20;
    #define WHITE() glColor4f(1.f, 1.f, 1.f, .2f)
    glLineWidth(1);
    glBegin(GL_LINES);
    for (i = 0; i <= n; ++i)
    {
      p = mn + i * d;
      // Draw lines parallel to the x-axis (constant y, varying z)
      if (i == 0 || i == 10 || i == n)
      {
        glColor4f(0.f, 1.f, 0.f, .3f); // Special lines for center and boundaries
      }
      else
      {
        WHITE();
      }
      glVertex3f(p, eps, mn);  // Use 'p' for x, 'mn' for z, keep y = eps (constant height)
      glVertex3f(p, eps, mx);  // Same, but z = mx for the second point
      // Draw lines parallel to the z-axis (constant x, varying z)
      if (i == 0 || i == 10 || i == n)
      {
        glColor4f(1.f, 0.f, 0.f, .3f); // Special lines for center and boundaries
      }
      else
      {
        WHITE();
      }
      glVertex3f(mn, eps, p);  // Use 'p' for z, 'mn' for x, y = eps (constant height)
      glVertex3f(mx, eps, p);  // Same, but x = mx for the second point
    }
    glEnd();
    #undef WHITE
  }

  /*
   * Resizes the GUI.
   */
  void GUIrenderer::resize(int width, int height)
  {
    if (0 == height)
    {
      height = 1; // Avoid division by zero.
    }
    GLfloat ratio = width / (GLfloat) height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    mProjection = glm::perspective(glm::radians(45.0f), ratio, .01f, 100.f);
  }

  /*
   * Used to enhance the particle belonging the current layer.
   */
  void GUIrenderer::enhanceVoxel()
  {
    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][currentLayer];
    float cx = (SIDE - cell.pos[0] - 0.5f) * GRID_SIZE - 0.25f;
    float cy = (SIDE - cell.pos[1] - 0.5f) * GRID_SIZE - 0.25f;
    float cz = (SIDE - cell.pos[2] - 0.5f) * GRID_SIZE - 0.25f;
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor3d(0.7, 0.7, 0.7);

    // Generate a random pattern around the center
    const int POINT_COUNT = 12; // Number of random points
    const float MAX_OFFSET = 0.005f; // Maximum offset for random points
    for (int i = 0; i < POINT_COUNT; ++i)
    {
        float dx = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
        float dy = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
        float dz = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;

        glVertex3f(cx + dx, cy + dy, cz + dz);
    }
    glEnd();
  }

}
