#ifndef BUTTON_H_
#define BUTTON_H_

#include <GL/freeglut.h>

class Button
{
public:
    enum CoordMode { NDC, PIXEL };

    Button(float x, float y, float w, float h, const char* label, CoordMode mode = NDC);

    bool contains(int mouseX, int mouseY, int winW = 0, int winH = 0) const;
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
    // TODO: float getHeight() const { return h; }

private:
    float x_, y_, w_, h_;
    const char* label_;
    CoordMode mode_;
    int getTextWidth(void* font) const;
    void drawInternal(float px, float py, float pw, float ph, bool isDefault) const;
};

#endif
