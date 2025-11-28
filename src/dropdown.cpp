/*
 * dropdown.cpp (FIXED: Items now spread downward visually)
 */

#include "dropdown.h"
#include <algorithm>
#include <iostream>
#include "text_renderer.h"
#include "draw_utils.h"

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

bool Dropdown::containsDropdown(int mx, int my, int winW, int winH) const
{
    if (!isOpen_) return false;

    // FIXED: Items spread downward (decreasing Y in OpenGL coords)
    const int VISIBLE = 6;
    int actualItemCount = std::min((int)options_.size(), VISIBLE);

    float top    = y;  // Top of dropdown = bottom of header
    float bottom = y - height * actualItemCount;  // FIXED: Subtract to go down

    return mx >= x && mx <= x + width && my >= bottom && my <= top;
}

int Dropdown::getItemIndexAt(int mx, int my, int winW, int winH) const
{
    if (!isOpen_) return -1;

    float curY = y;  // FIXED: Start at header bottom
    const int VISIBLE = 6;

    int end = std::min((int)options_.size(), scrollOffset_ + VISIBLE);
    int actualEnd = std::min((int)options_.size(), end);

    for (int i = scrollOffset_; i < actualEnd; ++i)
    {
        float itemTop    = curY;
        float itemBottom = curY - height;  // FIXED: Subtract to go down
        if (my >= itemBottom && my <= itemTop)
            return i;
        curY -= height;  // FIXED: Subtract to move down
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
        if (!isOpen_) hoverIndex_ = -1;
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
            hoverIndex_ = -1;
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

void Dropdown::draw(TextRenderer& renderer, int winW, int winH)
{
    // Header background
    drawQuad2D(x, y, x+width, y+height,
               glm::vec3(0.92f,0.92f,0.92f), winW, winH);
    drawLineLoop2D({{x,y},{x+width,y},{x+width,y+height},{x,y+height}},
                   glm::vec3(0.35f), winW, winH);

    // Header text
    std::string txt = header_.empty() ? getSelectedItem() : header_;
    renderer.RenderText(txt, x+6, y+height*0.5f-6, 0.3f,
                        glm::vec3(0.f), winW, winH);

    // Arrow
    if (header_.empty()) {
        float ax = x + width - 10;
        float ay = y + height * 0.5f;
        if (isOpen_) {
            drawTriangle2D({{ax-0.01f,ay+0.005f},
                           {ax+0.01f,ay+0.005f},
                           {ax,ay-0.005f}},
                           glm::vec3(0.f), winW, winH);
        } else {
            drawTriangle2D({{ax-0.01f,ay-0.005f},
                           {ax+0.01f,ay-0.005f},
                           {ax,ay+0.005f}},
                           glm::vec3(0.f), winW, winH);
        }
    }

    if (!isOpen_ || options_.empty()) return;

    const int VISIBLE=6;
    float curY=y;  // FIXED: Start at header bottom
    int start=scrollOffset_;
    int end=std::min((int)options_.size(), start+VISIBLE);
    int actualItemCount=end-start;

    // FIXED: Dropdown background (spreading downward)
    float dropdownTop = y;
    float dropdownBottom = y - height * actualItemCount;
    drawQuad2D(x, dropdownBottom, x+width, dropdownTop,
               glm::vec3(1.f), winW, winH);
    drawLineLoop2D({{x,dropdownBottom},{x+width,dropdownBottom},
                    {x+width,dropdownTop},{x,dropdownTop}},
                   glm::vec3(0.35f), winW, winH);

    // Items (FIXED: spreading downward)
    for (int i=start;i<end;++i) {
        bool isSelected=(i==selectedIndex_);
        bool isHovered=(i==hoverIndex_);
        glm::vec3 bgColor=glm::vec3(1.f);
        if (isSelected) bgColor=glm::vec3(0.68f,0.85f,0.90f);
        else if (isHovered) bgColor=glm::vec3(0.95f);

        float itemTop = curY;
        float itemBottom = curY - height;

        if (isSelected||isHovered)
            drawQuad2D(x, itemBottom, x+width, itemTop, bgColor, winW, winH);

        renderer.RenderText(options_[i], x+6, itemBottom+height*0.5f-6, 0.3f,
                            glm::vec3(0.f), winW, winH);
        curY -= height;  // FIXED: Subtract to move down
    }

    // Scrollbar (FIXED: positioned correctly for downward spread)
    if ((int)options_.size()>VISIBLE) {
        float totalH=height*options_.size();
        float visibleH=height*VISIBLE;
        float barH=visibleH*visibleH/totalH;
        float ratio=(float)scrollOffset_/(options_.size()-VISIBLE);
        float barYTop = y - (visibleH-barH)*ratio;
        float barYBottom = barYTop - barH;
        drawQuad2D(x+width-0.01f, barYBottom, x+width, barYTop,
                   glm::vec3(0.65f), winW, winH);
    }
}

void Dropdown::open() { isOpen_ = true; }
void Dropdown::close() { isOpen_ = false; hoverIndex_ = -1; }
void Dropdown::toggle() { isOpen_ = !isOpen_; if (!isOpen_) hoverIndex_ = -1; }