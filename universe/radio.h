/*
 * radio.h
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <string>
#include <GL/gl.h>
#include <math.h>

namespace framework
{
  extern void drawString8(std::string s, int x, int y);
}

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
    framework::drawString8(label, x + 8 + 10, y);
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

#endif /* RADIO_H_ */
