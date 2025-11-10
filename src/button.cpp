#include "button.h"
#include <cmath>

Button::Button(float x, float y, float w, float h, const char* label, CoordMode mode)
    : x_(x), y_(y), w_(w), h_(h), label_(label), mode_(mode) {}

bool Button::contains(int mouseX, int mouseY, int winW, int winH) const
{
    //float px = x_, py = y_, pw = w_, ph = h_;

    if (mode_ == NDC) {
        // Convert mouse from pixel (top-left) to NDC
        float ndcX = (float)mouseX / winW * 2.0f - 1.0f;
        float ndcY = 1.0f - (float)mouseY / winH * 2.0f;
        return ndcX >= x_ && ndcX <= x_ + w_ && ndcY >= y_ && ndcY <= y_ + h_;
    } else {
        // PIXEL mode: Y=0 at top
        return mouseX >= x_ && mouseX <= x_ + w_ &&
               mouseY >= y_ && mouseY <= y_ + h_;
    }
}

void Button::draw(bool isDefault) const
{
    //float px = x_, py = y_, pw = w_, ph = h_;

    if (mode_ == NDC) {
        // Already in NDC → draw directly
        drawInternal(x_, y_, w_, h_, isDefault);
    } else {
        // PIXEL → just draw
        drawInternal(x_, y_, w_, h_, isDefault);
    }
}

void Button::drawInternal(float px, float py, float pw, float ph, bool isDefault) const
{
    // Shadow
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(px + 0.02f, py - 0.02f);
        glVertex2f(px + pw + 0.02f, py - 0.02f);
        glVertex2f(px + pw + 0.02f, py + ph - 0.02f);
        glVertex2f(px + 0.02f, py + ph - 0.02f);
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
    int winW = glutGet(GLUT_WINDOW_WIDTH);
    float scale = (mode_ == NDC) ? (2.0f / winW) : 1.0f;
    float textNDC = textWidth * scale;

    float tx = px + (pw - textNDC) * 0.5f;
    float ty = py + ph * 0.5f - 0.02f;

    glRasterPos2f(tx, ty);
    for (const char* c = label_; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void Button::drawAsHyperlink(bool hovered) const
{
    glColor3f(hovered ? 0.1f : 0.0f, hovered ? 0.6f : 0.3f, 1.0f);

    int textWidth = getTextWidth(GLUT_BITMAP_HELVETICA_12);
    int winW = glutGet(GLUT_WINDOW_WIDTH);
    float scale = (mode_ == NDC) ? (2.0f / winW) : 1.0f;
    float textNDC = textWidth * scale;

    float tx = x_ + (w_ - textNDC) * 0.5f;
    float ty = y_ + 0.02f;

    glRasterPos2f(tx, ty);
    for (const char* c = label_; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glLineWidth(1.0f);
    glBegin(GL_LINES);
        glVertex2f(tx, ty - 0.015f);
        glVertex2f(tx + textNDC, ty - 0.015f);
    glEnd();
}

int Button::getTextWidth(void* font) const
{
    int w = 0;
    for (const char* c = label_; *c; ++c)
        w += glutBitmapWidth(font, *c);
    return w;
}
