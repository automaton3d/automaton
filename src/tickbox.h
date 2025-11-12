/*
 * tickbox.h
 */

#ifndef TICKBOX_H_
#define TICKBOX_H_

#include <GL/gl.h>
#include <string>
#include <functional>
#include "text.h"

namespace framework
{
//    extern void drawString8(std::string s, int x, int y);
}

class Tickbox
{
private:
    static constexpr int BOX_WIDTH_ = 15;
    static constexpr int BOX_HEIGHT_ = 15;
    static constexpr int LABEL_OFFSET_X_ = 22;
    static constexpr int LABEL_OFFSET_Y_ = 11;

    bool state_;
    std::string label_;
    int x_, y_;

    // Color members (initialized to original defaults)
    float borderColor_[3] = {1.0f, 1.0f, 1.0f};   // white border
    float labelColor_[3]  = {1.0f, 1.0f, 1.0f};   // white text
    float fillOnColor_[3] = {0.0f, 1.0f, 0.0f};   // green when ON
    float fillOffColor_[3]= {0.5f, 0.5f, 0.5f};   // gray when OFF

public:
    std::function<void(bool)> onToggle;  // Optional callback

    Tickbox(int x, int y, const std::string& label)
        : state_(false), label_(label), x_(x), y_(y) {}

    // ✅ NEW: setColor() with optional parameters
    void setColor(const float* border = nullptr,
                  const float* labelC = nullptr,
                  const float* fillOn = nullptr,
                  const float* fillOff = nullptr)
    {
        if (border)   std::copy(border, border + 3, borderColor_);
        if (labelC)   std::copy(labelC, labelC + 3, labelColor_);
        if (fillOn)   std::copy(fillOn, fillOn + 3, fillOnColor_);
        if (fillOff)  std::copy(fillOff, fillOff + 3, fillOffColor_);
    }

    void setPosition(int x, int y) { x_ = x; y_ = y; }

    void draw() const
    {
        // Draw border
        glColor3fv(borderColor_);
        glBegin(GL_LINE_LOOP);
        glVertex2i(x_, y_);
        glVertex2i(x_, y_ + BOX_HEIGHT_);
        glVertex2i(x_ + BOX_WIDTH_, y_ + BOX_HEIGHT_);
        glVertex2i(x_ + BOX_WIDTH_, y_);
        glEnd();

        // Fill box
        glColor3fv(state_ ? fillOnColor_ : fillOffColor_);
        glBegin(GL_QUADS);
        glVertex2i(x_, y_);
        glVertex2i(x_, y_ + BOX_HEIGHT_);
        glVertex2i(x_ + BOX_WIDTH_, y_ + BOX_HEIGHT_);
        glVertex2i(x_ + BOX_WIDTH_, y_);
        glEnd();

        // Draw label
        glColor3fv(labelColor_);
        framework::drawString(label_, x_ + LABEL_OFFSET_X_, y_ + LABEL_OFFSET_Y_, 8);
    }

    void setState(bool newState)
    {
        if (state_ != newState)
        {
            state_ = newState;
            if (onToggle) onToggle(state_);
        }
    }

    bool getState() const { return state_; }

    void toggle() { setState(!state_); }

    bool contains(int mx, int my) const
    {
        return mx >= x_ && mx <= x_ + BOX_WIDTH_ &&
               my >= y_ && my <= y_ + BOX_HEIGHT_;
    }

    bool onMouseButton(int mx, int my, bool pressed)
    {
        if (!pressed) return false;
        if (contains(mx, my))
        {
            toggle();
            return true;
        }
        return false;
    }

    int getX() const { return x_; }
    int getY() const { return y_; }

    // FIX: Removed 'const' qualifier
    void setX(int x) { x_ = x; }
    void setY(int y) { y_ = y; }
};

#endif /* TICKBOX_H_ */
