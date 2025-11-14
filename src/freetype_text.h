// freetype_text.h
#ifndef FREETYPE_TEXT_H_
#define FREETYPE_TEXT_H_

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/gl.h>

namespace framework
{
    /**
     * FreeType-based text renderer that mimics GLUT bitmap fonts.
     * This is a wrapper to replace glutBitmapCharacter calls without
     * changing existing code.
     */
    class FreeTypeText
    {
    public:
        static FreeTypeText& instance();

        // Initialize FreeType with a font
        bool initialize(const std::string& fontPath);

        // Clean up resources
        void cleanup();

        // Render a single character at current raster position
        void renderCharacter(char c, int fontSize);

        // Get character width (mimics glutBitmapWidth)
        int getCharWidth(char c, int fontSize);

        // Get string width (mimics glutBitmapLength)
        int getStringWidth(const std::string& text, int fontSize);

        // Set raster position (mimics glRasterPos2f)
        void setRasterPos(float x, float y);

        // Render complete string at current raster position
        void renderString(const std::string& text, int fontSize);

    private:
        FreeTypeText();
        ~FreeTypeText();
        FreeTypeText(const FreeTypeText&) = delete;
        FreeTypeText& operator=(const FreeTypeText&) = delete;

        struct Character {
            unsigned int textureID;
            int width;
            int height;
            int bearingX;
            int bearingY;
            unsigned int advance;
        };

        // Load a character at specific font size
        bool loadCharacter(char c, int fontSize);

        // Get or create character glyph
        Character* getCharacter(char c, int fontSize);

        FT_Library ftLibrary_;
        FT_Face ftFace_;

        // Cache: map of (fontSize, char) -> Character
        std::map<int, std::map<char, Character>> characterCache_;

        // Current raster position
        float rasterX_;
        float rasterY_;

        bool initialized_;

        // Shader program for text rendering
        unsigned int shaderProgram_;
        unsigned int VAO_;
        unsigned int VBO_;

        void initShaders();
        void renderGlyph(const Character& ch, float x, float y);
    };

    // Global convenience functions to maintain API compatibility
    void ftInitText(const std::string& fontPath);
    void ftCleanupText();

} // namespace framework

#endif // FREETYPE_TEXT_H_

// ============================================================================

// freetype_text.cpp
#include "freetype_text.h"
#include <iostream>
#include <GL/glew.h> // You'll need GLEW for modern OpenGL

namespace framework
{
    // Vertex shader for text rendering
    const char* textVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
        out vec2 TexCoords;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    // Fragment shader for text rendering
    const char* textFragmentShader = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;
        
        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    FreeTypeText::FreeTypeText()
        : ftLibrary_(nullptr)
        , ftFace_(nullptr)
        , rasterX_(0.0f)
        , rasterY_(0.0f)
        , initialized_(false)
        , shaderProgram_(0)
        , VAO_(0)
        , VBO_(0)
    {
    }

    FreeTypeText::~FreeTypeText()
    {
        cleanup();
    }

    FreeTypeText& FreeTypeText::instance()
    {
        static FreeTypeText inst;
        return inst;
    }

    bool FreeTypeText::initialize(const std::string& fontPath)
    {
        if (initialized_)
            return true;

        // Initialize FreeType
        if (FT_Init_FreeType(&ftLibrary_))
        {
            std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return false;
        }

        // Load font
        if (FT_New_Face(ftLibrary_, fontPath.c_str(), 0, &ftFace_))
        {
            std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
            return false;
        }

        // Initialize OpenGL resources
        initShaders();

        // Configure VAO/VBO for texture quads
        glGenVertexArrays(1, &VAO_);
        glGenBuffers(1, &VBO_);
        glBindVertexArray(VAO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        initialized_ = true;
        return true;
    }

    void FreeTypeText::cleanup()
    {
        if (!initialized_)
            return;

        // Delete all texture characters
        for (auto& sizeMap : characterCache_)
        {
            for (auto& charPair : sizeMap.second)
            {
                glDeleteTextures(1, &charPair.second.textureID);
            }
        }
        characterCache_.clear();

        // Delete OpenGL resources
        if (VAO_) glDeleteVertexArrays(1, &VAO_);
        if (VBO_) glDeleteBuffers(1, &VBO_);
        if (shaderProgram_) glDeleteProgram(shaderProgram_);

        // Cleanup FreeType
        if (ftFace_) FT_Done_Face(ftFace_);
        if (ftLibrary_) FT_Done_FreeType(ftLibrary_);

        initialized_ = false;
    }

    void FreeTypeText::initShaders()
    {
        // Compile vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &textVertexShader, NULL);
        glCompileShader(vertexShader);

        // Check for errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Compile fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &textFragmentShader, NULL);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Link shaders
        shaderProgram_ = glCreateProgram();
        glAttachShader(shaderProgram_, vertexShader);
        glAttachShader(shaderProgram_, fragmentShader);
        glLinkProgram(shaderProgram_);

        glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram_, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    bool FreeTypeText::loadCharacter(char c, int fontSize)
    {
        // Set pixel size
        FT_Set_Pixel_Sizes(ftFace_, 0, fontSize);

        // Load character glyph
        if (FT_Load_Char(ftFace_, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph " << c << std::endl;
            return false;
        }

        // Generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            ftFace_->glyph->bitmap.width,
            ftFace_->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ftFace_->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character
        Character character = {
            texture,
            (int)ftFace_->glyph->bitmap.width,
            (int)ftFace_->glyph->bitmap.rows,
            ftFace_->glyph->bitmap_left,
            ftFace_->glyph->bitmap_top,
            (unsigned int)ftFace_->glyph->advance.x
        };

        characterCache_[fontSize][c] = character;
        return true;
    }

    FreeTypeText::Character* FreeTypeText::getCharacter(char c, int fontSize)
    {
        // Check if character exists in cache
        auto sizeIt = characterCache_.find(fontSize);
        if (sizeIt != characterCache_.end())
        {
            auto charIt = sizeIt->second.find(c);
            if (charIt != sizeIt->second.end())
                return &charIt->second;
        }

        // Load character if not in cache
        if (loadCharacter(c, fontSize))
            return &characterCache_[fontSize][c];

        return nullptr;
    }

    void FreeTypeText::setRasterPos(float x, float y)
    {
        rasterX_ = x;
        rasterY_ = y;
    }

    void FreeTypeText::renderCharacter(char c, int fontSize)
    {
        Character* ch = getCharacter(c, fontSize);
        if (!ch)
            return;

        renderGlyph(*ch, rasterX_, rasterY_);

        // Advance cursor
        rasterX_ += (ch->advance >> 6); // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    void FreeTypeText::renderGlyph(const Character& ch, float x, float y)
    {
        float xpos = x + ch.bearingX;
        float ypos = y - (ch.height - ch.bearingY);

        float w = ch.width;
        float h = ch.height;

        // Update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void FreeTypeText::renderString(const std::string& text, int fontSize)
    {
        if (!initialized_)
            return;

        // Save OpenGL state
        GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        GLboolean blend = glIsEnabled(GL_BLEND);
        GLint blendSrc, blendDst;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);

        // Set up for text rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // Use shader
        glUseProgram(shaderProgram_);

        // Get viewport for orthographic projection
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        // Set up orthographic projection (pixel coordinates)
        // Note: This assumes bottom-left origin (OpenGL convention)
        float projection[16] = {
            2.0f / viewport[2], 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / viewport[3], 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 1.0f
        };

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"),
                          1, GL_FALSE, projection);
        glUniform3f(glGetUniformLocation(shaderProgram_, "textColor"),
                   1.0f, 1.0f, 1.0f); // White text (get from glColor if needed)

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO_);

        // Render each character
        for (char c : text)
        {
            renderCharacter(c, fontSize);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Restore OpenGL state
        if (!blend) glDisable(GL_BLEND);
        else glBlendFunc(blendSrc, blendDst);
        if (depthTest) glEnable(GL_DEPTH_TEST);
    }

    int FreeTypeText::getCharWidth(char c, int fontSize)
    {
        Character* ch = getCharacter(c, fontSize);
        if (!ch)
            return 0;
        return (ch->advance >> 6);
    }

    int FreeTypeText::getStringWidth(const std::string& text, int fontSize)
    {
        int width = 0;
        for (char c : text)
        {
            width += getCharWidth(c, fontSize);
        }
        return width;
    }

    // Global convenience functions
    void ftInitText(const std::string& fontPath)
    {
        FreeTypeText::instance().initialize(fontPath);
    }

    void ftCleanupText()
    {
        FreeTypeText::instance().cleanup();
    }

} // namespace framework
