// freetype_text.h
#define FREETYPE_TEXT_H_

#include <string>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

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

