#ifndef BUTTON_H_
#define BUTTON_H_

#include <string>
#include "text_renderer.h"

class Button {
public:
    Button(float x, float y, float w, float h, const std::string& label, bool isDefault = false);
    ~Button();

    // Disable copy to prevent double-free
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;

    void draw(unsigned int shaderProgram, TextRenderer& renderer,
              int screenWidth, int screenHeight);

    void drawAsHyperlink(TextRenderer& renderer, bool hovered,
                         int screenWidth, int screenHeight);  // Removed const

    bool contains(int px, int py) const;
    void setDefault(bool value) { isDefault_ = value; }
    bool getDefault() const { return isDefault_; }
    const std::string& getLabel() const { return label_; }

private:
    void setupGeometry();
    void cleanup();

    float x_, y_, w_, h_;
    std::string label_;
    bool isDefault_;

    unsigned int shadowVAO{}, shadowVBO{};
    unsigned int bgVAO{}, bgVBO{};
    unsigned int borderVAO{}, borderVBO{};
    mutable unsigned int underlineVAO{};
    mutable unsigned int underlineVBO{};
    mutable bool underlineInitialized{false};

};

#endif