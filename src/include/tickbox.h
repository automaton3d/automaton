/*
 * tickbox.h
 */
#ifndef TICKBOX_H_
#define TICKBOX_H_

#include <glad/glad.h>
#include <string>
#include <functional>
#include <algorithm>
#include "text_renderer.h"
#include "draw_utils.h"

class Tickbox
{
private:
    static constexpr int BOX_WIDTH_ = 15;
    static constexpr int BOX_HEIGHT_ = 15;
    static constexpr int LABEL_OFFSET_X_ = 22;
    static constexpr int LABEL_OFFSET_Y_ = 4;

    bool state_;
    std::string label_;
    int x_, y_;

    // Color members (initialized to defaults)
    float borderColor_[3] = {1.0f, 1.0f, 1.0f};   // white border
    float labelColor_[3]  = {1.0f, 1.0f, 1.0f};   // white text
    float fillOnColor_[3] = {0.0f, 1.0f, 0.0f};   // green when ON
    float fillOffColor_[3]= {0.5f, 0.5f, 0.5f};   // gray when OFF

    float fontScale_ = 1.0f;

public:
    std::function<void(bool)> onToggle;  // Optional callback

    Tickbox(int x, int y, const std::string& label)
        : state_(false), label_(label), x_(x), y_(y) {}

    void initGL();

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
    void setFontScale(float s) { fontScale_ = s; }

    void draw(TextRenderer& renderer, int screenWidth, int screenHeight) const
    {
        glm::vec3 border(borderColor_[0], borderColor_[1], borderColor_[2]);
        glm::vec3 fill(state_ ? fillOnColor_[0] : fillOffColor_[0],
                   state_ ? fillOnColor_[1] : fillOffColor_[1],
                   state_ ? fillOnColor_[2] : fillOffColor_[2]);
        glm::vec3 label(labelColor_[0], labelColor_[1], labelColor_[2]);

        // Border - using glm::vec2 instead of std::pair
        ::drawLineLoop2D({glm::vec2(x_, y_),
                      glm::vec2(x_+BOX_WIDTH_, y_),
                      glm::vec2(x_+BOX_WIDTH_, y_+BOX_HEIGHT_),
                      glm::vec2(x_, y_+BOX_HEIGHT_)},
                     border, screenWidth, screenHeight, 1.0f);

        // Fill
        ::drawQuad2D(x_, y_, x_+BOX_WIDTH_, y_+BOX_HEIGHT_,
                 fill, screenWidth, screenHeight);

        // Label
        renderer.RenderText(label_,
                        x_ + LABEL_OFFSET_X_,
                        y_ + LABEL_OFFSET_Y_,
                        fontScale_,
                        label,
                        screenWidth, screenHeight);
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
    void setX(int x) { x_ = x; }
    void setY(int y) { y_ = y; }
};

#endif /* TICKBOX_H_ */
