/*
 * vslider.h (modernized)
 *
 * A vertical slider object with adaptive thumb scaling
 */

#ifndef VSLIDER_H_
#define VSLIDER_H_

#include <algorithm>  // for std::clamp
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "draw_utils.h"

namespace framework
{
  class VSlider
  {
  private:
    float x_, y_;           // Bottom-left corner of the track
    float width_, height_;  // Dimensions of the track
    float thumbY_;          // Current Y position of the thumb
    float thumbHeight_;     // Height of the thumb
    bool dragging_;         // Is the thumb being dragged?
    bool visible_;          // Should the slider be drawn?

  public:
    VSlider(float x, float y, float width, float height, float thumbHeight)
        : x_(x), y_(y), width_(width), height_(height),
          thumbY_(y), thumbHeight_(thumbHeight),
          dragging_(false), visible_(true)
    {}

    // Default constructor for cases where initialization is deferred
    VSlider()
        : x_(0.0f), y_(0.0f), width_(20.0f), height_(100.0f),
          thumbY_(0.0f), thumbHeight_(20.0f),
          dragging_(false), visible_(true)
    {}

    void setVisible(bool state) { visible_ = state; }
    bool isVisible() const { return visible_; }

    void draw(int winW, int winH) const
    {
      if (!visible_) return;

      // Track background
      drawQuad2D(x_, y_, x_ + width_, y_ + height_,
                 glm::vec3(0.6f, 0.6f, 0.6f), winW, winH);

      // Thumb
      drawQuad2D(x_, thumbY_, x_ + width_, thumbY_ + thumbHeight_,
                 glm::vec3(0.2f, 0.2f, 0.8f), winW, winH);

      // Thumb outline
      drawLineLoop2D({{x_ - 1, thumbY_ - 1},
                      {x_ + width_ + 1, thumbY_ - 1},
                      {x_ + width_ + 1, thumbY_ + thumbHeight_ + 1},
                      {x_ - 1, thumbY_ + thumbHeight_ + 1}},
                     glm::vec3(0.0f, 0.7f, 0.9f), winW, winH, 1.0f);
    }

    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      if (!visible_) return;

      // Convert mouseY from top-origin to bottom-origin if needed
      float bottomY = windowHeight - mouseY;

      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
        if (mouseX >= x_ && mouseX <= x_ + width_ &&
            bottomY >= thumbY_ && bottomY <= thumbY_ + thumbHeight_)
        {
          dragging_ = true;
        }
      }
      else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
      {
        dragging_ = false;
      }
    }

    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (!visible_ || !dragging_) return;

      // Convert mouseY from top-origin to bottom-origin if needed
      float bottomY = windowHeight - mouseY;

      thumbY_ = bottomY - thumbHeight_ / 2.0f;
      thumbY_ = std::clamp(thumbY_, y_, y_ + height_ - thumbHeight_);
    }

    float getValue() const
    {
      if (!visible_ || height_ <= thumbHeight_)
        return 0.0f;

      return (thumbY_ - y_) / (height_ - thumbHeight_);
    }

    void setValue(float value)
    {
      if (!visible_) return;
      
      float clamped = std::clamp(value, 0.0f, 1.0f);
      if (height_ > thumbHeight_)
      {
        thumbY_ = y_ + clamped * (height_ - thumbHeight_);
      }
    }

    int getFirstIndex(int totalItems, int windowSize)
    {
      if (totalItems <= windowSize)
      {
        visible_ = false;
        thumbY_ = y_;
        thumbHeight_ = height_;
        return 0;
      }

      visible_ = true;

      float ratio = static_cast<float>(windowSize) / totalItems;
      thumbHeight_ = std::clamp(height_ * ratio, 10.0f, height_ - 4.0f);

      int maxFirst = totalItems - windowSize;
      float v = std::clamp(getValue(), 0.0f, 1.0f);

      int index = static_cast<int>(v * maxFirst + 0.5f);
      return std::clamp(index, 0, maxFirst);
    }

    bool isDragging() const { return dragging_; }
    
    // Getters for layout/positioning
    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return width_; }
    float getHeight() const { return height_; }
    
    // Setters for repositioning
    void setPosition(float x, float y) { x_ = x; y_ = y; }
    void setSize(float width, float height) { width_ = width; height_ = height; }
  };

} // namespace framework

#endif /* VSLIDER_H_ */