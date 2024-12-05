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
	extern EntropyCalculator entropyCalc;
}

namespace framework
{
using namespace std;

extern unsigned long long timer;

vector<Tickbox> checkboxes;
vector<Radio> layers;
vector<Radio> dataset;
vector<Radio> viewpoint;

bool entropyFlag;

random_device rd;
mt19937 gen(rd());

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

unsigned lastPos[W_DIM][3];

using namespace automaton;

RendererOpenGL1::RendererOpenGL1() : Renderer()
{
}

RendererOpenGL1::~RendererOpenGL1()
{
}

void RendererOpenGL1::init()
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
  for (unsigned w = 0; w < W_DIM && w < LAYERS; w++)
  {
	  sprintf(s, "Layer %2d", w);
	  layers.push_back(Radio(1700, 100 + 25 * w, s));
  }
  layers[0].setSelected(true);
  // Initialize entropy
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  // Initialize progress bar data
  int barWidth = viewport[2] / 4; // Bar is 1/4 of the screen width
  double totalRatio = (double) FRAME;
  barWidths[0] = (int)(barWidth * (double)CONVOL / totalRatio);                       // CONVOL stripe
  barWidths[1] = (int)(barWidth * 1 / totalRatio);                                    // COLLISION stripe
  barWidths[2] = (int)(barWidth * (double)(W_DIM + 3 * (SIDE - 1)) / totalRatio); // DIFFUSION stripe
  barWidths[3] = (int)(barWidth * (double)(3 * (SIDE - 1)) / totalRatio);             // RELOCATION stripe
  barWidths[4] = (int)(barWidth * (double)LIGHT / totalRatio);                        // LIGHT stripe
}

void RendererOpenGL1::render()
{
  renderClear();
  renderObjects();
  renderText();
  if (entropyFlag)
	  renderEntropy();
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

void drawString(string s, int x, int y)
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

/*
 * Renders a progress bar showing a complete light step.
 */
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
        	case 3: glColor3f(0.0f, 0.2f, 0.5f); break;
        	case 4: glColor3f(0.5f, 0.0f, 0.0f); break;
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

void RendererOpenGL1::renderText()
{
  setOrthographicProjection();
  glPushMatrix();
  glLoadIdentity();
  glColor3f(1.0f, 1.0f, 1.0f);
  unsigned long millis = GetTickCount64() - tbegin;
  char s[100];
  sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
  drawString(s, 50, 40);
  sprintf(s, "Light: %llu tick: %llu", timer / automaton::FRAME, timer);
  renderBitmapString(900, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);

  sprintf(s, "SIDE %u", SIDE);
  renderBitmapString(1750, 40, GLUT_BITMAP_TIMES_ROMAN_24, s);
  //
  if (poincare)
  {
    sprintf(s, "POINCARE: %llu", timer);
    renderBitmapString(300, 400, GLUT_BITMAP_TIMES_ROMAN_24, s);
  }
  // Get the primary monitor
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  // Get the video mode of the monitor
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  // Draw the help text
  for(int i = 0; i < 10; i++)
	  drawString(help[i], mode->width - 500, 20 * i + mode->height - 260);
  // Add the progress bar rendering
  renderProgressBar();
  //
  glPopMatrix();
  // Update positions
  for (unsigned w = 0; w < W_DIM && w < LAYERS; w++)
  {
      Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
      if (cell.pos[0] != lastPos[w][0] || cell.pos[1] != lastPos[w][1] || cell.pos[2] != lastPos[w][2])
      {
    	  glColor3f(1.0f, 0.0f, 1.0f);
      }
      else
      {
    	  glColor3f(1.0f, 1.0f, 0.0f);
      }
	  sprintf(s, "(%u, %u, %u)", cell.pos[0], cell.pos[1], cell.pos[2]);
	  drawString(s, 1800, 100 + 25 * w);
      lastPos[w][0] = cell.pos[0];
      lastPos[w][1] = cell.pos[1];
      lastPos[w][2] = cell.pos[2];
  }



  resetPerspectiveProjection();
}

void drawText(const string& text, int x, int y)
{
    glRasterPos2i(x, y);
    for (char c : text)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c); // Use GLUT for bitmap fonts
    }
}

void drawBoldText(const string& text, int x, int y, float offset = 0.5f)
{
    for (float dx = -offset; dx <= offset; dx += offset)
    {
        for (float dy = -offset; dy <= offset; dy += offset)
        {
            glRasterPos2i(x + dx, y + dy);
            for (char c : text)
            {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }
        }
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
        drawText(to_string(i), x - 5, graphY - 15); // Adjust x offset for centering text

        // Vertical ticks (entropy)
        int y = graphY + (i * graphHeight / numTicks);
        glBegin(GL_LINES);
        glVertex2i(graphX, y);
        glVertex2i(graphX - 5, y); // Tick length = 5
        glEnd();
        drawText(to_string(i), graphX - 20, y - 5); // Adjust y offset for centering text
    }
    Entropy entropy = entropyCalc.getEntropy();
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

void RendererOpenGL1::renderPoints()
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
				float px = x * GRID_SIZE - 0.25f;
				float py = y * GRID_SIZE - 0.25f;
				float pz = z * GRID_SIZE - 0.25f;
				glVertex3f(px, py, pz);
			}
		}
	}
	glEnd();
    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][currentLayer];
    float cx = (SIDE - cell.pos[0] - 0.5f) * GRID_SIZE - 0.25f;
    float cy = (SIDE - cell.pos[1] - 0.5f) * GRID_SIZE - 0.25f;
    float cz = (SIDE - cell.pos[2] - 0.5f) * GRID_SIZE - 0.25f;
   	glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor4d(1, 1, 1, 1);
    glVertex3f(cx, cy, cz);
    glVertex3f(cx + 0.005, cy, cz);
    glVertex3f(cx - 0.005, cy, cz);
    glVertex3f(cx, cy + 0.005, cz);
    glVertex3f(cx, cy - 0.005, cz);
    glVertex3f(cx, cy, cz + 0.005);
    glVertex3f(cx, cy, cz - 0.005);
    glVertex3f(cx + 0.01, cy, cz);
    glVertex3f(cx - 0.01, cy, cz);
    glVertex3f(cx, cy + 0.01, cz);
    glVertex3f(cx, cy - 0.01, cz);
    glVertex3f(cx, cy, cz + 0.01);
    glVertex3f(cx, cy, cz - 0.01);
    glEnd();
}

/*
 * Render the center of the bubbles only.
 */
void RendererOpenGL1::renderParticles()
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
    //
    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][currentLayer];
    float cx = (SIDE - cell.pos[0] - 0.5f) * GRID_SIZE - 0.25f;
    float cy = (SIDE - cell.pos[1] - 0.5f) * GRID_SIZE - 0.25f;
    float cz = (SIDE - cell.pos[2] - 0.5f) * GRID_SIZE - 0.25f;
   	glPointSize(1.0f);
    glBegin(GL_POINTS);
    glColor4d(1, 1, 1, 1);
    glVertex3f(cx, cy, cz);
    glVertex3f(cx + 0.005, cy, cz);
    glVertex3f(cx - 0.005, cy, cz);
    glVertex3f(cx, cy + 0.005, cz);
    glVertex3f(cx, cy - 0.005, cz);
    glVertex3f(cx, cy, cz + 0.005);
    glVertex3f(cx, cy, cz - 0.005);
    glVertex3f(cx + 0.01, cy, cz);
    glVertex3f(cx - 0.01, cy, cz);
    glVertex3f(cx, cy + 0.01, cz);
    glVertex3f(cx, cy - 0.01, cz);
    glVertex3f(cx, cy, cz + 0.01);
    glVertex3f(cx, cy, cz - 0.01);
    glEnd();
    /*
   	glPointSize(1.0f);
    glBegin(GL_POINTS);
    //
    glColor4d(1, 1, 1, 1);
    const float radius = 0.01f;
    for (int i = 0; i < 10; i++)
    {
        // Generate random offsets within the radius
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI; // Random angle for rotation
        float phi = static_cast<float>(rand()) / RAND_MAX * M_PI;         // Random angle for elevation
        float r = static_cast<float>(rand()) / RAND_MAX * radius;         // Random distance within radius

        // Convert spherical coordinates to Cartesian coordinates
        float dx = r * sinf(phi) * cosf(theta);
        float dy = r * sinf(phi) * sinf(theta);
        float dz = r * cosf(phi);

        // Draw the point at the calculated position
        glVertex3f(pcx + dx, pcy + dy, pcz + dz);
    }
    glEnd();
    */
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
    glBegin(GL_QUADS);

    glColor4f(0.8f, 0.4f, 0.4f, alpha);

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

