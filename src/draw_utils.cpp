// draw_utils.cpp
// Modern, fast 2D + 3D immediate-mode drawing – zero allocations per frame
// Fully core-profile safe, used by HUD, debug, and tomography

#include "draw_utils.h"
#include "projection_manager.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <sstream>

// External shader handles (must be initialized elsewhere)
extern GLuint colorProgram2D;
extern GLint  colorMvpLoc2D;
extern GLint  colorColorLoc2D;

extern GLuint colorProgram3D;
extern GLint  colorMvpLoc3D;
extern GLint  colorColorLoc3D;

#include <sstream>

std::vector<std::string> wrapText(const std::string& text, size_t width)
{
    std::istringstream words(text);
    std::string word;
    std::vector<std::string> lines;
    std::string current;

    while (words >> word) {
        if (current.size() + word.size() + 1 > width) {
            lines.push_back(current);
            current = word;
        } else {
            if (!current.empty()) current += " ";
            current += word;
        }
    }
    if (!current.empty()) lines.push_back(current);

    return lines;
}

// ===================================================================
// 2D Renderer (UI, HUD, overlays)
// ===================================================================
namespace Draw2D
{
    static GLuint VAO = 0;
    static GLuint VBO = 0;
    static const size_t MAX_VERTICES = 131072;
    static bool initialized = false;

    static void init()
    {
        if (initialized) return;

        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);

        glNamedBufferData(VBO, MAX_VERTICES * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, 0);
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(glm::vec2));

        initialized = true;
    }

    void shutdown()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        VAO = VBO = 0;
        initialized = false;
    }

    static void begin(const glm::mat4& proj, const glm::vec3& color)
    {
        init();
        glUseProgram(colorProgram2D);
        glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, &proj[0][0]);
        glUniform3fv(colorColorLoc2D, 1, &color[0]);
        glBindVertexArray(VAO);
    }

    static void upload(const void* data, size_t vertexCount)
    {
        glNamedBufferSubData(VBO, 0, vertexCount * sizeof(glm::vec2), data);
    }

    static void end()
    {
        glBindVertexArray(0);
        // Do NOT unbind program here – other systems may use it
    }
}

// ===================================================================
// 3D Renderer (debug lines, tomo plane, points)
// ===================================================================
namespace Draw3D
{
    static GLuint VAO = 0;
    static GLuint VBO = 0;
    static const size_t MAX_VERTICES = 1048576; // 1M vec3s
    static bool initialized = false;

    static void init()
    {
        if (initialized) return;

        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glNamedBufferData(VBO, MAX_VERTICES * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, 0);
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(glm::vec3));

        initialized = true;
    }

    void shutdown()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        VAO = VBO = 0;
        initialized = false;
    }

    static void begin(const glm::mat4& mvp, const glm::vec3& color)
    {
        init();
        glUseProgram(colorProgram3D);
        glUniformMatrix4fv(colorMvpLoc3D, 1, GL_FALSE, &mvp[0][0]);
        glUniform3fv(colorColorLoc3D, 1, &color[0]);
        glBindVertexArray(VAO);
    }

    static void upload(const void* data, size_t vertexCount)
    {
        glNamedBufferSubData(VBO, 0, vertexCount * sizeof(glm::vec3), data);
    }

    static void end()
    {
        glBindVertexArray(0);
    }
}

// ===================================================================
// 2D PRIMITIVES
// ===================================================================

void drawQuad2D(float x1, float y1, float x2, float y2,
                const glm::vec3& color,
                const glm::mat4& proj)
{
    const glm::vec2 verts[] = {
        {x1, y1}, {x2, y1},
        {x2, y2}, {x1, y2}
    };

    Draw2D::begin(proj, color);
    Draw2D::upload(verts, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    Draw2D::end();
}

void drawLineLoop2D(const std::vector<glm::vec2>& points,
                    const glm::vec3& color,
                    const glm::mat4& proj,
                    float thickness)
{
    if (points.size() < 2) return;
    Draw2D::begin(proj, color);
    glLineWidth(std::max(1.0f, thickness));
    Draw2D::upload(points.data(), points.size());
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(points.size()));
    Draw2D::end();
}

void drawTriangleFan2D(const std::vector<glm::vec2>& points,
                       const glm::vec3& color,
                       const glm::mat4& proj)
{
    if (points.size() < 3) return;
    Draw2D::begin(proj, color);
    Draw2D::upload(points.data(), points.size());
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(points.size()));
    Draw2D::end();
}

void drawLine2D(float x1, float y1, float x2, float y2,
                const glm::vec3& color,
                const glm::mat4& proj,
                float thickness)
{
    const glm::vec2 verts[] = { {x1,y1}, {x2,y2} };
    Draw2D::begin(proj, color);
    glLineWidth(std::max(1.0f, thickness));
    Draw2D::upload(verts, 2);
    glDrawArrays(GL_LINES, 0, 2);
    Draw2D::end();
}

void drawThickLine2D(float x1, float y1, float x2, float y2,
                     float thickness,
                     const glm::vec3& color,
                     const glm::mat4& proj)
{
    if (thickness <= 0.0f) return;

    glm::vec2 dir = glm::normalize(glm::vec2(x2 - x1, y2 - y1));
    glm::vec2 perp(-dir.y, dir.x);
    float h = thickness * 0.5f;

    const glm::vec2 verts[] = {
        glm::vec2(x1, y1) + perp * h,
        glm::vec2(x1, y1) - perp * h,
        glm::vec2(x2, y2) - perp * h,
        glm::vec2(x2, y2) + perp * h
    };

    Draw2D::begin(proj, color);
    Draw2D::upload(verts, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    Draw2D::end();
}

// ===================================================================
// 3D PRIMITIVES (for tomography plane, debug gizmos)
// ===================================================================

void drawQuad3D(const glm::vec3& v0, const glm::vec3& v1,
                const glm::vec3& v2, const glm::vec3& v3,
                const glm::vec3& color,
                const glm::mat4& mvp,
                float alpha)
{
    const glm::vec3 verts[] = { v0, v1, v2, v3 };

    Draw3D::begin(mvp, color);
    Draw3D::upload(verts, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    Draw3D::end();

    // Optional: if you add alpha uniform later
    (void)alpha;
}

void drawPoints3D(const std::vector<glm::vec3>& points,
                  const glm::vec3& color,
                  const glm::mat4& mvp,
                  float size)
{
    if (points.empty()) return;

    Draw3D::begin(mvp, color);
    Draw3D::upload(points.data(), points.size());
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    Draw3D::end();
}

// ===================================================================
// SHUTDOWN
// ===================================================================
void draw2DShutdown()
{
    Draw2D::shutdown();
    Draw3D::shutdown();
}

// ===================================================================
// LEGACY (safe to remove later)
// ===================================================================
void drawTriangle2D(const std::vector<glm::vec2>& verts,
                    const glm::vec3& color, int w, int h)
{
    auto proj = glm::ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);
    drawTriangleFan2D(verts, color, proj);
}

void drawTriangleFan2D(const std::vector<glm::vec2>& verts,
                       const glm::vec3& color, int w, int h)
{
    auto proj = glm::ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);
    drawTriangleFan2D(verts, color, proj);
}

void drawLineLoop2D(const std::vector<glm::vec2>& points,
                    const glm::vec3& color, int w, int h, float t)
{
    auto proj = glm::ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);
    drawLineLoop2D(points, color, proj, t);
}
