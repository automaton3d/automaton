// text_renderer.h

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <map>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Character
{
  unsigned int TextureID;
  glm::ivec2 Size;
  glm::ivec2 Bearing;
  unsigned int Advance; // Distance to advance to next glyph
};

// text_renderer.h
class TextRenderer {
public:
    bool init(const std::string& fontPath, int fontSize, unsigned int shader);

    // Versão completa (já existe)
    void RenderText(const std::string& text,
                    float x, float y,
                    float scale,
                    glm::vec3 color,
                    int screenWidth, int screenHeight);

    // NOVA: versão conveniente que usa tamanho atual da janela
    void RenderText(const std::string& text,
                    float x, float y,
                    float scale,
                    glm::vec3 color);

    // Opcional: versão ainda mais simples com scale = 1.0 e cor branca
    void RenderText(const std::string& text, float x, float y);

    float measureTextWidth(const std::string& text, float scale = 1.0f);
    ~TextRenderer() = default;

    // Para a versão simples funcionar, precisamos saber o tamanho da janela
    void setScreenSize(int width, int height) { screenW = width; screenH = height; }

private:
    std::map<char, Character> Characters;
    unsigned int VAO = 0, VBO = 0;
    unsigned int shaderID = 0;

    // Tamanho da janela atual (usado pelas versões simplificadas)
    int screenW = 800, screenH = 600;
};

#endif // TEXT_RENDERER_H