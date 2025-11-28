#ifndef HYPERLINK_H_
#define HYPERLINK_H_

#include <string>
#include "text_renderer.h"
#include "draw_utils.h"   // helpers para underline

class Hyperlink {
public:
    Hyperlink(float x, float y, float w, float h, const std::string& label)
        : x_(x), y_(y), w_(w), h_(h), label_(label) {}

    void initGL();

    // Desenha como hyperlink (texto + sublinhado)
    void drawAsHyperlink(TextRenderer& renderer, bool hovered,
                         int screenWidth, int screenHeight)
    {
        glm::vec3 color = hovered ? glm::vec3(0.1f, 0.6f, 1.0f)
                                  : glm::vec3(0.0f, 0.3f, 1.0f);

        // Centralizar texto horizontalmente
        float textWidth = renderer.measureTextWidth(label_, 1.0f);
        float tx = x_ + (w_ - textWidth) * 0.5f;
        float ty = y_ + h_ * 0.5f;

        // Renderizar texto
        renderer.RenderText(label_, tx, ty, 1.0f, color,
                            screenWidth, screenHeight);

        // Sublinhado
        drawLineLoop2D({{tx, ty+2.0f}, {tx+textWidth, ty+2.0f}},
                                  color, screenWidth, screenHeight, 1.0f);
    }

    // Verifica se o mouse está sobre o hyperlink
    bool contains(int mx, int my) const {
        return mx >= x_ && mx <= x_ + w_ &&
               my >= y_ && my <= y_ + h_;
    }

private:
    float x_, y_, w_, h_;
    std::string label_;
};

#endif /* HYPERLINK_H_ */
