/*
 * button.cpp
 */

#include "button.h"
#include <cmath>

Button::Button(float x, float y, float w, float h, const char* label)
    : x_(x), y_(y), w_(w), h_(h), label_(label) {}

bool Button::contains(int mouseX, int mouseY) const
{
    // Top-origin: (x_, y_) is top-left, y increases downward
    return mouseX >= x_ && mouseX <= x_ + w_ &&
           mouseY >= y_ && mouseY <= y_ + h_;
}

void Button::draw(bool isDefault) const
{
    drawInternal(x_, y_, w_, h_, isDefault);
}

void Button::drawInternal(float px, float py, float pw, float ph, bool isDefault) const
{
    // Shadow (shifted down/right in top-origin)
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(px + 2, py + 2);
        glVertex2f(px + pw + 2, py + 2);
        glVertex2f(px + pw + 2, py + ph + 2);
        glVertex2f(px + 2, py + ph + 2);
    glEnd();

    // Background
    glColor3f(isDefault ? 0.25f : 0.22f,
              isDefault ? 0.60f : 0.50f,
              isDefault ? 0.95f : 0.80f);
    glBegin(GL_QUADS);
        glVertex2f(px, py);
        glVertex2f(px + pw, py);
        glVertex2f(px + pw, py + ph);
        glVertex2f(px, py + ph);
    glEnd();

    // Text
    glColor3f(1.0f, 1.0f, 1.0f);
    int textWidth = getTextWidth(GLUT_BITMAP_HELVETICA_18);

    float tx = px + (pw - textWidth) * 0.5f;
    float ty = py + ph * 0.5f + 6; // centered in top-origin

    glRasterPos2f(tx, ty);
    for (const char* c = label_; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void Button::drawAsHyperlink(bool hovered) const
{
    glColor3f(hovered ? 0.1f : 0.0f, hovered ? 0.6f : 0.3f, 1.0f);

    int textWidth = getTextWidth(GLUT_BITMAP_HELVETICA_12);

    float tx = x_ + (w_ - textWidth) * 0.5f;
    float ty = y_ + h_ * 0.5f + 4; // adjust for top-origin

    glRasterPos2f(tx, ty);
    for (const char* c = label_; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // underline
    glLineWidth(1.0f);
    glBegin(GL_LINES);
        glVertex2f(tx, ty + 2);
        glVertex2f(tx + textWidth, ty + 2);
    glEnd();
}

int Button::getTextWidth(void* font) const
{
    int w = 0;
    for (const char* c = label_; *c; ++c)
        w += glutBitmapWidth(font, *c);
    return w;
}
