/*
 * slider.h
 *
 *  Created on: 15 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef SLIDER_H_
#define SLIDER_H_

#include <GL/glut.h>

class VerticalSlider
{
  private:
    float x, y;           // Bottom-left corner of the track
    float width, height;  // Dimensions of the track
    float thumbY;         // Current Y position of the thumb
    float thumbHeight;    // Height of the thumb
    bool dragging;        // Is the thumb being dragged?

  public:
    VerticalSlider(float x, float y, float width, float height, float thumbHeight)
        : x(x), y(y), width(width), height(height),
          thumbY(y), thumbHeight(thumbHeight), dragging(false) {}

    void draw() const
    {
      // Draw the track
      glColor3f(0.6f, 0.6f, 0.6f);
      glRectf(x, y, x + width, y + height);
      // Draw the thumb
      glColor3f(0.2f, 0.2f, 0.8f);
      glRectf(x, thumbY, x + width, thumbY + thumbHeight);
    }

    void onMouseClick(int button, int state, int mouseX, int mouseY, int windowHeight)
    {
      mouseY = windowHeight - mouseY; // Convert to OpenGL coordinates
      if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
      {
      	//printf("mouseX(%d) >= x(%f) && mouseX(%d) <= x+width(%f) && mouseY(%d) >= thumbY(%f) && mouseY(%d) <= thumbY(%f)+thumbHeight(%f)\t%d\n", mouseX, x, mouseX, x+width, mouseY, thumbY, mouseY, thumbY, thumbHeight, windowHeight);
        if (mouseX >= x && mouseX <= x + width &&
            mouseY >= windowHeight-thumbY && mouseY <= windowHeight-thumbY - thumbHeight)
        {
          // printf("dragging %d, %d\n", mouseX, mouseY);
          dragging = true;
        }
      }
      else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
      {
        dragging = false;
      }
    }

    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (dragging)
      {
        mouseY = windowHeight - mouseY; // Convert to OpenGL coordinates
        // Update thumb position
        thumbY = mouseY - thumbHeight / 2;
        // Clamp thumb position within the track
        if (thumbY < y) thumbY = y;
        if (thumbY > y + height - thumbHeight)
        thumbY = y + height - thumbHeight;
        //glutPostRedisplay(); // Redraw the screen
      }
    }

    float getValue() const
    {
      // Map thumb position to a range (e.g., 0 to 1)
      return (thumbY - y) / (height - thumbHeight);
    }
};

#endif /* SLIDER_H_ */
