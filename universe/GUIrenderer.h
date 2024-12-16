/*
 * mygl.h
 *
 * Declares the OpenGL rendering routines.
 */

#ifndef RSMZ_RENDEREROPENGL1_H
#define RSMZ_RENDEREROPENGL1_H

#include "renderer.h"
#include <cstdlib>
#include <GL/gl.h>
#include <math.h>
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtc/matrix_transform.hpp> // perspective
#include <GLFW/glfw3.h>
#include <GL/glut.h>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

#include "model/simulation.h"
#include "radio.h"
#include "layers.h"

//#define LAYERS	25	// max layers shown
#define WIDTH	480     // graph width

namespace framework
{
  using namespace std;

class GUIrenderer : public Renderer
{
  void enhanceVoxel();

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
  void renderTextObjects();
  void renderGadgets();
  void renderEntropy();
  void renderParticles();
  void renderProgressBar();
  void renderCenterBox(const char* text);
  void resize(int width, int height);

protected:
    glm::mat4 mProjection;

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

    int getX()
    {
      return x;
    }

    int getY()
    {
      return y;
    }
};

void drawString12(const string& text, int x, int y);
void drawBoldText(const string& text, int x, int y, float offset = 0.5f);
void drawString8(string s, int x, int y);
void render2Dstring(float x, float y, void *font, const char *string);
void render3Dstring(float x, float y, float z, void *font, const char *string);

extern bool active;
extern std::vector<Tickbox> checkboxes;
extern LayerList list;
//extern std::vector<Radio> layers;
extern std::vector<Radio> dataset;
extern std::vector<Radio> viewpoint;

void initText();
void reshape(GLFWwindow* window, int width, int height);

} // end namespace framework

#endif // RSMZ_RENDEREROPENGL1_H
