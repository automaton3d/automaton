// dropdown.cpp
#include "dropdown.h"
#include <algorithm>
#include <iostream>

Dropdown::Dropdown(float x_, float y_, float w_, float h_,
                   const std::vector<std::string>& opts)
    : x(x_), y(y_), width(w_), height(h_), options(opts),
      selectedIndex(0), isOpen(false), scrollOffset(0)
{
}

/* ------------------------------------------------- */
std::string Dropdown::getSelectedItem() const
{
    if (selectedIndex >= 0 && selectedIndex < (int)options.size())
        return options[selectedIndex];
    return options.empty() ? "" : options[0];
}

void Dropdown::selectByValue(const std::string& value)
{
    for (size_t i = 0; i < options.size(); ++i)
        if (options[i] == value) { selectedIndex = i; scrollOffset = 0; return; }
    if (!options.empty()) selectedIndex = 0;
}

/* ------------------------------------------------- */
static float ndcX(int mx, int w) { return mx * 2.0f / w - 1.0f; }
static float ndcY(int my, int h) { return 1.0f - my * 2.0f / h; }

bool Dropdown::containsHeader(int mx, int my, int winW, int winH) const
{
    float nx = ndcX(mx, winW);
    float ny = ndcY(my, winH);
    return nx >= x && nx <= x + width && ny >= y && ny <= y + height;
}

/* List opens **below** the header */
bool Dropdown::containsDropdown(int mx, int my, int winW, int winH) const
{
    if (!isOpen) return false;
    float nx = ndcX(mx, winW);
    float ny = ndcY(my, winH);
    float top    = y;                 // top of header
    float bottom = y - height * 6;    // max 6 items visible
    return nx >= x && nx <= x + width && ny <= top && ny >= bottom;
}

int Dropdown::getItemIndexAt(int mx, int my, int winW, int winH) const
{
    if (!isOpen) return -1;
    float ny = ndcY(my, winH);
    float itemY = y;                         // first item at header top
    for (int i = scrollOffset; i < (int)options.size(); ++i)
    {
        float itemBottom = itemY - height;
        if (ny <= itemY && ny >= itemBottom)
            return i;
        itemY = itemBottom;
        if (itemY < y - height * 6) break;
    }
    return -1;
}

/* ------------------------------------------------- */
void Dropdown::scroll(int dir)
{
    const int VISIBLE = 6;
    int maxScroll = (int)options.size() - VISIBLE;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset += dir;
    scrollOffset = std::max(0, std::min(scrollOffset, maxScroll));
}

void Dropdown::close() { isOpen = false; }

/* ------------------------------------------------- */
bool Dropdown::handleClick(int mx, int my, int winW, int winH)
{
    if (containsHeader(mx, my, winW, winH))
    {
        isOpen = !isOpen;
        return false;               // just toggled
    }

    if (isOpen && containsDropdown(mx, my, winW, winH))
    {
        int idx = getItemIndexAt(mx, my, winW, winH);
        if (idx >= 0 && idx < (int)options.size())
        {
            selectedIndex = idx;
            isOpen = false;
            return true;            // item selected
        }
    }
    return false;
}

/* ------------------------------------------------- */
void Dropdown::draw(int winW, int winH)
{
    /* ---- HEADER ---- */
    glColor3f(0.92f, 0.92f, 0.92f);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();

    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();

    std::string txt = getSelectedItem();
    if (txt.empty() && !options.empty()) txt = options[0];
    glColor3f(0.f, 0.f, 0.f);
    glRasterPos2f(x + 0.01f, y + height * 0.5f - 0.008f);
    for (char c : txt) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    /* arrow */
    float ax = x + width - 0.03f;
    float ay = y + height * 0.5f;
    glBegin(GL_TRIANGLES);
        if (isOpen) {   // up
            glVertex2f(ax - 0.01f, ay - 0.005f);
            glVertex2f(ax + 0.01f, ay - 0.005f);
            glVertex2f(ax,        ay + 0.005f);
        } else {        // down
            glVertex2f(ax - 0.01f, ay + 0.005f);
            glVertex2f(ax + 0.01f, ay + 0.005f);
            glVertex2f(ax,        ay - 0.005f);
        }
    glEnd();

    /* ---- LIST (downward) ---- */
    if (!isOpen || options.empty()) return;

    const int VISIBLE = 6;
    float listTop    = y;                     // start right under header
    float listBottom = y - height * VISIBLE;

    /* background */
    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
        glVertex2f(x, listTop);
        glVertex2f(x + width, listTop);
        glVertex2f(x + width, listBottom);
        glVertex2f(x, listBottom);
    glEnd();

    /* border */
    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, listTop);
        glVertex2f(x + width, listTop);
        glVertex2f(x + width, listBottom);
        glVertex2f(x, listBottom);
    glEnd();

    /* items */
    int start = scrollOffset;
    int end   = std::min((int)options.size(), start + VISIBLE);
    float curY = listTop;

    for (int i = start; i < end; ++i)
    {
        if (i == selectedIndex)
        {
            glColor3f(0.68f, 0.85f, 0.90f);
            glBegin(GL_QUADS);
                glVertex2f(x, curY);
                glVertex2f(x + width, curY);
                glVertex2f(x + width, curY - height);
                glVertex2f(x, curY - height);
            glEnd();
        }

        glColor3f(0.f, 0.f, 0.f);
        glRasterPos2f(x + 0.01f, curY - height * 0.5f - 0.006f);
        for (char c : options[i])
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

        curY -= height;
    }

    /* scrollbar (optional) */
    if ((int)options.size() > VISIBLE)
    {
    float totalH   = height * options.size();
    float visibleH = height * VISIBLE;
    float barH     = visibleH * visibleH / totalH;
    float ratio    = (float)scrollOffset / (options.size() - VISIBLE);
    // ✅ Correct top and bottom coordinates
    float barYTop    = listTop - (visibleH - barH) * ratio;
    float barYBottom = barYTop - barH;

    glColor3f(0.65f, 0.65f, 0.65f);
    glBegin(GL_QUADS);
        glVertex2f(x + width - 0.01f, barYTop);
        glVertex2f(x + width,          barYTop);
        glVertex2f(x + width,          barYBottom);
        glVertex2f(x + width - 0.01f, barYBottom);
    glEnd();
    }
}
