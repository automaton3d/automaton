/*
 * vslider.h
 *
 * A vertical slider object with adaptive thumb scaling
 */

#ifndef VSLIDER_H_
#define VSLIDER_H_

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <iostream>
#include <algorithm>  // for std::clamp

namespace framework
{
  class VSlider
  {
  private:
    float x, y;           // Bottom-left corner of the track
    float width, height;  // Dimensions of the track
    float thumbY;         // Current Y position of the thumb
    float thumbHeight;    // Height of the thumb
    bool dragging;        // Is the thumb being dragged?
    bool visible;         // Should the slider be drawn?

  public:
    /**
     * Constructor.
     */
    VSlider(float x, float y, float width, float height, float thumbHeight)
        : x(x), y(y), width(width), height(height),
          thumbY(y), thumbHeight(thumbHeight),
          dragging(false), visible(true)
    {
    }

    void setVisible(bool state) { visible = state; }

    /**
     * Renders the slider (if visible).
     */
    void draw() const
    {
      if (!visible) return;

      // Track
      glColor3f(0.6f, 0.6f, 0.6f);
      glRectf(x, y, x + width, y + height);

      // Thumb
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

    /**
     * Mouse press/release.
     */
    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      if (!visible) return;

      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
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

    /**
     * Mouse drag movement.
     */
    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (!visible || !dragging) return;

      thumbY = mouseY - thumbHeight / 2;
      thumbY = std::clamp(thumbY, y, y + height - thumbHeight);
    }

    /**
     * Normalized slider value [0..1].
     */
    float getValue() const
    {
      if (!visible || height <= thumbHeight)
        return 0.0f;

      return (thumbY - y) / (height - thumbHeight);
    }

    /**
     * Compute first index shown in window.
     */
    int getFirstIndex(int totalItems, int windowSize)
    {
      if (totalItems <= windowSize)
      {
        visible = false;
        thumbY = y;
        thumbHeight = height; // fill track so nothing shows distorted
        return 0;
      }

      visible = true;

      // Recalculate thumb height proportionally
      float ratio = static_cast<float>(windowSize) / totalItems;
      thumbHeight = std::clamp(height * ratio, 10.0f, height - 4.0f);

      // Compute scroll offset
      int maxFirst = totalItems - windowSize;
      float v = std::clamp(getValue(), 0.0f, 1.0f);

      int index = static_cast<int>(v * maxFirst + 0.5f);
      return std::clamp(index, 0, maxFirst);
    }

    bool isDragging() const { return dragging; }
  };

} // namespace framework

#endif /* VSLIDER_H_ */
