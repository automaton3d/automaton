/*
 * mygl.cpp
 *
 * Implements the OpenGL rendering routines.
 */

#include "mygl.h"
#include "model/entropy.h"
#include <string>
#include <GL/glut.h>

namespace automaton
{
	extern Entropy entropy;
}

namespace framework
{
extern unsigned long timer;

std::vector<Tickbox> checkboxes;
std::vector<Radio> layers;
std::vector<Radio> dataset;
std::vector<Radio> viewpoint;

bool entropyFlag;

std::random_device rd;
std::mt19937 gen(rd());

unsigned long tbegin;

int barWidths[4];

std::string help[10] =
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

using namespace automaton;

RendererOpenGL1::RendererOpenGL1() : Renderer()
{
}

RendererOpenGL1::~RendererOpenGL1()
{
}

void RendererOpenGL1::init()
{
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
#ifdef PRODUCTION
  checkboxes[0].setState(true);
  checkboxes[1].setState(true);
  checkboxes[2].setState(true);
  checkboxes[5].setState(true);
#else
  checkboxes[0].setState(true);
  checkboxes[3].setState(true);
  //checkboxes[4].setState(true);
  checkboxes[5].setState(true);
  checkboxes[6].setState(true);
#endif

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
  //
  char s[100];
  for (int i = 0; i < LAYERS; i++)
  {
	  std::sprintf(s, "Layer %d", i);
	  drawString(s, 50, 40);
	  layers.push_back(Radio(1800, 200 + 25 * i, s));
  }
  layers[0].setSelected(true);
  // Initialize entropy
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  entropy.setWidth(viewport[2]/4);
  entropy.setHeight(viewport[3]/4);
  //
  int barWidth = viewport[2] / 4; // Bar is 1/4 of the screen width
  double totalRatio = (double)(CONVOL + SIDE + 3 * CENTER + 3 * SIDE + LIGHT);
  barWidths[0] = (int)(barWidth * (double)CONVOL / totalRatio);
  barWidths[1] = (int)(barWidth * (double)(SIDE + 3 * CENTER) / totalRatio);
  barWidths[2] = (int)(barWidth * (double)(3 * SIDE) / totalRatio);
  barWidths[3] = (int)(barWidth * (double)LIGHT / totalRatio);

}

void RendererOpenGL1::render()
{
  renderClear();
  renderObjects();
  renderText();
  if (entropyFlag)
	  renderEntropy();
}

void RendererOpenGL1::renderCenter()
{
    glPointSize(4);
    glBegin(GL_POINTS);
    if (mCamera)
    {
        const glm::vec3 & p = mCamera->getCenter();
        glColor3f (1.f, 1.f, 0.f);
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
}

void RendererOpenGL1::renderClear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);
}

void setOrthographicProjection()
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport[2], 0, viewport[3], -1, 1);
    glScalef(1, -1, 1);
    glTranslatef(0, -viewport[3], 0);
    glMatrixMode(GL_MODELVIEW);
}

void resetPerspectiveProjection()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void drawString(std::string s, int x, int y)
{
	glRasterPos2f(x, y);
	for (int i = 0; s[i] != '\0'; ++i)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
}

void renderBitmapString(float x, float y, void *font, const char *string)
{
    glRasterPos2f(x, y);  // Set raster position where text will start
    for (int i = 0; i < string[i]; i++)
        glutBitmapCharacter(font, string[i]);
}

void RendererOpenGL1::renderProgressBar()
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

    // Draw the progress bar background
    glColor3f(0.2f, 0.2f, 0.2f); // Dark grey
    glBegin(GL_QUADS);
    glVertex2i(barX, barY);
    glVertex2i(barX + barWidth, barY);
    glVertex2i(barX + barWidth, barY + barHeight);
    glVertex2i(barX, barY + barHeight);
    glEnd();

    // Draw the sections of the bar with proportional widths
    int accumulatedWidth = 0;
    for (int i = 0; i < 4; i++)
    {
        // Calculate this section's start and end positions
        int sectionStart = accumulatedWidth;
        int sectionEnd = accumulatedWidth + barWidths[i];

        // Set color for each section
        switch (i)
        {
        case 0: glColor3f(0.5f, 0.5f, 0.0f); break; // Red
        case 1: glColor3f(0.0f, 0.5f, 0.0f); break; // Green
        case 2: glColor3f(0.0f, 0.0f, 0.5f); break; // Blue
        case 3: glColor3f(0.5f, 0.0f, 0.0f); break; // Yellow
        }

        // Draw the section
        glBegin(GL_QUADS);
        glVertex2i(barX + sectionStart, barY);
        glVertex2i(barX + sectionEnd, barY);
        glVertex2i(barX + sectionEnd, barY + barHeight);
        glVertex2i(barX + sectionStart, barY + barHeight);
        glEnd();

        // Update the accumulated width
        accumulatedWidth += barWidths[i];
    }
    // Draw the progress bar outline
    glColor3f(1.0f, 1.0f, 1.0f); // White outline
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2i(barX, barY);
    glVertex2i(barX + barWidth-1, barY);
    glVertex2i(barX + barWidth-1, barY + barHeight);
    glVertex2i(barX, barY + barHeight);
    glEnd();

    // Draw the pointer
    glColor3f(1.0f, 1.0f, 1.0f); // Red pointer
    glBegin(GL_QUADS);
    glVertex2i(pointerX - 2, barY+1);          // Pointer width: 10
    glVertex2i(pointerX + 2, barY+1);
    glVertex2i(pointerX + 2, barY + barHeight - 1);
    glVertex2i(pointerX - 2, barY + barHeight - 1);
    glEnd();
}

void RendererOpenGL1::renderText()
{
  setOrthographicProjection();
  glPushMatrix();
  glLoadIdentity();
  glColor3f(1.0f, 1.0f, 1.0f);
  unsigned long millis = GetTickCount64() - tbegin;
  char s[100];
  std::sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
  drawString(s, 50, 40);
  std::sprintf(s, "Light: %lu tick: %lu", timer / automaton::FRAME, timer);
  renderBitmapString(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);

  std::sprintf(s, "SIDE %u", SIDE);
  renderBitmapString(1750, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  // Get the primary monitor
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();

  // Get the video mode of the monitor
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  for(int i = 0; i < 10; i++)
	  drawString(help[i], mode->width - 500, 20 * i + mode->height - 260);

  renderProgressBar(); // Add the progress bar rendering


  glPopMatrix();
  resetPerspectiveProjection();
}

void drawText(const std::string& text, int x, int y)
{
    glRasterPos2i(x, y);
    for (char c : text)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c); // Use GLUT for bitmap fonts
    }
}

void RendererOpenGL1::renderEntropy()
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
    for (int i = 0; i <= numTicks; ++i)
    {
        // Horizontal ticks (time)
        int x = graphX + (i * graphWidth / numTicks);
        glBegin(GL_LINES);
        glVertex2i(x, graphY);
        glVertex2i(x, graphY - 5); // Tick length = 5
        glEnd();
        drawText(std::to_string(i), x - 5, graphY - 15); // Adjust x offset for centering text

        // Vertical ticks (entropy)
        int y = graphY + (i * graphHeight / numTicks);
        glBegin(GL_LINES);
        glVertex2i(graphX, y);
        glVertex2i(graphX - 5, y); // Tick length = 5
        glEnd();
        drawText(std::to_string(i), graphX - 20, y - 5); // Adjust y offset for centering text
    }

    // Draw entropy function
    glColor3f(1.0f, 0.0f, 0.0f); // Red entropy function
    glBegin(GL_LINE_STRIP);

    for (int x = 0; x <= graphWidth; ++x)
    {
        // Normalize x to range [-1, 1]
        float y = entropy.getY(x);
        glVertex2i(graphX + x, graphY + static_cast<int>(y));
    }
    glEnd();

    // Draw axis labels
    drawText("t", graphX + graphWidth / 2 - 10, graphY - 30); // Horizontal axis label
    drawText("H", graphX - 40, graphY + graphHeight / 2);  // Vertical axis label

    // Restore previous matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void RendererOpenGL1::renderPoints()
{
	// Cell spacing.
	const float GRID_SIZE =  0.5 / SIDE;
	// Size of each lattice point.
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	for (int i = 0; i < SIDE; i++)
	{
		for (int j = 0; j < SIDE; j++)
		{
			for (int k = 0; k < SIDE; k++)
			{
				COLORREF color = automaton::voxels[i*SIDE2 + j*SIDE + k];
				if (!color)
					continue;
				// Extrair os componentes R, G, B
				BYTE r = GetRValue(color);
				BYTE g = GetGValue(color);
				BYTE b = GetBValue(color);

				// Converter para valores normalizados entre 0.0 e 1.0
				GLdouble red = r / 255.0;
				GLdouble green = g / 255.0;
				GLdouble blue = b / 255.0;

				float alpha = 0.5;
				// Definir a cor no OpenGL
				glColor4d(red, green, blue, alpha);
				float px = i * GRID_SIZE - 0.25f;
				float py = j * GRID_SIZE - 0.25f;
				float pz = k * GRID_SIZE - 0.25f;
				glVertex3f(px, py, pz);
			}
		}
	}
	glEnd();
}

/*
 * Render only the centers of the bubbles.
 */
void RendererOpenGL1::renderParticles()
{
	// Cell spacing.
	const float GRID_SIZE =  0.5 / SIDE;
	// Size of each lattice point.
	glPointSize(8.0f);
	glBegin(GL_POINTS);
	for (unsigned w = 0; w < W_DIM; w++)
	{
        Cell *cell = lattice_current + w * SIDE3;
		float alpha = 0.5;
		// Definir a cor no OpenGL
		glColor4d(0.6, 1, 0, alpha);
		float px = cell->pos[0] * GRID_SIZE;
		float py = cell->pos[1] * GRID_SIZE;
		float pz = cell->pos[2] * GRID_SIZE;
		glVertex3f(px, py, pz);
	}
	glEnd();
}

void RendererOpenGL1::renderGadgets()
{
  glDisable(GL_DEPTH_TEST);
  setOrthographicProjection();
  glPointSize(1);
  glPushMatrix();
  for (Tickbox& checkbox : checkboxes)
  {
    checkbox.draw();
  }
  for (Radio& layer : layers)
  {
    layer.draw();
  }
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
 * Render objects via GUI.
 */
void RendererOpenGL1::renderObjects()
{
  glEnable(GL_DEPTH_TEST);
  glPushMatrix();
  glMultMatrixf(glm::value_ptr(mProjection));

  if (mCamera)
  {
    glPushMatrix();
    glMultMatrixf(mCamera->getMatrixFlat());
  }
  renderCenter();
  if (checkboxes[2].getState())
    renderGrid();
  if (checkboxes[5].getState())
    renderAxes();
  entropyFlag = checkboxes[3].getState();
  if (checkboxes[4].getState())
  {
	  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode.
	  renderCube();
	  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore fill mode.
  }
  if (checkboxes[6].getState())
    renderParticles();
  if (checkboxes[0].getState() || checkboxes[1].getState())
    renderPoints();
  if (mCamera)
  {
    glPopMatrix();
  }
  glPopMatrix();
  renderGadgets();
}

void RendererOpenGL1::renderAxes()
{
  glLineWidth(2);
  glBegin(GL_LINES);
  glColor3f ( 0.6f,    0.f,  0.f);
  glVertex3f( 0.0f,   0.f,  0.f);
  glVertex3f( 0.5f,   0.f,  0.f);
  glColor3f ( 0.f,    0.6f,  0.f);
  glVertex3f( 0.f,    0.f,  0.f);
  glVertex3f( 0.f,   0.5f,  0.f);
  glColor3f ( 0.f,    0.f,  0.6f);
  glVertex3f( 0.0f,   0.f,  0.f);
  glVertex3f( 0.f,    0.f,  0.5f);
  glEnd();
  glLineWidth(1);
}

void RendererOpenGL1::renderCube()
{
    GLfloat alpha = 0.6f;
    glBegin(GL_TRIANGLES);
    // Adjusted coordinates for centering
    glColor4f(0.8f, 0.4f, 0.4f, alpha);
    glVertex3f(0.25f , -0.25f , -0.25f );
    glVertex3f(0.25f ,  0.25f , -0.25f );
    glVertex3f(0.25f ,  0.25f ,  0.25f );
    glVertex3f(0.25f , -0.25f , -0.25f );
    glVertex3f(0.25f , -0.25f ,  0.25f );
    glVertex3f(0.25f ,  0.25f ,  0.25f );
    //
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f(-0.25f ,  0.25f , -0.25f );
    glVertex3f(-0.25f ,  0.25f ,  0.25f );
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f(-0.25f , -0.25f ,  0.25f );
    glVertex3f(-0.25f ,  0.25f ,  0.25f );
    //
    glVertex3f(-0.25f ,  0.25f , -0.25f );
    glVertex3f( 0.25f ,  0.25f , -0.25f );
    glVertex3f( 0.25f ,  0.25f ,  0.25f );
    glVertex3f(-0.25f ,  0.25f , -0.25f );
    glVertex3f(-0.25f ,  0.25f ,  0.25f );
    glVertex3f( 0.25f ,  0.25f ,  0.25f );
    //
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f( 0.25f , -0.25f , -0.25f );
    glVertex3f( 0.25f , -0.25f ,  0.25f );
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f(-0.25f , -0.25f ,  0.25f );
    glVertex3f( 0.25f , -0.25f ,  0.25f );
    //
    glVertex3f(-0.25f , -0.25f ,  0.25f );
    glVertex3f(-0.25f ,  0.25f ,  0.25f );
    glVertex3f( 0.25f ,  0.25f ,  0.25f );
    glVertex3f(-0.25f , -0.25f ,  0.25f );
    glVertex3f( 0.25f , -0.25f ,  0.25f );
    glVertex3f( 0.25f ,  0.25f ,  0.25f );
    //
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f(-0.25f ,  0.25f , -0.25f );
    glVertex3f( 0.25f ,  0.25f , -0.25f );
    glVertex3f(-0.25f , -0.25f , -0.25f );
    glVertex3f( 0.25f , -0.25f , -0.25f );
    glVertex3f( 0.25f ,  0.25f , -0.25f );
    glEnd();
}

void RendererOpenGL1::renderGrid()
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

void RendererOpenGL1::resize(int width, int height)
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

}

