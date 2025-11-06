/*
 * hslider.h
 *
 * A horizontal slider object
 */

#ifndef HSLIDER_H_
#define HSLIDER_H_

#include <GLFW/glfw3.h>
#include <GL/gl.h>

namespace framework
{
  class HSlider
  {
  private:
    float x, y;           // Bottom-left corner of the track
    float width, height;  // Dimensions of the track
    float thumbX;         // Current X position of the thumb
    float thumbWidth;     // Width of the thumb
    bool dragging;        // Is the thumb being dragged?
    float dragOffsetX;    // Offset from thumb left edge when drag started
    bool hoverReset;

  public:
    /**
     * Constructor.
     */
    HSlider(float x, float y, float width, float height, float thumbWidth)
        : x(x), y(y), width(width), height(height),
          thumbX(x), thumbWidth(thumbWidth), dragging(false), dragOffsetX(0), hoverReset(false)
    {
    }

    /**
     * Renders the slider.
     */
    void draw() const
    {
      // Track
      glColor3f(0.6f, 0.6f, 0.6f);
      glRectf(x, y, x + width, y + height);

      // Thumb
      glColor3f(0.2f, 0.2f, 0.8f);
      glRectf(thumbX, y, thumbX + thumbWidth, y + height);

      // Thumb outline
      glColor3f(0.0f, 0.7f, 0.9f);
      glBegin(GL_LINE_LOOP);
        glVertex2f(thumbX - 1, y - 1);
        glVertex2f(thumbX + thumbWidth + 1, y - 1);
        glVertex2f(thumbX + thumbWidth + 1, y + height + 1);
        glVertex2f(thumbX - 1, y + height + 1);
      glEnd();

      // ---- Reset triangle (above center) ----
      float triBaseX = x + width * 0.5f;
      float triBaseY = y + height - 33.0f;
      float triSize  = 10.0f;

      // ✅ Change color when hovered
      if (hoverReset)
        glColor3f(1.0f, 0.7f, 0.1f); // bright yellow
      else
        glColor3f(0.8f, 0.3f, 0.2f); // normal orange-red

      glBegin(GL_TRIANGLES);
        glVertex2f(triBaseX - triSize / 2.0f, triBaseY);
        glVertex2f(triBaseX + triSize / 2.0f, triBaseY);
        glVertex2f(triBaseX, triBaseY + triSize);
      glEnd();

      glColor3f(0.2f, 0.2f, 0.2f);
      glBegin(GL_LINE_LOOP);
        glVertex2f(triBaseX - triSize / 2.0f, triBaseY);
        glVertex2f(triBaseX + triSize / 2.0f, triBaseY);
        glVertex2f(triBaseX, triBaseY + triSize);
      glEnd();
    }

    /**
     * GLFW mouse button handler.
     */
    // ---- Handle mouse button ----
    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
        // --- Thumb click ---
        if (mouseX >= thumbX && mouseX <= thumbX + thumbWidth &&
            mouseY >= y && mouseY <= y + height)
        {
          dragging = true;
          dragOffsetX = mouseX - thumbX;
          return;
        }

        // --- Triangle click ---
        float triBaseX = x + width * 0.5f;
        float triBaseY = y + height - 33.0f;
        float triSize  = 10.0f;
        float minX = triBaseX - triSize / 2.0f;
        float maxX = triBaseX + triSize / 2.0f;
        float minY = triBaseY;
        float maxY = triBaseY + triSize;

        if (mouseX >= minX && mouseX <= maxX &&
            mouseY >= minY && mouseY <= maxY)
        {
          setThumbPosition(0.5f);
          return;
        }
      }
      else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
      {
        dragging = false;
      }
    }

    void onMouseMove(int mouseX, int mouseY)
    {
      float triBaseX = x + width * 0.5f;
      float triBaseY = y + height - 33.0f;
      float triSize  = 10.0f;
      float minX = triBaseX - triSize / 2.0f;
      float maxX = triBaseX + triSize / 2.0f;
      float minY = triBaseY;
      float maxY = triBaseY + triSize;

      hoverReset = (mouseX >= minX && mouseX <= maxX &&
                    mouseY >= minY && mouseY <= maxY);
    }

    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (dragging)
      {
        thumbX = mouseX - dragOffsetX;
        if (thumbX < x) thumbX = x;
        if (thumbX > x + width - thumbWidth) thumbX = x + width - thumbWidth;
      }
    }


    /**
     * Returns normalized slider value [0..1]
     * 0 = left, 1 = right
     */
    float getValue() const
    {
      if (width <= thumbWidth)
        return 0.0f;
      // Map thumb position to [0..1]
      return (thumbX - x) / (width - thumbWidth);
    }

    /**
     * Sets the slider value [0..1]
     */
    void setValue(float value)
    {
      if (value < 0.0f) value = 0.0f;
      if (value > 1.0f) value = 1.0f;
      thumbX = x + value * (width - thumbWidth);
    }

    /**
     * Computes first index shown in a window.
     */
    int getFirstIndex(int totalItems, int windowSize = 25) const
    {
      if (totalItems <= windowSize)
        return 0;

      int maxFirst = totalItems - windowSize;

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

    /**
     * Returns true if thumb is currently being dragged
     */
    bool isDragging() const
    {
      return dragging;
    }

    /**
     * Returns the X position of the slider
     */
    float getX() const
    {
      return x;
    }

    /**
     * Returns the Y position of the slider
     */
    float getY() const
    {
      return y;
    }

    /**
     * Computes which slice index based on slider position
     * Maps slider value [0..1] to slice [0..totalSlices-1]
     */
    int getSliceIndex(int totalSlices) const
    {
      if (totalSlices <= 1)
        return 0;

      float v = getValue();
      if (v < 0.0f) v = 0.0f;
      if (v > 1.0f) v = 1.0f;

      // Map to slice index
      int index = static_cast<int>(v * (totalSlices - 1) + 0.5f);
      if (index < 0) index = 0;
      if (index >= totalSlices) index = totalSlices - 1;

      return index;
    }

    void setThumbPosition(float pos)
    {
      if (pos < 0.0f) pos = 0.0f;
      if (pos > 1.0f) pos = 1.0f;

      thumbX = x + pos * (width - thumbWidth);
    }
  };

} // namespace framework

#endif // HSLIDER_H_
