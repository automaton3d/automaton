// Fixed dropdown.cpp
#include "dropdown.h"
#include <algorithm>
#include <iostream>

Dropdown::Dropdown(float x_, float y_, float w_, float h_,
                   const std::vector<std::string>& opts,
                   const std::string& header)
    : x(x_), y(y_), width(w_), height(h_), isOpen(false),
      options_(opts), selectedIndex_(0), scrollOffset_(0),
      header_(header)
{
}

std::string Dropdown::getSelectedItem() const
{
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)options_.size())
        return options_[selectedIndex_];
    return options_.empty() ? "" : options_[0];
}

void Dropdown::selectByValue(const std::string& value)
{
    for (size_t i = 0; i < options_.size(); ++i)
        if (options_[i] == value) { selectedIndex_ = i; scrollOffset_ = 0; return; }
    if (!options_.empty()) selectedIndex_ = 0;
}

int Dropdown::getSelectedIndex() const { return selectedIndex_; }

bool Dropdown::containsHeader(int mx, int my, int winW, int winH) const
{
    return mx >= x && mx <= x + width &&
           my >= y && my <= y + height;
}

bool Dropdown::containsDropdown(int mx, int my, int winW, int winH) const
{
    if (!isOpen) return false;
    float top    = y + height;
    float bottom = y + height * 6;
    return mx >= x && mx <= x + width && my >= top && my <= bottom;
}

int Dropdown::getItemIndexAt(int mx, int my, int winW, int winH) const
{
    if (!isOpen) return -1;

    float curY = y + height;
    for (int i = scrollOffset_; i < (int)options_.size(); ++i)
    {
        float itemTop    = curY;
        float itemBottom = curY + height;
        if (my >= itemTop && my <= itemBottom)
            return i;
        curY += height;
    }
    return -1;
}

bool Dropdown::isMouseOver(int mx, int my, int winW, int winH) const
{
    return containsHeader(mx, my, winW, winH) ||
           (isOpen && containsDropdown(mx, my, winW, winH));
}

void Dropdown::scroll(int dir)
{
    const int VISIBLE = 6;
    int maxScroll = (int)options_.size() - VISIBLE;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset_ = std::clamp(scrollOffset_ + dir, 0, maxScroll);
}

bool Dropdown::handleClick(int mx, int my, int winW, int winH)
{
    if (containsHeader(mx, my, winW, winH))
    {
        isOpen = !isOpen;
        return false;
    }

    if (isOpen && containsDropdown(mx, my, winW, winH))
    {
        int idx = getItemIndexAt(mx, my, winW, winH);
        if (idx >= 0 && idx < (int)options_.size())
        {
            selectedIndex_ = idx;
            isOpen = false;
            return true;
        }
    }
    return false;
}

void Dropdown::draw(int winW, int winH)
{
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
    glColor3f(0.f, 0.f, 0.f);
    glRasterPos2f(x + 6, y + height * 0.5f + 4);
    for (char c : txt) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    float ax = x + width - 10;
    float ay = y + height * 0.5f;

    glBegin(GL_TRIANGLES);
        if (isOpen)
        {
            glVertex2f(ax - 0.01f, ay - 0.005f);
            glVertex2f(ax + 0.01f, ay - 0.005f);
            glVertex2f(ax,        ay + 0.005f);
        }
        else
        {
            glVertex2f(ax - 0.01f, ay + 0.005f);
            glVertex2f(ax + 0.01f, ay + 0.005f);
            glVertex2f(ax,        ay - 0.005f);
        }
    glEnd();

    if (!isOpen || options_.empty()) return;

    const int VISIBLE = 6;
    float curY = y + height;

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
        glVertex2f(x, curY);
        glVertex2f(x + width, curY);
        glVertex2f(x + width, curY + height * VISIBLE);
        glVertex2f(x, curY + height * VISIBLE);
    glEnd();

    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, curY);
        glVertex2f(x + width, curY);
        glVertex2f(x + width, curY + height * VISIBLE);
        glVertex2f(x, curY + height * VISIBLE);
    glEnd();

    int start = scrollOffset_;
    int end = std::min((int)options_.size(), start + VISIBLE);

    for (int i = start; i < end; ++i)
    {
        if (i == selectedIndex_)
        {
            glColor3f(0.68f, 0.85f, 0.90f);
            glBegin(GL_QUADS);
                glVertex2f(x, curY);
                glVertex2f(x + width, curY);
                glVertex2f(x + width, curY + height);
                glVertex2f(x, curY + height);
            glEnd();
        }

        glColor3f(0.f, 0.f, 0.f);
        glRasterPos2f(x + 6, curY + height * 0.5f + 4);
        for (char c : options_[i])
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        curY += height;
    }

    if ((int)options_.size() > VISIBLE)
    {
        float totalH   = height * options_.size();
        float visibleH = height * VISIBLE;
        float barH     = visibleH * visibleH / totalH;
        float ratio    = (float)scrollOffset_ / (options_.size() - VISIBLE);

        float barYTop    = y + height + (visibleH - barH) * (1 - ratio);
        float barYBottom = barYTop - barH;

        glColor3f(0.65f, 0.65f, 0.65f);
        glBegin(GL_QUADS);
            glVertex2f(x + width - 0.01f, barYTop);
            glVertex2f(x + width,         barYTop);
            glVertex2f(x + width,         barYBottom);
            glVertex2f(x + width - 0.01f, barYBottom);
        glEnd();
    }
}

void Dropdown::open() { isOpen = true; }
void Dropdown::close() { isOpen = false; }
void Dropdown::toggle() { isOpen = !isOpen; }
