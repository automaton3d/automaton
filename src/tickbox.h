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

public:
    std::function<void(bool)> onToggle;  // Optional callback

    Tickbox(int x, int y, const std::string& label)
        : state(false), label(label), x(x), y(y) {}

    void draw() const
    {
        // Draw border
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y);
        glEnd();

        // Fill box
        glColor3f(state ? 0.0f : 0.5f, state ? 1.0f : 0.5f, state ? 0.0f : 0.5f);
        glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y + BOX_HEIGHT);
        glVertex2i(x + BOX_WIDTH, y);
        glEnd();

        // Draw label
        glColor3f(1.0f, 1.0f, 1.0f);
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

    void toggle()
    {
        setState(!state);
    }

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
