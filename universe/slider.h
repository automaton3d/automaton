/*
 * slider.h
 *
 *  Created on: 15 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef SLIDER_H_
#define SLIDER_H_

#include <GLFW/glfw3.h>
#include <GL/gl.h>

class LayerSlider
{
  private:
    float x, y;           // Bottom-left corner of the track
    float width, height;  // Dimensions of the track
    float thumbY;         // Current Y position of the thumb
    float thumbHeight;    // Height of the thumb
    bool dragging;        // Is the thumb being dragged?

  public:
    LayerSlider(float x, float y, float width, float height, float thumbHeight)
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

      // Thumb outline
      glColor3f(0.0f, 0.7f, 0.9f);
      glBegin(GL_LINE_LOOP);
        glVertex2f(x - 1, thumbY - 1);
        glVertex2f(x + width + 1, thumbY - 1);
        glVertex2f(x + width + 1, thumbY + thumbHeight + 1);
        glVertex2f(x - 1, thumbY + thumbHeight + 1);
      glEnd();
    }

    // GLFW mouse button handler
    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      // Flip Y to OpenGL coordinates
      mouseY = windowHeight - mouseY;

      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
        // Check if inside thumb bounds
        if (mouseX >= x && mouseX <= x + width &&
            mouseY >= thumbY && mouseY <= thumbY + thumbHeight)
        {
          dragging = true;
        }
      }
      else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
      {
        dragging = false;
      }
    }

    // GLFW cursor motion handler
    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (dragging)
      {
        mouseY = windowHeight - mouseY; // Flip to OpenGL coords

        // Center thumb around mouse Y
        thumbY = mouseY - thumbHeight / 2;

        // Clamp thumb within track
        if (thumbY < y) thumbY = y;
        if (thumbY > y + height - thumbHeight)
          thumbY = y + height - thumbHeight;
      }
    }

    float getValue() const
    {
      // Map thumb position to [0..1]
      return (thumbY - y) / (height - thumbHeight);
    }
};

#endif /* SLIDER_H_ */
