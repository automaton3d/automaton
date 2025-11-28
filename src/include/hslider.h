/*
 * hslider.h
 *
 * A horizontal slider object
 */

#ifndef HSLIDER_H_
#define HSLIDER_H_

#include <glm/glm.hpp>
#include <algorithm>
#include <GLFW/glfw3.h>
#include "draw_utils.h"

namespace framework
{
  class HSlider
  {
  private:
    float x_, y_;           // Bottom-left corner of the track
    float width_, height_;  // Dimensions of the track
    float thumbX_;          // Current X position of the thumb
    float thumbWidth_;      // Width of the thumb
    bool dragging_;         // Is the thumb being dragged?
    float dragOffsetX_;     // Offset from thumb left edge when drag started
    bool hoverReset_;       // Is mouse hovering over reset triangle?

  public:
    HSlider(float x, float y, float width, float height, float thumbWidth)
        : x_(x), y_(y), width_(width), height_(height),
          thumbX_(x), thumbWidth_(thumbWidth),
          dragging_(false), dragOffsetX_(0.0f), hoverReset_(false)
    {}

    // Default constructor for deferred initialization
    HSlider()
        : x_(0.0f), y_(0.0f), width_(100.0f), height_(20.0f),
          thumbX_(0.0f), thumbWidth_(20.0f),
          dragging_(false), dragOffsetX_(0.0f), hoverReset_(false)
    {}

    void draw(int winW, int winH) const
    {
      // Track background
      drawQuad2D(x_, y_, x_ + width_, y_ + height_,
                 glm::vec3(0.6f, 0.6f, 0.6f), winW, winH);

      // Thumb
      drawQuad2D(thumbX_, y_, thumbX_ + thumbWidth_, y_ + height_,
                 glm::vec3(0.2f, 0.2f, 0.8f), winW, winH);

      // Thumb outline
      drawLineLoop2D({{thumbX_ - 1, y_ - 1},
                      {thumbX_ + thumbWidth_ + 1, y_ - 1},
                      {thumbX_ + thumbWidth_ + 1, y_ + height_ + 1},
                      {thumbX_ - 1, y_ + height_ + 1}},
                     glm::vec3(0.0f, 0.7f, 0.9f), winW, winH, 1.0f);

      // Reset triangle (ABOVE track, pointing DOWNWARD)
      float triBaseX = x_ + width_ * 0.5f;
      float triBaseY = y_ + height_ + 10.0f;  // Above the track
      float triSize  = 10.0f;

      glm::vec3 triColor = hoverReset_ ? glm::vec3(1.0f, 0.7f, 0.1f)
                                       : glm::vec3(0.8f, 0.3f, 0.2f);

      // Triangle pointing DOWN (tip at bottom)
      drawTriangle2D({{triBaseX - triSize / 2.0f, triBaseY + triSize},  // top-left
                      {triBaseX + triSize / 2.0f, triBaseY + triSize},  // top-right
                      {triBaseX, triBaseY}},                             // bottom (tip)
                     triColor, winW, winH);

      drawLineLoop2D({{triBaseX - triSize / 2.0f, triBaseY + triSize},
                      {triBaseX + triSize / 2.0f, triBaseY + triSize},
                      {triBaseX, triBaseY}},
                     glm::vec3(0.2f, 0.2f, 0.2f), winW, winH, 1.0f);
    }

    // Handle mouse button events
    void onMouseButton(int button, int action, int mouseX, int mouseY, int windowHeight)
    {
      // Convert mouseY from top-origin to bottom-origin
      float bottomY = windowHeight - mouseY;

      if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
      {
        // Check thumb click
        if (mouseX >= thumbX_ && mouseX <= thumbX_ + thumbWidth_ &&
            bottomY >= y_ && bottomY <= y_ + height_)
        {
          dragging_ = true;
          dragOffsetX_ = mouseX - thumbX_;
          return;
        }

        // Check reset triangle click (downward-pointing triangle above track)
        float triBaseX = x_ + width_ * 0.5f;
        float triBaseY = y_ + height_ + 10.0f;
        float triSize  = 10.0f;
        float minX = triBaseX - triSize / 2.0f;
        float maxX = triBaseX + triSize / 2.0f;
        float minY = triBaseY;                    // tip of triangle
        float maxY = triBaseY + triSize;          // base of triangle

        if (mouseX >= minX && mouseX <= maxX &&
            bottomY >= minY && bottomY <= maxY)
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

    void onMouseMove(int mouseX, int mouseY, int windowHeight)
    {
      // Convert mouseY from top-origin to bottom-origin
      float bottomY = windowHeight - mouseY;

      float triBaseX = x_ + width_ * 0.5f;
      float triBaseY = y_ + height_ + 10.0f;
      float triSize  = 10.0f;
      float minX = triBaseX - triSize / 2.0f;
      float maxX = triBaseX + triSize / 2.0f;
      float minY = triBaseY;
      float maxY = triBaseY + triSize;

      hoverReset_ = (mouseX >= minX && mouseX <= maxX &&
                     bottomY >= minY && bottomY <= maxY);
    }

    void onMouseDrag(int mouseX, int mouseY, int windowHeight)
    {
      if (dragging_)
      {
        thumbX_ = mouseX - dragOffsetX_;
        thumbX_ = std::clamp(thumbX_, x_, x_ + width_ - thumbWidth_);
      }
    }

    /**
     * Resize and recenter the slider horizontally when window size changes.
     * Maintains the current slider value (thumb position ratio).
     */
    void resize(int winW, int winH)
    {
      // Save current value (0.0 to 1.0)
      float currentValue = getValue();
      
      // Recalculate centered X position
      x_ = (winW - width_) / 2.0f;
      
      // Restore thumb position based on saved value
      setValue(currentValue);
    }

    // Value operations (0.0 to 1.0)
    float getValue() const 
    { 
      if (width_ <= thumbWidth_) return 0.0f;
      return (thumbX_ - x_) / (width_ - thumbWidth_); 
    }

    void setValue(float value) 
    {
      float clamped = std::clamp(value, 0.0f, 1.0f);
      thumbX_ = x_ + clamped * (width_ - thumbWidth_);
    }

    void setThumbPosition(float pos) 
    {
      setValue(pos); // Use setValue for consistency
    }

    // Index-based operations for scrolling/selection
    int getFirstIndex(int totalItems, int windowSize = 25) const 
    {
      if (totalItems <= windowSize) return 0;
      int maxFirst = totalItems - windowSize;
      float v = getValue();
      int index = static_cast<int>(v * maxFirst + 0.5f);
      return std::clamp(index, 0, maxFirst);
    }

    int getSliceIndex(int totalSlices) const 
    {
      if (totalSlices <= 1) return 0;
      float v = getValue();
      int index = static_cast<int>(v * (totalSlices - 1) + 0.5f);
      return std::clamp(index, 0, totalSlices - 1);
    }

    // State queries
    bool isDragging() const { return dragging_; }
    bool isHoveringReset() const { return hoverReset_; }

    // Getters for layout
    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return width_; }
    float getHeight() const { return height_; }
    float getThumbX() const { return thumbX_; }
    float getThumbWidth() const { return thumbWidth_; }

    // Setters for repositioning
    void setPosition(float x, float y) 
    { 
      float currentValue = getValue();
      x_ = x; 
      y_ = y; 
      setValue(currentValue); // Preserve slider position
    }

    void setSize(float width, float height) 
    { 
      float currentValue = getValue();
      width_ = width; 
      height_ = height; 
      setValue(currentValue); // Preserve slider position
    }

    void setThumbWidth(float thumbWidth)
    {
      float currentValue = getValue();
      thumbWidth_ = thumbWidth;
      setValue(currentValue); // Preserve slider position
    }
  };

} // namespace framework

#endif // HSLIDER_H_