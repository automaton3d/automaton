/*
 * dropdown.cpp
 */

#include "dropdown.h"
#include <algorithm>
#include <iostream>

Dropdown::Dropdown(float x_, float y_, float w_, float h_,
                   const std::vector<std::string>& opts,
                   const std::string& header)
    : x(x_), y(y_), width(w_), height(h_), isOpen_(false),
      options_(opts), selectedIndex_(0), scrollOffset_(0),
      header_(header), selectionChanged_(false), hoverIndex_(-1)
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

/*
bool Dropdown::containsDropdown(int mx, int my, int winW, int winH) const
{
    if (!isOpen_) return false;
    float top    = y + height;
    float bottom = y + height * 7;  // Changed from 6 to 7 to match VISIBLE
    return mx >= x && mx <= x + width && my >= top && my <= bottom;
}
*/

bool Dropdown::containsDropdown(int mx, int my, int winW, int winH) const
{
    if (!isOpen_) return false;

    // ✅ FIXED: Calculate actual visible item count
    const int VISIBLE = 6;
    int actualItemCount = std::min((int)options_.size(), VISIBLE);

    float top    = y + height;
    float bottom = y + height + height * actualItemCount;  // ✅ Use actualItemCount

    return mx >= x && mx <= x + width && my >= top && my <= bottom;
}

int Dropdown::getItemIndexAt(int mx, int my, int winW, int winH) const
{
    if (!isOpen_) return -1;

    float curY = y + height;
    const int VISIBLE = 6;

    // ✅ Calculate actual visible items
    int end = std::min((int)options_.size(), scrollOffset_ + VISIBLE);
    int actualEnd = std::min((int)options_.size(), end);

    for (int i = scrollOffset_; i < actualEnd; ++i)
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
           (isOpen_ && containsDropdown(mx, my, winW, winH));
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
        isOpen_ = !isOpen_;
        selectionChanged_ = false;
        if (!isOpen_) hoverIndex_ = -1;  // Clear hover when closing
        return false;
    }

    if (isOpen_ && containsDropdown(mx, my, winW, winH))
    {
        int idx = getItemIndexAt(mx, my, winW, winH);
        if (idx >= 0 && idx < (int)options_.size())
        {
            selectedIndex_ = idx;
            isOpen_ = false;
            selectionChanged_ = true;
            hoverIndex_ = -1;  // Clear hover on selection
            return true;
        }
    }
    return false;
}

bool Dropdown::wasJustSelected()
{
    if (selectionChanged_)
    {
        selectionChanged_ = false;
        return true;
    }
    return false;
}

void Dropdown::updateHover(int mx, int my, int winW, int winH)
{
    if (!isOpen_)
    {
        hoverIndex_ = -1;
        return;
    }

    hoverIndex_ = getItemIndexAt(mx, my, winW, winH);
}

void Dropdown::clearHover()
{
    hoverIndex_ = -1;
}

void Dropdown::clearSelection()
{
    selectedIndex_ = -1;
    isOpen_ = false;
    hoverIndex_ = -1;
}

void Dropdown::draw(int winW, int winH)
{
    // Draw header
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

    // Display header text if available, otherwise selected item
    std::string txt = header_.empty() ? getSelectedItem() : header_;
    glColor3f(0.f, 0.f, 0.f);
    glRasterPos2f(x + 6, y + height * 0.5f + 4);
    for (char c : txt) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);

    // Draw arrow (only if this is a regular dropdown, not a menu)
    if (header_.empty())
    {
        float ax = x + width - 10;
        float ay = y + height * 0.5f;

        glBegin(GL_TRIANGLES);
            if (isOpen_)
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
    }

    if (!isOpen_ || options_.empty()) return;

    const int VISIBLE = 6;
    float curY = y + height;

    // ✅ FIXED: Calculate actual number of items to display
    int start = scrollOffset_;
    int end = std::min((int)options_.size(), start + VISIBLE);
    int actualItemCount = end - start;  // Actual items being shown

    // Draw dropdown background - adjusted to actual item count
    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
        glVertex2f(x, curY);
        glVertex2f(x + width, curY);
        glVertex2f(x + width, curY + height * actualItemCount);  // ✅ Use actualItemCount
        glVertex2f(x, curY + height * actualItemCount);          // ✅ Use actualItemCount
    glEnd();

    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, curY);
        glVertex2f(x + width, curY);
        glVertex2f(x + width, curY + height * actualItemCount);  // ✅ Use actualItemCount
        glVertex2f(x, curY + height * actualItemCount);          // ✅ Use actualItemCount
    glEnd();

    // Draw items
    for (int i = start; i < end; ++i)
    {
        bool isSelected = (i == selectedIndex_);
        bool isHovered = (i == hoverIndex_);

        if (isSelected)
        {
            glColor3f(0.68f, 0.85f, 0.90f);
        }
        else if (isHovered)
        {
            glColor3f(0.95f, 0.95f, 0.95f);
        }
        else
        {
            glColor3f(1.f, 1.f, 1.f);
        }

        if (isSelected || isHovered)
        {
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

    // Draw scrollbar if needed
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

void Dropdown::open() { isOpen_ = true; }
void Dropdown::close() { isOpen_ = false; hoverIndex_ = -1; }
void Dropdown::toggle() { isOpen_ = !isOpen_; if (!isOpen_) hoverIndex_ = -1; }
