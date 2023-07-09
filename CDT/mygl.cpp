#include "mygl.h"
#include <GL/glut.h>
#include <cstdio>

namespace automaton
{
  extern unsigned long timer;
  extern Cell *latt0;
}

void explore(int org[3], int level);

namespace framework
{

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

	// Enable transparency.
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	tbegin = GetTickCount64();
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

void resetPerspectiveProjectionxx()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
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
	glRasterPos2f(50, 40);

	unsigned long millis = GetTickCount64() - tbegin;
	char s[100];
	std::sprintf(s, "Elapsed %.1fs ", millis / 1000.0);
	drawString(s, 900, 40);
	//
	std::sprintf(s, "Light: %lu tick: %lu", automaton::timer / LIGHT, automaton::timer);
	drawString(s, 50, 40);
	//
	for(int i = 0; i < 10; i++)
		drawString(help[i], 50, 20 * i + 80);
	glPopMatrix();
	resetPerspectiveProjection();
}

void RendererOpenGL1::renderPoints()
{
	// Cell spacing.
    const float GRID_SIZE =  0.5 / SIDE2;
    // Size of each lattice point.
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    //
    for (int i = 0; i < SIDE6; i++)
    {
  	  COLORREF color = automaton::voxels[i];
  	  if (color != RGB(0,0,0))
  	  {
  		  int x = i % SIDE2;
  		  int y = (i / SIDE2) % SIDE2;
  		  int z = (i / SIDE4) % SIDE2;
  		  float r = (color >> 16) & 255;
  		  float g = (color >> 8) & 255;
  		  float b = (color & 255);
          glColor3d(r, g, b);
          float px = x * GRID_SIZE - 0.25f;
          float py = y * GRID_SIZE - 0.25f;
          float pz = z * GRID_SIZE - 0.25f;
          glVertex3f(px, py, pz);
  	  }
    }
    glEnd();
}

void RendererOpenGL1::renderObjects()
{
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(mProjection));

    if (mCamera)
    {
        glPushMatrix();
        glMultMatrixf(mCamera->getMatrixFlat());
    }
    renderCenter();
	renderGrid();
	renderAxes();
//	renderCube();
	renderPoints();
    if (mCamera)
    {
        glPopMatrix();
    }
    glPopMatrix();
}

void RendererOpenGL1::renderAxes()
{
	glLineWidth(2);

	glBegin(GL_LINES);
    glColor3f ( 1.f,    0.f,  0.f);
    glVertex3f( 0.0f,   0.f,  0.f);
    glVertex3f( 0.5f,   0.f,  0.f);
    glColor3f ( 0.f,    1.f,  0.f);
    glVertex3f( 0.f,    0.f,  0.f);
    glVertex3f( 0.f,   0.5f,  0.f);
    glColor3f ( 0.f,    0.f,  1.f);
    glVertex3f( 0.0f,   0.f,  0.f);
    glVertex3f( 0.f,    0.f,  0.5f);
	glEnd();
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

} // end namespace rsmz

