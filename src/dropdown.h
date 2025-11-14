/*
 * dropdown.h
 */

#ifndef DROPDOWN_H_
#define DROPDOWN_H_

#include <vector>
#include <string>
#include <GL/freeglut.h>

class Dropdown
{
public:
    float x, y, width, height;
    bool isOpen;

    Dropdown(float x, float y, float w, float h, const std::vector<std::string>& opts);

    void draw(int winW, int winH);
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

private:
    std::vector<std::string> options_;
    int selectedIndex_;
    int scrollOffset_;
};
#endif // DROPDOWN_H_
