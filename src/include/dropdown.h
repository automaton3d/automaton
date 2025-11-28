/*
 * dropdown.h (adapted)
 */

#ifndef DROPDOWN_H_
#define DROPDOWN_H_

#include <vector>
#include <string>
#include "text_renderer.h"

class Dropdown
{
public:
    float x, y, width, height;
    bool isOpen_;

    Dropdown(float x, float y, float w, float h,
             const std::vector<std::string>& opts,
             const std::string& header = "");

    void initGL();

    // ✅ Adapted: draw now uses TextRenderer
    void draw(TextRenderer& renderer, int winW, int winH);

    bool handleClick(int mx, int my, int winW, int winH);
    void scroll(int direction);
    void open();
    void close();
    void toggle();

    bool containsHeader(int mx, int my, int winW, int winH) const;
    bool containsDropdown(int mx, int my, int winW, int winH) const;
    int getItemIndexAt(int mx, int my, int winW, int winH) const;

    std::string getSelectedItem() const;
    void selectByValue(const std::string& value);
    int getSelectedIndex() const;
    bool isMouseOver(int mx, int my, int winW, int winH) const;
    void clearSelection();
    bool wasJustSelected();

    // Hover functionality
    void updateHover(int mx, int my, int winW, int winH);
    void clearHover();

private:
    std::vector<std::string> options_;
    int selectedIndex_;
    int scrollOffset_;
    std::string header_;
    bool selectionChanged_;
    int hoverIndex_;  // -1 means no hover
};

#endif // DROPDOWN_H_
