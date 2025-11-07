/*
 * tickbox.h
 */
#ifndef TICKBOX_H_
#define TICKBOX_H_

#include <GL/gl.h>
#include <string>
#include <functional>

namespace framework
{
    extern void drawString8(std::string s, int x, int y);
}

class Tickbox
{
private:
    static constexpr int BOX_WIDTH = 15;
    static constexpr int BOX_HEIGHT = 15;
    static constexpr int LABEL_OFFSET_X = 22;
    static constexpr int LABEL_OFFSET_Y = 11;

    bool state;
    std::string label;
    int x, y;

    // Color members (initialized to original defaults)
    float borderColor[3] = {1.0f, 1.0f, 1.0f};   // white border
    float labelColor[3]  = {1.0f, 1.0f, 1.0f};   // white text
    float fillOnColor[3] = {0.0f, 1.0f, 0.0f};   // green when ON
    float fillOffColor[3]= {0.5f, 0.5f, 0.5f};   // gray when OFF

public:
    std::function<void(bool)> onToggle;  // Optional callback

    Tickbox(int x, int y, const std::string& label)
        : state(false), label(label), x(x), y(y) {}

    // ✅ NEW: setColor() with optional parameters
    void setColor(const float* border = nullptr,
                  const float* labelC = nullptr,
                  const float* fillOn = nullptr,
                  const float* fillOff = nullptr)
    {
        if (border)   std::copy(border, border + 3, borderColor);
        if (labelC)   std::copy(labelC, labelC + 3, labelColor);
        if (fillOn)   std::copy(fillOn, fillOn + 3, fillOnColor);
        if (fillOff)  std::copy(fillOff, fillOff + 3, fillOffColor);
    }

    void draw() const
    {
        // Draw border
        glColor3fv(borderColor);
        glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y);
        glEnd();

        // Fill box
        glColor3fv(state ? fillOnColor : fillOffColor);
        glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y);
        glEnd();

        // Draw label
        glColor3fv(labelColor);
        framework::drawString8(label, x + LABEL_OFFSET_X, y + LABEL_OFFSET_Y);
    }

    void setState(bool newState)
    {
        if (state != newState)
        {
            state = newState;
            if (onToggle) onToggle(state);
        }
    }

    bool getState() const { return state; }

    void toggle() { setState(!state); }

    bool contains(int mx, int my) const
    {
        return mx >= x && mx <= x + BOX_WIDTH &&
               my >= y && my <= y + BOX_HEIGHT;
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

    int getX() const { return x; }
    int getY() const { return y; }
};

#endif /* TICKBOX_H_ */
