// rect_renderer.cpp
/*
 * rect_renderer.cpp
 */
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "rect_renderer.h"

namespace framework
{
const char* rectVertexShader = R"glsl(
    #version 460 core
    layout (location = 0) in vec2 aPos;

    uniform mat4 projection;
    uniform vec2 offset;
    uniform vec2 size;

    void main() {
        vec2 scaled = aPos * size + offset;
        gl_Position = projection * vec4(scaled, 0.0, 1.0);
    }
)glsl";

const char* rectFragmentShader = R"glsl(
    #version 460 core
    out vec4 FragColor;
    uniform vec3 color;

    void main() {
        FragColor = vec4(color, 1.0);
    }
)glsl";

unsigned int compileRectShader() {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &rectVertexShader, nullptr);
    glCompileShader(vs);

    // Check vertex shader compilation
    GLint success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, nullptr, infoLog);
        std::cerr << "Rect vertex shader compilation failed: " << infoLog << std::endl;
    }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &rectFragmentShader, nullptr);
    glCompileShader(fs);

    // Check fragment shader compilation
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, nullptr, infoLog);
        std::cerr << "Rect fragment shader compilation failed: " << infoLog << std::endl;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Check linking
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Rect shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void RectRenderer::init()
{
  shaderID = compileRectShader();
  float vertices[] =
  {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
  };
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void RectRenderer::draw(float x, float y, float width, float height, glm::vec3 color, int screenWidth, int screenHeight)
{
  // Overloaded version that accepts screen dimensions
  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));
  glUseProgram(shaderID);
  glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
  glUniform3fv(glGetUniformLocation(shaderID, "color"), 1, &color[0]);
  glUniform2f(glGetUniformLocation(shaderID, "offset"), x, y);
  glUniform2f(glGetUniformLocation(shaderID, "size"), width, height);
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

RectRenderer::~RectRenderer()
{
  if (VAO) glDeleteVertexArrays(1, &VAO);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (shaderID) glDeleteProgram(shaderID); // Assuming shaderID is a program ID
}

}
