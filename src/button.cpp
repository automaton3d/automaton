#include "button.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "globals.h"

Button::Button(float x, float y, float w, float h, const std::string& label, bool isDefault)
    : x_(x), y_(y), w_(w), h_(h), label_(label), isDefault_(isDefault) 
{
    setupGeometry();
}

Button::~Button() {
    cleanup();
}

void Button::setupGeometry() {
    // Shadow geometry (offset by 2 pixels down-right)
    float shadowVertices[] = {
        x_ + 2.0f, y_ - 2.0f,
        x_ + w_ + 2.0f, y_ - 2.0f,
        x_ + w_ + 2.0f, y_ + h_ - 2.0f,
        x_ + 2.0f, y_ + h_ - 2.0f
    };
    glGenVertexArrays(1, &shadowVAO);
    glGenBuffers(1, &shadowVBO);
    glBindVertexArray(shadowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shadowVertices), shadowVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Background geometry
    float bgVertices[] = {
        x_, y_,
        x_ + w_, y_,
        x_ + w_, y_ + h_,
        x_, y_ + h_
    };
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Border geometry (same as background)
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);
    glBindVertexArray(borderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Button::cleanup() {
    if (shadowVAO) glDeleteVertexArrays(1, &shadowVAO);
    if (shadowVBO) glDeleteBuffers(1, &shadowVBO);
    if (bgVAO) glDeleteVertexArrays(1, &bgVAO);
    if (bgVBO) glDeleteBuffers(1, &bgVBO);
    if (borderVAO) glDeleteVertexArrays(1, &borderVAO);
    if (borderVBO) glDeleteBuffers(1, &borderVBO);
if (underlineVAO) glDeleteVertexArrays(1, &underlineVAO);
if (underlineVBO) glDeleteBuffers(1, &underlineVBO);
}

bool Button::contains(int mouseX, int mouseY) const {
    return mouseX >= x_ && mouseX <= x_ + w_ &&
           mouseY >= y_ && mouseY <= y_ + h_;
}

void Button::draw(unsigned int shaderProgram, TextRenderer& renderer,
                  int screenWidth, int screenHeight)
{
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(projection));

    // Shadow
    glUniform3f(colorColorLoc2D, 0.1f, 0.1f, 0.1f);
    glBindVertexArray(shadowVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Background
    glUniform3f(colorColorLoc2D,
                isDefault_ ? 0.3f : 0.2f,
                isDefault_ ? 0.7f : 0.6f,
                isDefault_ ? 0.9f : 0.8f);
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Border
    glUniform3f(colorColorLoc2D, 1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBindVertexArray(borderVAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);

    // Centered text
    float textScale = 0.35f;  // Adjusted scale
    float textWidth = renderer.measureTextWidth(label_, textScale);
    float textX = x_ + (w_ - textWidth) * 0.5f;
    float textY = y_ + (h_ - 16.0f * textScale) * 0.5f - 3.0f;  // Lower 3 pixels
    renderer.RenderText(label_, textX, textY, textScale,
                        glm::vec3(1.0f), screenWidth, screenHeight);
}

void Button::drawAsHyperlink(TextRenderer& renderer, bool hovered,
                              int screenWidth, int screenHeight)
{
    glm::vec3 color = hovered ? glm::vec3(0.1f, 0.6f, 1.0f)
                              : glm::vec3(0.0f, 0.3f, 1.0f);

    float textScale = 0.35f;
    float textWidth = renderer.measureTextWidth(label_, textScale);
    float tx = x_ + (w_ - textWidth) * 0.5f;
    float ty = y_ + h_ * 0.5f - 3.0f;

    renderer.RenderText(label_, tx, ty, textScale, color,
                        screenWidth, screenHeight);

    // Initialize underline geometry once
    if (!underlineInitialized) {
        float underlineVertices[] = {
            tx, ty - 1.0f,
            tx + textWidth, ty - 1.0f
        };

        glGenVertexArrays(1, &underlineVAO);
        glGenBuffers(1, &underlineVBO);
        glBindVertexArray(underlineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, underlineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(underlineVertices),
                     underlineVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        
        underlineInitialized = true;
    }

    // Draw the cached underline
    glUseProgram(colorProgram2D);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth,
                                      0.0f, (float)screenHeight);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorColorLoc2D, color.r, color.g, color.b);

    glBindVertexArray(underlineVAO);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);

    glUseProgram(0);
}