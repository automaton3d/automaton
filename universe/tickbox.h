/*
 * tickbox.h
 */

#ifndef TICKBOX_H_
#define TICKBOX_H_

#include <GL/gl.h>
#include <math.h>
#include <string>
#include "GLutils.h"

namespace framework
{
  extern void drawString8(std::string s, int x, int y);
}

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
      framework::drawString8(label, x + 22, y + 11);
    }

    void setState(bool newState)
    {
      state = newState;
    }

    bool getState() const
    {
      return state;
    }

    void flipState()
    {
      state = !state;
    }

    int getX() const
    {
      return x;
    }

    int getY() const
    {
      return y;
    }

    bool clicked(double xpos, double ypos, int windowHeight) const
    {
      return xpos >= x && xpos <= x + 80 &&
             ypos >= y && ypos <= y + 15;
    }

};


#endif /* TICKBOX_H_ */
