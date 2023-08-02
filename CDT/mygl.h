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
#include "model/simulation.h"

namespace framework
{
class RendererOpenGL1 : public Renderer
{
public:
  RendererOpenGL1();
  virtual ~RendererOpenGL1();

  void init();
  virtual void render();
  void renderAxes();
    void renderCenter();
  void renderClear();
  void renderCube();
  void renderGrid();
  void renderObjects();
  void renderPoints();
  void renderText();
  void renderGadgets();
  void resize(int width, int height);

protected:
    glm::mat4 mProjection;

};

void drawString(std::string s, int x, int y);

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
        drawString(label, x + 22, y + 11);
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

class Radio
{
private:
    bool selected;
    std::string label;
    int x, y;

public:
    Radio(int x, int y, std::string label)
        : selected(false), label(label), x(x), y(y) {}

    void draw()
    {
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 360; ++i)
        {
            float angle = i * (3.14159 / 180);
            float dx = x + 8 * cos(angle);
            float dy = y + 8 * sin(angle);
            glVertex2f(dx, dy);
        }
        glEnd();

        if (selected)
        {
            glColor3f(0.0, 1.0, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x, y);  // Center of the circle
            for (int i = 0; i <= 360; ++i)
            {
                float angle = i * (3.14159 / 180);
                float dx = x + 8 * cos(angle);
                float dy = y + 8 * sin(angle);
                glVertex2f(dx, dy);
            }
            glEnd();
        }
        glPointSize(2);
        glBegin(GL_POINTS);
        glColor3f(1.0, 1.0, 1.0);
        glVertex2f(x, y);
        glEnd();
        drawString(label, x + 8 + 10, y);
        glFlush();
    }

    void setSelected(bool isSelected)
    {
        selected = isSelected;
    }

    bool isSelected() const
    {
        return selected;
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

extern bool active;
extern std::vector<Tickbox> checkboxes;
extern std::vector<Radio> dataset;
extern std::vector<Radio> viewpoint;

void initText();
void reshape(GLFWwindow* window, int width, int height);

} // end namespace framework

#endif // RSMZ_RENDEREROPENGL1_H
