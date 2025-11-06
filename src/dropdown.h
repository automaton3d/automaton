// dropdown.h
#ifndef DROPDOWN_H_
#define DROPDOWN_H_

#include <vector>
#include <string>
#include <GL/freeglut.h>

class Dropdown
{
public:
    float x, y, width, height;
    std::vector<std::string> options;
    int selectedIndex;
    bool isOpen;
    int scrollOffset;

    Dropdown(float x, float y, float w, float h, const std::vector<std::string>& opts);

    void draw(int winW, int winH);
    bool handleClick(int mx, int my, int winW, int winH);
    void scroll(int direction);
    void close();

    bool containsHeader(int mx, int my, int winW, int winH) const;
    bool containsDropdown(int mx, int my, int winW, int winH) const;
    int getItemIndexAt(int mx, int my, int winW, int winH) const;

    std::string getSelectedItem() const;
    void selectByValue(const std::string& value);
};

#endif // DROPDOWN_H_
