#ifndef RADIO_H_
#define RADIO_H_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <string>
#include <vector>
#include "text_renderer.h"
#include "draw_utils.h"

class Radio
{
private:
    bool selected_;
    std::string label_;
    int x_, y_;
    static constexpr int RADIO_HEIGHT_ = 15;
    static constexpr float RADIO_RADIUS = 8.0f;
    static constexpr float PI = 3.14159265359f;
    float fontScale_ = 1.0f;

public:
    Radio(int x, int y, std::string label)
        : selected_(false), label_(std::move(label)), x_(x), y_(y) {}

    // Default constructor for container initialization
    Radio()
        : selected_(false), label_(""), x_(0), y_(0) {}

    void setFontScale(float s) { fontScale_ = s; }

    void draw(TextRenderer& renderer, int screenWidth, int screenHeight) 
    {
        drawAt(renderer, x_, y_, screenWidth, screenHeight);
    }

    void drawAt(TextRenderer& renderer, int xPos, int yPos,
                int screenWidth, int screenHeight)
    {
        // Outer circle (line loop)
        std::vector<glm::vec2> circleVerts;
        circleVerts.reserve(360);
        for (int i = 0; i < 360; ++i) {
            float angle = i * (PI / 180.0f);
            circleVerts.push_back({
                xPos + RADIO_RADIUS * std::cos(angle), 
                yPos + RADIO_RADIUS * std::sin(angle)
            });
        }
        drawLineLoop2D(circleVerts, glm::vec3(1.0f), screenWidth, screenHeight, 1.0f);

        // Filled circle if selected
        if (selected_) {
            std::vector<glm::vec2> fanVerts;
            fanVerts.reserve(362); // center + 361 points
            fanVerts.push_back({static_cast<float>(xPos), static_cast<float>(yPos)});
            for (int i = 0; i <= 360; ++i) {
                float angle = i * (PI / 180.0f);
                fanVerts.push_back({
                    xPos + RADIO_RADIUS * std::cos(angle), 
                    yPos + RADIO_RADIUS * std::sin(angle)
                });
            }
            drawTriangleFan2D(fanVerts, glm::vec3(0.0f, 1.0f, 0.0f), screenWidth, screenHeight);
        }

        // Center dot (small quad for visibility)
        drawQuad2D(xPos - 1, yPos - 1, xPos + 1, yPos + 1, 
                   glm::vec3(1.0f), screenWidth, screenHeight);

        // Label text (to the right of the circle)
        renderer.RenderText(label_,
                    xPos + 18, yPos - 7,
                    fontScale_,
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    screenWidth, screenHeight);
    }

    // State management
    void setSelected(bool isSelected) { selected_ = isSelected; }
    bool isSelected() const { return selected_; }
    void toggle() { selected_ = !selected_; }

    // Hit testing (for mouse clicks)
    bool clicked(double xpos, double ypos, int windowHeight) const 
    {
        // Convert from top-origin to bottom-origin
        double bottomY = windowHeight - ypos;
        return clickedAt(xpos, bottomY, x_, y_);
    }

    bool clickedAt(double xpos, double ypos, int drawX, int drawY) const 
    {
        // Hit test area: radio circle + label text area
        // Horizontal: from circle left edge to ~100 pixels right
        // Vertical: centered around drawY with some margin
        return xpos >= drawX - RADIO_RADIUS - 2 && xpos <= drawX + 100 &&
               ypos >= drawY - RADIO_RADIUS - 1 && ypos <= drawY + RADIO_RADIUS + 1;
    }

    // Getters
    int getX() const { return x_; }
    int getY() const { return y_; }
    const std::string& getLabel() const { return label_; }

    // Setters
    void setPosition(int x, int y) { x_ = x; y_ = y; }
    void setLabel(const std::string& label) { label_ = label; }

    // Static helper to manage radio button groups
    static void selectOne(std::vector<Radio>& group, size_t index)
    {
        if (index >= group.size()) return;
        for (auto& radio : group) {
            radio.setSelected(false);
        }
        group[index].setSelected(true);
    }

    static int getSelectedIndex(const std::vector<Radio>& group)
    {
        for (size_t i = 0; i < group.size(); ++i) {
            if (group[i].isSelected()) return static_cast<int>(i);
        }
        return -1; // None selected
    }
};

#endif /* RADIO_H_ */