/*
 * tickbox.cpp
 *
 * Implements the Tickbox class, a simple GUI component for boolean options.
 */

#include <iostream>

#include "tickbox.h"
#include "projection_manager.h"
#include "draw_utils.h"

// Definição das variáveis estáticas
glm::vec3 Tickbox::borderColor_  = glm::vec3(1.0f);
glm::vec3 Tickbox::labelColor_   = glm::vec3(0.0f);
glm::vec3 Tickbox::fillOn_       = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 Tickbox::fillOff_      = glm::vec3(0.55f);
float Tickbox::fontScale_        = 0.35f;

// ============================================================================
// setColors
// ============================================================================

void Tickbox::setColors(const glm::vec3& border,
                        const glm::vec3& labelC,
                        const glm::vec3& fillOn,
                        const glm::vec3& fillOff)
{
    borderColor_ = border;
    labelColor_  = labelC;
    fillOn_      = fillOn;
    fillOff_     = fillOff;
}

// ============================================================================
// draw
// ============================================================================

void Tickbox::draw(TextRenderer& renderer) const
{
    const glm::mat4& P =
        ProjectionManager::instance().get2DOrtho();

    int screenW = gViewport[2];
    int screenH = gViewport[3];

    float dynamicScale = fontScale_;

    float boxY = y_;

    // ------------------------------------------------------------------------
    // Fill
    // ------------------------------------------------------------------------

    drawQuad2D(
        x_,
        boxY,
        x_ + BOX_SIZE,
        boxY + BOX_SIZE,
        state_ ? fillOn_ : fillOff_,
        P
    );

    // ------------------------------------------------------------------------
    // Border
    // ------------------------------------------------------------------------

    std::vector<glm::vec2> border = {
        {x_,                boxY},
        {x_ + BOX_SIZE,     boxY},
        {x_ + BOX_SIZE,     boxY + BOX_SIZE},
        {x_,                boxY + BOX_SIZE}
    };

    drawLineLoop2D(
        border,
        borderColor_,
        P,
        2.0f
    );

    // ------------------------------------------------------------------------
    // Check mark
    // ------------------------------------------------------------------------

    if (state_)
    {
        float m = 4.0f;

        glm::vec2 p1(
            x_ + m,
            boxY + BOX_SIZE * 0.5f
        );

        glm::vec2 p2(
            x_ + BOX_SIZE * 0.5f,
            boxY + BOX_SIZE - m
        );

        glm::vec2 p3(
            x_ + BOX_SIZE - m,
            boxY + m
        );

        drawLine2D_new(
            p1.x, p1.y,
            p2.x, p2.y,
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            P
        );

        drawLine2D_new(
            p2.x, p2.y,
            p3.x, p3.y,
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            P
        );
    }

    // ------------------------------------------------------------------------
    // Label
    // ------------------------------------------------------------------------

    float boxCenterY =
        y_ + BOX_SIZE * 0.5f;

    float textBaseline =
        screenH - boxCenterY - 5.0f;

    renderer.RenderText(
        label_,
        x_ + BOX_SIZE + 8,
        textBaseline,
        dynamicScale,
        labelColor_,
        screenW,
        screenH
    );
}

// ============================================================================
// contains
// ============================================================================

bool Tickbox::contains(int mx, int my) const
{
    return mx >= x_ &&
           mx <= x_ + BOX_SIZE &&
           my >= y_ &&
           my <= y_ + BOX_SIZE;
}

// ============================================================================
// onClick
// ============================================================================

void Tickbox::onClick(int mx, int my)
{
    if (contains(mx, my))
    {
        state_ = !state_;

        if (onToggle)
            onToggle(state_);
    }
}

// ============================================================================
// setState
// ============================================================================

void Tickbox::setState(bool s)
{
    if (state_ != s)
    {
        state_ = s;

        if (onToggle)
            onToggle(s);
    }
}

// ============================================================================
// Helpers
// ============================================================================

bool Tickbox::getState() const
{
    return state_;
}

void Tickbox::toggle()
{
    setState(!state_);
}