/*
 * mygl.h
 *
 * Declares the OpenGL rendering routines.
 */

#ifndef RSMZ_RENDEREROPENGL1_H
#define RSMZ_RENDEREROPENGL1_H

#include "renderer.h"
#include <GL/glut.h>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <array>
#include <map>
#include <memory>

// --- INCLUSÕES DE OPENGL E EXTENSÕES (ORDEM CRÍTICA) ---
//#include <GL/glew.h> // <-- AGORA EM PRIMEIRO!

// Inclusões de outras bibliotecas de janela/contexto que dependem do GL
//#include <GLFW/glfw3.h>

// Inclusões GLM (Seu projeto)
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtc/matrix_transform.hpp> // perspective

// Inclusões de headers internos/do projeto
#include "model/simulation.h"
#include "radio.h"
#include "layers.h"
#include "GLutils.h"

//#define LAYERS	25	// max layers shown
#define WIDTH	480     // graph width

namespace framework
{
  using namespace std;

class GUIrenderer : public Renderer
{

private:
  void enhanceVoxel();
  std::vector<std::array<float, 2>> screenPositions;
  std::map<std::pair<int,int>, int> counts;

public:
  GUIrenderer();
  virtual ~GUIrenderer();

  void init();
  virtual void render();
  void renderAxes();
  void renderCenter();
  void renderClear();
  void renderList();
  void renderCube();
  void renderPlane();
  void renderObjects();
  void renderWavefront();
  void render2DObjects();
  void renderCounts();
  void renderGadgets();
  void renderMomentum();
  void renderSpin();
  void renderSineMask();
  void renderHunting();
  void renderCenters();
  void renderProgressBar();
  void renderCenterBox(const char* text);
  void resize(int width, int height);
  bool projectPoint(const float obj[3],
                    const GLdouble modelview[16],
                    const GLdouble projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY);
  void setProjection(const glm::mat4& proj) { mProjection = proj; }
  // In the GUIrenderer class public methods:
  void renderHyperlink();

protected:
    glm::mat4 mProjection;
    std::vector<std::array<unsigned, 3>> lastPositions;

};

void drawString8(std::string s, int x, int y);

class Tickbox
{
private:
    bool state;
    std::string label;
    int x, y;

public:
    Tickbox(int x, int y, std::string label) : state(false), label(label), x(x), y(y) {}

    void draw()
    {
      glColor3f(1.0, 1.0, 1.0);
      glBegin(GL_LINE_LOOP);
      glVertex2i(x, y);
      glVertex2i(x, y+15);
      glVertex2i(x+15, y+15);
      glVertex2i(x+15, y);
      glEnd();
      glBegin(GL_QUADS);
      if (state)
        glColor3f(0.0, 1.0, 0.0);
      else
        glColor3f(0.5, 0.5, 0.5);
      glVertex2i(x, y);
      glVertex2i(x, y+15);
      glVertex2i(x+15, y+15);
      glVertex2i(x+15, y);
      glEnd();
      glColor3f(1.0, 1.0, 1.0);
      drawString8(label, x + 22, y + 11);
      glFlush();
    }

    void setState(bool newState)
    {
      state = newState;
    }

    bool getState() const
    {
      return state;
    }

    int getX() const
    {
      return x;
    }

    int getY() const
    {
      return y;
    }

};

void drawString12(const string& text, int x, int y);
void drawBoldText(const string& text, int x, int y, float offset = 0.5f);
void drawString8(string s, int x, int y);
void render2Dstring(float x, float y, void *font, const char *string);
void render3Dstring(float x, float y, float z, void *font, const char *string);
void blinkText(unsigned long long timer);
void triggerEvent(unsigned long long timer);

extern bool active;
extern std::vector<Tickbox> checkboxes;
extern std::unique_ptr<LayerList> list;
extern std::vector<Tickbox> delays;
extern std::vector<Radio> viewpoint;
extern std::vector<Radio> projection;

void initText();
void reshape(GLFWwindow* window, int width, int height);

// In the framework namespace, with other extern declarations:
extern bool helpHover;

} // end namespace framework

#endif // RSMZ_RENDEREROPENGL1_H
