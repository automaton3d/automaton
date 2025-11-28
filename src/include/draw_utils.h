#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Externos: já definidos em seu projeto
extern GLuint colorProgram2D;
extern GLint colorMvpLoc2D;
extern GLint colorColorLoc2D;


    // Desenha um quad 2D (retângulo preenchido)
    void drawQuad2D(float x1, float y1, float x2, float y2,
                    const glm::vec3& color,
                    int winW, int winH);

    // Desenha um loop de linhas (borda)
void drawLineLoop2D(const std::vector<glm::vec2>& points, 
                    const glm::vec3& color, 
                    int screenWidth, int screenHeight,
                    float width = 1.0f); 
                    
    // Desenha um triângulo 2D
    void drawTriangle2D(const std::vector<glm::vec2>& verts,
                        const glm::vec3& color,
                        int winW, int winH);

    void drawTriangleFan2D(const std::vector<glm::vec2>& verts,
                           const glm::vec3& color,
                           int winW, int winH);
