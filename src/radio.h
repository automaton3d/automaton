/*
 * radio.h
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <GL/gl.h>
#include <cmath>
#include <string>

namespace framework
{
  extern void drawString8(std::string s, int x, int y);
}

class Radio
{
private:
  bool selected_;
  std::string label_;
  int x_, y_;
  static constexpr int RADIO_HEIGHT_ = 15;

public:
  Radio(int x, int y, std::string label)
        : selected_(false), label_(label), x_(x), y_(y) {}

  void draw()
  {
    drawAt(x_, y_);
  }

  void drawAt(int xPos, int yPos)
  {
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; ++i)
    {
      float angle = i * (3.14159f / 180.0f);
      float dx = xPos + 8 * cos(angle);
      float dy = yPos + 8 * sin(angle);
      glVertex2f(dx, dy);
    }
    glEnd();

    if (selected_)
    {
      glColor3f(0.0, 1.0, 0.0);
      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(xPos, yPos);
      for (int i = 0; i <= 360; ++i)
      {
        float angle = i * (3.14159f / 180.0f);
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

    // Vertically centered label
    framework::drawString8(label_, xPos + 18, yPos + 3);
  }

  void setSelected(bool isSelected) { selected_ = isSelected; }

  bool isSelected() const { return selected_; }

  bool clicked(double xpos, double ypos) const
  {
    return xpos >= x_ - 2 && xpos <= x_ + 100 &&
           ypos >= y_ - 9 && ypos <= y_ + 7;
  }

  bool clickedAt(double xpos, double ypos, int drawX, int drawY) const
  {
    return xpos >= drawX - 2 && xpos <= drawX + 100 &&
           ypos >= drawY - 9 && ypos <= drawY + 7;
  }

  int getX() const { return x_; }
  int getY() const { return y_; }
};

#endif /* RADIO_H_ */
