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
    float x_, y_;           // Bottom-left corner of the track
    float width_, height_;  // Dimensions of the track
    float thumbY_;         // Current Y position of the thumb
    float thumbHeight_;    // Height of the thumb
    bool dragging_;        // Is the thumb being dragged?
    bool visible_;         // Should the slider be drawn?

  public:
    /**
     * Constructor.
     */
    VSlider(float x, float y, float width, float height, float thumbHeight)
        : x_(x), y_(y), width_(width), height_(height),
          thumbY_(y), thumbHeight_(thumbHeight),
          dragging_(false), visible_(true)
    {
    }

    void setVisible(bool state) { visible_ = state; }

    /**
     * Renders the slider (if visible).
     */
    void draw() const
    {
      if (!visible_) return;

      // Track
      glColor3f(0.6f, 0.6f, 0.6f);
      glRectf(x_, y_, x_ + width_, y_ + height_);

      // Thumb
      glColor3f(0.2f, 0.2f, 0.8f);
      glRectf(x_, thumbY_, x_ + width_, thumbY_ + thumbHeight_);

      // Thumb outline
      glColor3f(0.0f, 0.7f, 0.9f);
      glBegin(GL_LINE_LOOP);
        glVertex2f(x_ - 1, thumbY_ - 1);
        glVertex2f(x_ + width_ + 1, thumbY_ - 1);
        glVertex2f(x_ + width_ + 1, thumbY_ + thumbHeight_ + 1);
        glVertex2f(x_ - 1, thumbY_ + thumbHeight_ + 1);
      glEnd();
    }

    /**
     * Mouse press/release.
     */
    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      if (!visible_) return;

      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
        if (mouseX >= x_ && mouseX <= x_ + width_ &&
            mouseY >= thumbY_ && mouseY <= thumbY_ + thumbHeight_)
        {
          dragging_ = true;
        }
      }
      else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
      {
        dragging_ = false;
      }
    }

    /**
     * Mouse drag movement.
     */
    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (!visible_ || !dragging_) return;

      thumbY_ = mouseY - thumbHeight_ / 2;
      thumbY_ = std::clamp(thumbY_, y_, y_ + height_ - thumbHeight_);
    }

    /**
     * Normalized slider value [0..1].
     */
    float getValue() const
    {
      if (!visible_ || height_ <= thumbHeight_)
        return 0.0f;

      return (thumbY_ - y_) / (height_ - thumbHeight_);
    }

    /**
     * Compute first index shown in window.
     */
    int getFirstIndex(int totalItems, int windowSize)
    {
      if (totalItems <= windowSize)
      {
        visible_ = false;
        thumbY_ = y_;
        thumbHeight_ = height_; // fill track so nothing shows distorted
        return 0;
      }

      visible_ = true;

      // Recalculate thumb height proportionally
      float ratio = static_cast<float>(windowSize) / totalItems;
      thumbHeight_ = std::clamp(height_ * ratio, 10.0f, height_ - 4.0f);

      // Compute scroll offset
      int maxFirst = totalItems - windowSize;
      float v = std::clamp(getValue(), 0.0f, 1.0f);

      int index = static_cast<int>(v * maxFirst + 0.5f);
      return std::clamp(index, 0, maxFirst);
    }

    bool isDragging() const { return dragging_; }
  };

} // namespace framework

#endif /* VSLIDER_H_ */
