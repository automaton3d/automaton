
#include "draw_utils.h"
#include <glm/gtc/type_ptr.hpp>

/*
// DEBUG
#include <iostream>
// Test function to verify this file is being compiled
void __test_draw_utils_linked() {
    std::cout << "draw_utils.cpp is linked!" << std::endl;
}
*/

    void drawQuad2D(float x1, float y1, float x2, float y2,
                    const glm::vec3& color,
                    int winW, int winH)
    {
        std::vector<glm::vec2> verts = {
            {x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}
        };

        glUseProgram(colorProgram2D);
        glm::mat4 ortho = glm::ortho(0.0f,(float)winW,0.0f,(float)winH);
        glUniformMatrix4fv(colorMvpLoc2D,1,GL_FALSE,glm::value_ptr(ortho));
        glUniform3f(colorColorLoc2D,color.r,color.g,color.b);

        GLuint vao,vbo;
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(glm::vec2),verts.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);
        glBindVertexArray(0);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
    }

void drawLineLoop2D(const std::vector<glm::vec2>& points, 
                    const glm::vec3& color, 
                    int screenWidth, int screenHeight,
                    float width)
{
    if (!colorProgram2D) return;
    glUseProgram(colorProgram2D);

    glm::mat4 ortho = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D, color.r, color.g, color.b);

    std::vector<float> verts;
    for (auto& p : points) {
        verts.push_back(p.x);  // ← Changed from p.first
        verts.push_back(p.y);  // ← Changed from p.second
    }

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glLineWidth(width);
    glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)points.size());
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void drawTriangle2D(const std::vector<glm::vec2>& verts,
                        const glm::vec3& color,
                        int winW, int winH)
    {
        if (verts.size()!=3) return;

        glUseProgram(colorProgram2D);
        glm::mat4 ortho = glm::ortho(0.0f,(float)winW,0.0f,(float)winH);
        glUniformMatrix4fv(colorMvpLoc2D,1,GL_FALSE,glm::value_ptr(ortho));
        glUniform3f(colorColorLoc2D,color.r,color.g,color.b);

        GLuint vao,vbo;
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(glm::vec2),verts.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES,0,3);
        glBindVertexArray(0);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
    }

    void drawTriangleFan2D(const std::vector<glm::vec2>& verts,
                                  const glm::vec3& color,
                                  int winW, int winH)
{
    if (verts.size() < 3) return;

    glUseProgram(colorProgram2D);
    glm::mat4 ortho = glm::ortho(0.0f,(float)winW,0.0f,(float)winH);
    glUniformMatrix4fv(colorMvpLoc2D,1,GL_FALSE,glm::value_ptr(ortho));
    glUniform3f(colorColorLoc2D,color.r,color.g,color.b);

    GLuint vao,vbo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(glm::vec2),verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN,0,(GLsizei)verts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
}

