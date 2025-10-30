/*
 * radio.h
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <GL/gl.h>
#include <math.h>
#include <string>

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
    drawAt(x, y); // reuse the flexible version
  }

  void drawAt(int xPos, int yPos)
  {
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; ++i)
    {
      float angle = i * (3.14159 / 180);
      float dx = xPos + 8 * cos(angle);
      float dy = yPos + 8 * sin(angle);
      glVertex2f(dx, dy);
    }
    glEnd();

    if (selected)
    {
      glColor3f(0.0, 1.0, 0.0);
      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(xPos, yPos);
      for (int i = 0; i <= 360; ++i)
      {
        float angle = i * (3.14159 / 180);
        float dx = xPos + 8 * cos(angle);
        float dy = yPos + 8 * sin(angle);
        glVertex2f(dx, dy);
      }
      glEnd();
    }
    glPointSize(2);
    glBegin(GL_POINTS);
    glColor3f(1.0, 1.0, 1.0);
    glVertex2f(xPos, yPos);
    glEnd();
    framework::drawString8(label, xPos + 8 + 10, yPos);
  }

  void setSelected(bool isSelected) { selected = isSelected; }

  bool isSelected() const
  {
    return selected;
  }

  bool clicked(double xpos, double ypos) const
  {
    return xpos >= x - 2 && xpos <= x + 100 &&
           ypos >= y - 5 && ypos <= y + 25;
  }

  bool clicked(double xpos, double ypos, int windowHeight) const
  {
      double yGL = ypos;//windowHeight - ypos;
      return xpos >= x - 2 && xpos <= x + 100 &&
             yGL >= y - 5 && yGL <= y + 25;
  }

  int getX() const { return x; }
  int getY() const { return y; }
};

#endif /* RADIO_H_ */
