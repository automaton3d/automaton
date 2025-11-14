/*
 * button.h
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <GL/freeglut.h>

class Button
{
public:
    Button(float x, float y, float w, float h, const char* label);

    bool contains(int mouseX, int mouseY) const;
    void draw(bool isDefault = false) const;
    void drawAsHyperlink(bool hovered) const;

    void setPosition(int x, int y)
    {
        x_ = x;
        y_ = y;
    }

    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return w_; }
    float getHeight() const { return h_; }

private:
    float x_, y_, w_, h_;
    const char* label_;
    int getTextWidth(void* font) const;
    void drawInternal(float px, float py, float pw, float ph, bool isDefault) const;
};

#endif
