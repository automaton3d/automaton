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
    float x_, y_;           // Bottom-left corner of the track
    float width_, height_;  // Dimensions of the track
    float thumbX_;         // Current X position of the thumb
    float thumbWidth_;     // Width of the thumb
    bool dragging_;        // Is the thumb being dragged?
    float dragOffsetX_;    // Offset from thumb left edge when drag started
    bool hoverReset_;

  public:
    /**
     * Constructor.
     */
    HSlider(float x, float y, float width, float height, float thumbWidth)
        : x_(x), y_(y), width_(width), height_(height),
          thumbX_(x), thumbWidth_(thumbWidth), dragging_(false), dragOffsetX_(0), hoverReset_(false)
    {
    }

    /**
     * Renders the slider.
     */
    void draw() const
    {
      // Track
      glColor3f(0.6f, 0.6f, 0.6f);
      glRectf(x_, y_, x_ + width_, y_ + height_);

      // Thumb
      glColor3f(0.2f, 0.2f, 0.8f);
      glRectf(thumbX_, y_, thumbX_ + thumbWidth_, y_ + height_);

      // Thumb outline
      glColor3f(0.0f, 0.7f, 0.9f);
      glBegin(GL_LINE_LOOP);
        glVertex2f(thumbX_ - 1, y_ - 1);
        glVertex2f(thumbX_ + thumbWidth_ + 1, y_ - 1);
        glVertex2f(thumbX_ + thumbWidth_ + 1, y_ + height_ + 1);
        glVertex2f(thumbX_ - 1, y_ + height_ + 1);
      glEnd();

      // ---- Reset triangle (above center) ----
      float triBaseX = x_ + width_ * 0.5f;
      float triBaseY = y_ + height_ - 33.0f;
      float triSize  = 10.0f;

      // ✅ Change color when hovered
      if (hoverReset_)
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
        if (mouseX >= thumbX_ && mouseX <= thumbX_ + thumbWidth_ &&
            mouseY >= y_ && mouseY <= y_ + height_)
        {
          dragging_ = true;
          dragOffsetX_ = mouseX - thumbX_;
          return;
        }

        // --- Triangle click ---
        float triBaseX = x_ + width_ * 0.5f;
        float triBaseY = y_ + height_ - 33.0f;
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
        dragging_ = false;
      }
    }

    void onMouseMove(int mouseX, int mouseY)
    {
      float triBaseX = x_ + width_ * 0.5f;
      float triBaseY = y_ + height_ - 33.0f;
      float triSize  = 10.0f;
      float minX = triBaseX - triSize / 2.0f;
      float maxX = triBaseX + triSize / 2.0f;
      float minY = triBaseY;
      float maxY = triBaseY + triSize;

      hoverReset_ = (mouseX >= minX && mouseX <= maxX &&
                    mouseY >= minY && mouseY <= maxY);
    }

    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (dragging_)
      {
        thumbX_ = mouseX - dragOffsetX_;
        if (thumbX_ < x_) thumbX_ = x_;
        if (thumbX_ > x_ + width_ - thumbWidth_) thumbX_ = x_ + width_ - thumbWidth_;
      }
    }


    /**
     * Returns normalized slider value [0..1]
     * 0 = left, 1 = right
     */
    float getValue() const
    {
      if (width_ <= thumbWidth_)
        return 0.0f;
      // Map thumb position to [0..1]
      return (thumbX_ - x_) / (width_ - thumbWidth_);
    }

    /**
     * Sets the slider value [0..1]
     */
    void setValue(float value)
    {
      if (value < 0.0f) value = 0.0f;
      if (value > 1.0f) value = 1.0f;
      thumbX_ = x_ + value * (width_ - thumbWidth_);
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
      return dragging_;
    }

    /**
     * Returns the X position of the slider
     */
    float getX() const
    {
      return x_;
    }

    /**
     * Returns the Y position of the slider
     */
    float getY() const
    {
      return y_;
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

      thumbX_ = x_ + pos * (width_ - thumbWidth_);
    }
  };

} // namespace framework

#endif // HSLIDER_H_
