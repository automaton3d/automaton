#include "mygl.h"

namespace framework
{
extern unsigned long timer;

std::vector<Tickbox> checkboxes;
std::vector<Radio> dataset;
std::vector<Radio> viewpoint;


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dist(0, SIDE6-1);

unsigned long tbegin;

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
  checkboxes.push_back(Tickbox(50, 80, "Wavefront")); // 0
  checkboxes.push_back(Tickbox(50, 110, "Momentum"));  // 1
  checkboxes.push_back(Tickbox(50, 140, "Plane"));     // 2
  checkboxes.push_back(Tickbox(50, 170, "Cube"));      // 3
  checkboxes.push_back(Tickbox(50, 200, "Lattice"));   // 4
  checkboxes.push_back(Tickbox(50, 230, "Axes"));      // 5
  checkboxes.push_back(Tickbox(50, 260, "Track"));     // 6
  checkboxes[0].setState(true);
  checkboxes[1].setState(true);
  checkboxes[2].setState(true);
  checkboxes[5].setState(true);
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
}

void RendererOpenGL1::render()
{
  renderClear();
  renderObjects();
  renderText();
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

void RendererOpenGL1::renderText()
{
  setOrthographicProjection();
  glPushMatrix();
  glLoadIdentity();
  glColor3f(1.0f, 1.0f, 1.0f);
  unsigned long millis = GetTickCount64() - tbegin;
  char s[100];
  std::sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
  drawString(s, 900, 40);
  std::sprintf(s, "Light: %lu tick: %lu", timer / LIGHT, timer);
  drawString(s, 50, 40);

  // Get the primary monitor
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();

  // Get the video mode of the monitor
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  for(int i = 0; i < 10; i++)
    drawString(help[i], mode->width - 500, 20 * i + mode->height - 260);
  glPopMatrix();
  resetPerspectiveProjection();
}


int rev(int m, int n)
{
    int i = m;
    int xe = i / SIDE3;
    i %= SIDE3;
    int ye = i / SIDE4;
    i %= SIDE4;
    int ze = i / SIDE5;
    i %= SIDE5;

    int e = xe * SIDE3 + ye * SIDE4 + ze * SIDE5;
    return e + ((i + n) % SIDE3);
}


void RendererOpenGL1::renderPoints()
{
  // Cell spacing.
  const float GRID_SIZE =  0.5 / SIDE2;
  // Size of each lattice point.
  glPointSize(1.5f);
  glBegin(GL_POINTS);
  //
  // This debug code tests the skew code
  //
//#define DEBUG2
#ifdef DEBUG2
  glColor3d(255, 0, 0);
  int m = dist(gen);
  for (int i = 0; i < SIDE6; i++)
  {
    int n = (m/SIDE3)*SIDE3+(m%SIDE3+(i%SIDE3))%SIDE3;
	int xi = n % SIDE;
	int yi = (n / SIDE) % SIDE;
	int zi = (n / SIDE2) % SIDE;
	int xe = (n / SIDE3) % SIDE;
	int ye = (n / SIDE4) % SIDE;
	int ze = (n / SIDE5) % SIDE;

	int x = xe * SIDE + xi;
	int y = ye * SIDE + yi;
	int z = ze * SIDE + zi;

	float px = x * GRID_SIZE - 0.25f;
	float py = y * GRID_SIZE - 0.25f;
	float pz = z * GRID_SIZE - 0.25f;
	glVertex3f(px, py, pz);
  }
#endif

//#define DEBUG
#ifdef DEBUG

    automaton::Cell *ptr = automaton::latt0;
    for (int i = 0; i < SIDE6; i++, ptr++)
    {
      if (!ZERO(ptr->p))
      {
            glColor3d(255, 0, 0);
            float px = ptr->p[0] * GRID_SIZE - 0.25f;
            float py = ptr->p[1] * GRID_SIZE - 0.25f;
            float pz = ptr->p[2] * GRID_SIZE - 0.25f;
            glVertex3f(px+0.2, py, pz);
      }
      if (!ZERO(ptr->s))
      {
            glColor3d(0, 255, 0);
            float px = ptr->s[0] * GRID_SIZE - 0.25f;
            float py = ptr->s[1] * GRID_SIZE - 0.25f;
            float pz = ptr->s[2] * GRID_SIZE - 0.25f;
            glVertex3f(px, py, pz);
      }
    }

#endif

#define SYM
#ifdef SYM

    for (int i = 0; i < SIDE6; i++)
    {
      COLORREF color = automaton::voxels[i];
      if (color != RGB(0,0,0))
      {
        int x = i % SIDE2;
        int y = (i / SIDE2) % SIDE2;
        int z = (i / SIDE4) % SIDE2;
        float b = (color >> 16) & 255;
        float g = (color >> 8) & 255;
        float r = (color & 255);
          glColor3d(r, g, b);
          float px = x * GRID_SIZE - 0.25f;
          float py = y * GRID_SIZE - 0.25f;
          float pz = z * GRID_SIZE - 0.25f;
          glVertex3f(px, py, pz);
      }
    }

#endif
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
    if (checkboxes[3].getState())
      renderCube();
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
  GLfloat alpha = .9f;

    glBegin(GL_TRIANGLES);
    glColor4f ( .8f,    0.4f,   0.4f, alpha);
    glVertex3f( 0.2f,   0.f,   0.f);
    glVertex3f( 0.2f,   0.2f,  0.f);
    glVertex3f( 0.2f,   0.2f,  0.2f);
    glVertex3f( 0.2f,   0.f,   0.f);
    glVertex3f( 0.2f,   0.f,   0.2f);
    glVertex3f( 0.2f,   0.2f,  0.2f);

    glColor4f ( .4f,    0.2f,   0.2f, alpha);
    glVertex3f( 0.f,    0.f,   0.f);
    glVertex3f( 0.f,    0.2f,  0.f);
    glVertex3f( 0.f,    0.2f,  0.2f);
    glVertex3f( 0.f,    0.f,   0.f);
    glVertex3f( 0.f,    0.f,   0.2f);
    glVertex3f( 0.f,    0.2f,  0.2f);

    glColor4f ( 0.4f,   0.8f,  0.4f, alpha);
    glVertex3f( 0.f,    0.2f,  0.f);
  glVertex3f( 0.2f,   0.2f,  0.f);
  glVertex3f( 0.2f,   0.2f,  0.2f);
    glVertex3f( 0.f,    0.2f,  0.f);
  glVertex3f( 0.f,    0.2f,  0.2f);
  glVertex3f( 0.2f,   0.2f,  0.2f);

    glColor4f ( 0.2f,   0.4f,  0.2f, alpha);
    glVertex3f( 0.f,    0.f,  0.f);
  glVertex3f( 0.2f,   0.f,  0.f);
  glVertex3f( 0.2f,   0.f,  0.2f);
    glVertex3f( 0.f,    0.f,  0.f);
  glVertex3f( 0.f,    0.f,  0.2f);
  glVertex3f( 0.2f,   0.f,  0.2f);

    glColor4f ( .4f,    .4f,   .8f, alpha);
    glVertex3f( 0.f,    0.f,   0.2f);
    glVertex3f( 0.f,    0.2f,  0.2f);
    glVertex3f( 0.2f,   0.2f,  0.2f);
    glVertex3f( 0.f,    0.f,   0.2f);
    glVertex3f( 0.2f,   0.f,   0.2f);
    glVertex3f( 0.2f,   0.2f,  0.2f);

    glColor4f ( 0.2f,   0.2f,  .4f, alpha);
    glVertex3f( 0.f,    0.f,   0.f);
    glVertex3f( 0.f,    0.2f,  0.f);
    glVertex3f( 0.2f,   0.2f,  0.f);
    glVertex3f( 0.f,    0.f,   0.f);
    glVertex3f( 0.2f,   0.f,   0.f);
    glVertex3f( 0.2f,   0.2f,  0.f);

    glEnd();
}

void RendererOpenGL1::renderGrid()
{
  float p, d = .1, mn=-1.f, mx=1.f, eps=-1e-4;
  int i, n = 20;
    #define WHITE() glColor4f(1.f, 1.f, 1.f, .2f)

  glLineWidth(1);
  glBegin(GL_LINES);

  for(i = 0; i <= n; ++i)
  {
    p = mn + i*d;

        if (i == 0 || i == 10 || i == n)
        {
            glColor4f(0.f, 1.f, 0.f, .3f);
        }
        else
        {
            WHITE();
        }
    glVertex3f(p, mn, eps);
    glVertex3f(p, mx, eps);

        if (i == 0 || i == 10 || i == n)
        {
            glColor4f(1.f, 0.f, 0.f, .3f);
        }
        else
        {
            WHITE();
        }
    glVertex3f(mn, p, eps);
    glVertex3f(mx, p, eps);
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

