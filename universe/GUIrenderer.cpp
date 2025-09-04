/*
 * mygl.cpp
 *
 * Implements the OpenGL rendering routines.
 */

#include "GUIrenderer.h"

#include "GLutils.h"
#include "layers.h"
#include "slider.h"

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
  int barWidths[4];
  unsigned currentLayer = 0;
  // Global flag to control rendering mode: single cube or 27 cubes.
  bool MULTICUBE_MODE = false;

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

  string steps[4] =
  {
	"Convolution",
	"Diffusion",
	"Relocation",
	"Transport"
  };

  unsigned lastPos[W_DIM][3];

  using namespace automaton;

  const float GRID_SIZE = 0.5 / EL;

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
    voxels = (COLORREF*) malloc(L3 * sizeof(COLORREF));
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
    viewpoint[0].setSelected(true);
    // Initialize entropy
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    // Initialize progress bar data
    int barWidth = viewport[2] / 4; // Bar is 1/4 of the screen width
    double totalRatio = (double) FRAME;
    barWidths[0] = (int)(barWidth * (double) CONVOL / totalRatio);
    barWidths[1] = (int)(barWidth * (double) (DIFFUSION - CONVOL) / totalRatio);
    barWidths[2] = (int)(barWidth * (double) (RELOC - DIFFUSION) / totalRatio);
    barWidths[3] = (int)(barWidth * (double) (TRANSP - RELOC) / totalRatio);
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
    for (int i = 0; i < 4; i++)
    {
      // Calculate this section's start and end positions
      int sectionStart = accumulatedWidth;
      int sectionEnd = accumulatedWidth + barWidths[i];
      // Set color for each section
      if (timer % FRAME > 2)
      {
        switch (i)
        {
          case 0: glColor3f(0.3f, 0.3f, 0.0f); break;
          case 1: glColor3f(0.5f, 0.0f, 0.0f); break;
          case 2: glColor3f(0.0f, 0.5f, 0.0f); break;
          case 3: glColor3f(0.0f, 0.2f, 0.7f); break;
        }
      }
      else
      {
        switch (i)
        {
          case 0: glColor3f(0.7f, 0.7f, 0.0f); break;
          case 1: glColor3f(0.7f, 0.0f, 0.0f); break;
          case 2: glColor3f(0.0f, 0.7f, 0.0f); break;
          case 3: glColor3f(0.0f, 0.4f, 0.9f); break;
        }
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
  sprintf(s, "SIDE %u", EL);
  render2Dstring(1750, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  //
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
  // Set orthographic projection for 2D graph
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // TODO DRAW 2D GRAPHICS HERE

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
  const float GRID_SIZE =  0.5 / EL;
  // Size of each lattice point.
  glPointSize(2.0f);

  // Define offsets for 27 cubes (-1, 0, 1 along each axis)
  int offsets[3] = {-1, 0, 1};
  glBegin(GL_POINTS);
  for (int x = 0; x < EL; x++)
  {
    for (int y = 0; y < EL; y++)
    {
      for (int z = 0; z < EL; z++)
      {
        COLORREF color = automaton::voxels[x * L2 + y * EL + z];
        if (!color)
          continue;
        // Extract the R, G, B components.
        BYTE r = GetRValue(color);
        BYTE g = GetGValue(color);
        BYTE b = GetBValue(color);

        // Convert to normalized values between 0.0 and 1.0.
        GLdouble red   = r / 255.0;
        GLdouble green = g / 255.0;
        GLdouble blue  = b / 255.0;
        float alpha = 0.5;
        // Set the OpenGL color.
        glColor4d(red, green, blue, alpha);
        // Base position of the current voxel.
        float px = (x - EL / 2) * GRID_SIZE;
        float py = (y - EL / 2) * GRID_SIZE;
        float pz = (z - EL / 2) * GRID_SIZE;
        if (MULTICUBE_MODE)
        {
          // Render 27 cubes by translating the base grid position.
          for (int dx : offsets)
          {
            for (int dy : offsets)
            {
              for (int dz : offsets)
              {
                glVertex3f(px + dx * 0.5, py + dy * 0.5, pz + dz * 0.5);
              }
            }
          }
        }
        else
        {
          // Render a single cube.
          glVertex3f(px, py, pz);
        }
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
  const float GRID_SIZE = 0.5 / EL;
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
    float px = (EL - cell.x[0] - 0.5f) * GRID_SIZE - 0.25f;
    float py = (EL - cell.x[1] - 0.5f) * GRID_SIZE - 0.25f;
    float pz = (EL - cell.x[2] - 0.5f) * GRID_SIZE - 0.25f;
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
    glColor3f(0.3f, 0.3f, 0.8f); // Blue for the Z-axis
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
    float cx = (EL - cell.x[0] - 0.5f) * GRID_SIZE - 0.25f;
    float cy = (EL - cell.x[1] - 0.5f) * GRID_SIZE - 0.25f;
    float cz = (EL - cell.x[2] - 0.5f) * GRID_SIZE - 0.25f;
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
