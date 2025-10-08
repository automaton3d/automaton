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

namespace framework
{
  class LayerSlider
  {
  private:
    float x, y;           // Bottom-left corner of the track
    float width, height;  // Dimensions of the track
    float thumbY;         // Current Y position of the thumb
    float thumbHeight;    // Height of the thumb
    bool dragging;        // Is the thumb being dragged?

  public:

    /**
     * Constructor.
     */
    LayerSlider(float x, float y, float width, float height, float thumbHeight)
        : x(x), y(y), width(width), height(height),
          thumbY(y), thumbHeight(thumbHeight), dragging(false) {}

    /**
     * Renders the slider.
     */
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
      mouseY = 1080 - mouseY - thumbHeight; // Patch
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

    /*
     * GLFW cursor motion handler.
     */
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

    /**
     * Computes first index shown in 25-row window.
     */
    int getFirstIndex(int totalItems) const
    {
      constexpr int WINDOW_SIZE = 25;
      if (totalItems <= WINDOW_SIZE)
    	return 0;
      int maxFirst = totalItems - WINDOW_SIZE;
      // Ensure getValue() in [0,1]
      float v = getValue();
      if (v < 0.0f) v = 0.0f;
      if (v > 1.0f) v = 1.0f;
      // Round to nearest integer
      int index = static_cast<int>(v * maxFirst + 0.5f);
      if (index < 0) index = 0;
      if (index > maxFirst) index = maxFirst;
      return index;
    }
  };

} // namespace framework

#endif /* SLIDER_H_ */
