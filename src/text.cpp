#define VBVB
#ifdef VBVB
/*
 * text.cpp
 *
 * Text drawing routines.
 */
#include <GUI.h>

namespace framework
{
  using namespace std;

  void render2Dstring(float x, float y, void *font, const char *string)
  {
    glRasterPos2f(x, y);  // Set raster position where text will start
    for (int i = 0; i < string[i]; i++)
      glutBitmapCharacter(font, string[i]);
  }

  /*
   * Renders 3D strings.
   */
  void render3Dstring(float x, float y, float z, void *font, const char *string)
  {
    glRasterPos3f(x, y, z);  // Set raster position in 3D space
    for (int i = 0; string[i] != '\0'; i++)  // Loop through the string
      glutBitmapCharacter(font, string[i]);
  }

  void drawString(string s, int x, int y, int size)
  {
    glRasterPos2f(x, y);
	if (size == 8)
	{
	  for (int i = 0; s[i] != '\0'; ++i)
	    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);

	}
	else if (size == 12)
	{
	  for (char c : s)
	  {
	    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
	  }
	}
	else if (size == 15)
	{
	  for (char c : s)
	  {
	    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
	  }
	}
	else if (size == 18)
	{
	  for (char c : s)
	  {
	    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
	  }
	}
  }

  /*
   * Draw bold text.
   */
  void drawBoldText(const string& text, int x, int y, float offset)
  {
    for (float dx = -offset; dx <= offset; dx += offset)
    {
      for (float dy = -offset; dy <= offset; dy += offset)
      {
        glRasterPos2i(x + dx, y + dy);
        for (char c : text)
        {
          glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }
      }
    }
  }

}
#else

// text.cpp
// Freetype-backed implementation of the API declared in text.h
// - Bottom-left origin for 2D (y=0 at bottom), consistent with freeglut convention.
// - Matches existing function signatures; no changes needed elsewhere.
// - Simple bold via multi-pass offset.
// Requirements: Link against freetype, OpenGL, and freeglut. Ensure FONT_PATH points to a valid .ttf.

#include "text.h"

#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <ft2build.h>
#include FT_FREETYPE_H



#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace framework {

// ---------- Configuration ----------
#ifndef FONT_PATH
// Set this to a valid TTF in your project or system.
// Common options:
// - "fonts/DejaVuSans.ttf"
// - "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
// - "C:/Windows/Fonts/arial.ttf"
#define FONT_PATH "fonts/DejaVuSans.ttf"
#endif

// ---------- Glyph cache ----------
struct Glyph {
    GLuint texID = 0;
    int width = 0;
    int height = 0;
    int bearingX = 0;
    int bearingY = 0;
    int advance = 0; // in 26.6 fixed-point >> 6 (pixels)
    FT_UInt glyphIndex = 0; // for kerning
};

struct FontSizeCache {
    // Per-size glyph cache
    std::map<uint32_t, Glyph> glyphs; // codepoint -> glyph
    int pixelSize = 0;
};

static FT_Library g_ftLib = nullptr;
static FT_Face g_ftFace = nullptr;
static std::map<int, FontSizeCache> g_sizeCache; // size (px) -> cache
static bool g_inited = false;


#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>


glm::vec3 unProject(const glm::vec3& win,
                    const glm::mat4& model,
                    const glm::mat4& proj,
                    const int viewport[4]) {
    glm::vec4 tmp;
    tmp.x = (win.x - viewport[0]) / viewport[2] * 2.0f - 1.0f;
    tmp.y = (win.y - viewport[1]) / viewport[3] * 2.0f - 1.0f;
    tmp.z = 2.0f * win.z - 1.0f;
    tmp.w = 1.0f;

    glm::mat4 inv = glm::inverse(proj * model);
    tmp = inv * tmp;
    tmp /= tmp.w;
    return glm::vec3(tmp);
}



// ---------- Utility: UTF-8 decoding ----------
static std::u32string utf8_to_utf32(const std::string& s) {
    std::u32string out;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0, n = s.size();

    while (i < n) {
        uint32_t cp = 0;
        unsigned char c = p[i];

        if (c < 0x80) { // 1 byte
            cp = c;
            i += 1;
        } else if ((c >> 5) == 0x6 && i + 1 < n) { // 2 bytes
            cp = ((c & 0x1F) << 6) | (p[i+1] & 0x3F);
            i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < n) { // 3 bytes
            cp = ((c & 0x0F) << 12) | ((p[i+1] & 0x3F) << 6) | (p[i+2] & 0x3F);
            i += 3;
        } else if ((c >> 3) == 0x1E && i + 3 < n) { // 4 bytes
            cp = ((c & 0x07) << 18) | ((p[i+1] & 0x3F) << 12) | ((p[i+2] & 0x3F) << 6) | (p[i+3] & 0x3F);
            i += 4;
        } else {
            // Invalid; skip
            i += 1;
            continue;
        }
        out.push_back(cp);
    }
    return out;
}

// ---------- FreeType init ----------
static void ensureInit() {
    if (g_inited) return;

    if (FT_Init_FreeType(&g_ftLib)) {
        throw std::runtime_error("Failed to initialize FreeType");
    }
    if (FT_New_Face(g_ftLib, FONT_PATH, 0, &g_ftFace)) {
        throw std::runtime_error(std::string("Failed to load font: ") + FONT_PATH);
    }

    // Use a neutral charmap (Unicode) if available
    if (FT_Select_Charmap(g_ftFace, FT_ENCODING_UNICODE)) {
        // Some fonts may not have Unicode charmap; continue anyway
    }

    g_inited = true;
}

// ---------- Load glyph for a given pixel size ----------
static const Glyph& getGlyph(uint32_t codepoint, int pixelSize) {
    ensureInit();

    FontSizeCache& cache = g_sizeCache[pixelSize];
    if (cache.pixelSize != pixelSize) {
        cache.pixelSize = pixelSize;
        FT_Set_Pixel_Sizes(g_ftFace, 0, pixelSize);
    }

    auto it = cache.glyphs.find(codepoint);
    if (it != cache.glyphs.end()) return it->second;

    Glyph g;

    // Get glyph index
    FT_UInt glyphIndex = FT_Get_Char_Index(g_ftFace, codepoint);
    g.glyphIndex = glyphIndex;

    // Render glyph
    if (FT_Load_Glyph(g_ftFace, glyphIndex, FT_LOAD_DEFAULT)) {
        // Fallback to missing glyph (typically .notdef)
        glyphIndex = FT_Get_Char_Index(g_ftFace, 0);
        FT_Load_Glyph(g_ftFace, glyphIndex, FT_LOAD_DEFAULT);
    }
    FT_Render_Glyph(g_ftFace->glyph, FT_RENDER_MODE_NORMAL);

    FT_GlyphSlot slot = g_ftFace->glyph;
    FT_Bitmap& bmp = slot->bitmap;

    g.width = bmp.width;
    g.height = bmp.rows;
    g.bearingX = slot->bitmap_left;
    g.bearingY = slot->bitmap_top;
    g.advance = slot->advance.x >> 6; // pixels

    // Create GL texture for the glyph
    std::vector<unsigned char> rgba;
    rgba.resize(g.width * g.height * 4, 0);

    for (int y = 0; y < g.height; ++y) {
        for (int x = 0; x < g.width; ++x) {
            unsigned char alpha = bmp.buffer[y * bmp.pitch + x];
            size_t idx = (y * g.width + x) * 4;
            rgba[idx + 0] = 255;
            rgba[idx + 1] = 255;
            rgba[idx + 2] = 255;
            rgba[idx + 3] = alpha;
        }
    }

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g.width, g.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    g.texID = texID;
    cache.glyphs[codepoint] = g;
    return cache.glyphs[codepoint];
}

// ---------- Kerning ----------
static int getKerning(FT_UInt leftIdx, FT_UInt rightIdx) {
    if (!FT_HAS_KERNING(g_ftFace)) return 0;
    FT_Vector delta{};
    if (FT_Get_Kerning(g_ftFace, leftIdx, rightIdx, FT_KERNING_DEFAULT, &delta) == 0) {
        return static_cast<int>(delta.x >> 6); // pixels
    }
    return 0;
}

// ---------- OpenGL helpers ----------
struct GLStateGuard {
    GLboolean blendEnabled;
    GLint blendSrc, blendDst;
    GLboolean textureEnabled;

    GLStateGuard() {
        blendEnabled = glIsEnabled(GL_BLEND);
        textureEnabled = glIsEnabled(GL_TEXTURE_2D);
        glGetIntegerv(GL_BLEND_SRC, &blendSrc);
        glGetIntegerv(GL_BLEND_DST, &blendDst);
    }
    ~GLStateGuard() {
        if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        glBlendFunc(blendSrc, blendDst);
        if (textureEnabled) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    }
};

static void beginTextDraw() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.f, 1.f, 1.f, 1.f);
}

static void drawGlyphQuad(const Glyph& g, float x, float y) {
    // Place glyph so that (x, y) is the baseline origin (lower-left convention),
    // FreeType gives bearing relative to baseline; invert y for bitmap top.
    float xpos = x + g.bearingX;
    float ypos = y + g.bearingY - g.height; // move down because bitmap top is bearingY

    float w = static_cast<float>(g.width);
    float h = static_cast<float>(g.height);

    glBindTexture(GL_TEXTURE_2D, g.texID);
    glBegin(GL_QUADS);
        glTexCoord2f(0.f, 0.f); glVertex2f(xpos,     ypos);
        glTexCoord2f(1.f, 0.f); glVertex2f(xpos + w, ypos);
        glTexCoord2f(1.f, 1.f); glVertex2f(xpos + w, ypos + h);
        glTexCoord2f(0.f, 1.f); glVertex2f(xpos,     ypos + h);
    glEnd();
}

// ---------- 2D projection switching ----------
static void pushOrtho2D() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    // Bottom-left origin
    gluOrtho2D(0.0, static_cast<GLdouble>(w), 0.0, static_cast<GLdouble>(h));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void popOrtho2D() {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ---------- Core render routine ----------
static void renderText2D(const std::u32string& u32, int x, int y, int size, float extraOffsetX = 0.f, float extraOffsetY = 0.f) {
    ensureInit();
    FontSizeCache& cache = g_sizeCache[size];
    if (cache.pixelSize != size) {
        cache.pixelSize = size;
        FT_Set_Pixel_Sizes(g_ftFace, 0, size);
    }

    GLStateGuard guard;
    beginTextDraw();

    pushOrtho2D();

    float penX = static_cast<float>(x) + extraOffsetX;
    float penY = static_cast<float>(y) + extraOffsetY;

    FT_UInt prevGlyphIdx = 0;

    for (size_t i = 0; i < u32.size(); ++i) {
        uint32_t cp = u32[i];
        const Glyph& g = getGlyph(cp, size);

        if (prevGlyphIdx) {
            penX += static_cast<float>(getKerning(prevGlyphIdx, g.glyphIndex));
        }

        drawGlyphQuad(g, penX, penY);
        penX += g.advance;
        prevGlyphIdx = g.glyphIndex;
    }

    popOrtho2D();
}

// ---------- Public API ----------

void drawString(std::string s, int x, int y, int size) {
    // Render in screen space with bottom-left origin.
    // Matches expected freeglut behavior but with a TTF font via FreeType.
    renderText2D(utf8_to_utf32(s), x, y, size);
}

void drawBoldText(const std::string& text, int x, int y, float offset) {
    // Simple bold by multi-pass with small offsets.
    // You can tweak offset to taste; defaults to 0.5 pixels.
    auto u32 = utf8_to_utf32(text);
    int size = 16; // Default size if none provided by API; adjust if your code expects a specific size.
    // First pass (shadow/weight)
    renderText2D(u32, x, y, size, offset, 0.0f);
    // Second pass (main)
    renderText2D(u32, x, y, size, 0.0f, 0.0f);
}

void render2Dstring(float x, float y, void * /*font*/, const char * string) {
    // The 'font' argument is ignored for compatibility; FreeType uses the TTF specified by FONT_PATH.
    // Size heuristic: use 16px to approximate GLUT_BITMAP_8_BY_13/HELVETICA_12.
    int size = 16;
    renderText2D(utf8_to_utf32(std::string(string ? string : "")), static_cast<int>(x), static_cast<int>(y), size);
}

void render3Dstring(float x, float y, float z, void * /*font*/, const char * string) {
    // Render at a 3D position by placing glyph quads in the XY plane at Z.
    // Glyphs are sized in pixels approximated via current projection scale:
    // We derive a scale factor using viewport height to keep a pixel-like size.
    ensureInit();

    std::string s = std::string(string ? string : "");
    auto u32 = utf8_to_utf32(s);
    int size = 16;

    FontSizeCache& cache = g_sizeCache[size];
    if (cache.pixelSize != size) {
        cache.pixelSize = size;
        FT_Set_Pixel_Sizes(g_ftFace, 0, size);
    }

    GLStateGuard guard;
    beginTextDraw();

    // Compute pixel-to-world scale by projecting a unit distance
    // We use the current matrices; this stays compatible with existing code.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    // Convert a 1-pixel step to world units along X and Y:
    // Project two points and invert. If matrices are non-linear, this is an approximation.
    GLdouble model[16], proj[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    auto unprojectDelta = [&](double sx, double sy) -> std::pair<float, float> {
        GLdouble ox, oy, oz;
        GLdouble ox2, oy2, oz2;
        unProject(viewport[0] + sx, viewport[1] + sy, 0.0, model, proj, viewport, &ox, &oy, &oz);
        unProject(viewport[0], viewport[1], 0.0, model, proj, viewport, &ox2, &oy2, &oz2);
        return { static_cast<float>(ox - ox2), static_cast<float>(oy - oy2) };
    };

    auto dx = unprojectDelta(1.0, 0.0).first;
    auto dy = unprojectDelta(0.0, 1.0).second;
    float pxToWorldX = (dx == 0.0f) ? 0.001f : dx;
    float pxToWorldY = (dy == 0.0f) ? 0.001f : dy;

    // Position in world coordinates
    glPushMatrix();
    glTranslatef(x, y, z);

    float penX = 0.0f;
    float penY = 0.0f;
    FT_UInt prevGlyphIdx = 0;

    for (size_t i = 0; i < u32.size(); ++i) {
        const Glyph& g = getGlyph(u32[i], size);

        if (prevGlyphIdx) {
            float kern = static_cast<float>(getKerning(prevGlyphIdx, g.glyphIndex));
            penX += kern;
        }

        // Base point per glyph in world units
        float gx = penX * pxToWorldX;
        float gy = penY * pxToWorldY;

        float xpos = gx + g.bearingX * pxToWorldX;
        float ypos = gy + (g.bearingY - g.height) * pxToWorldY;

        float w = g.width * pxToWorldX;
        float h = g.height * pxToWorldY;

        glBindTexture(GL_TEXTURE_2D, g.texID);
        glBegin(GL_QUADS);
            glTexCoord2f(0.f, 0.f); glVertex3f(xpos,     ypos,     0.0f);
            glTexCoord2f(1.f, 0.f); glVertex3f(xpos + w, ypos,     0.0f);
            glTexCoord2f(1.f, 1.f); glVertex3f(xpos + w, ypos + h, 0.0f);
            glTexCoord2f(0.f, 1.f); glVertex3f(xpos,     ypos + h, 0.0f);
        glEnd();

        penX += static_cast<float>(g.advance);
        prevGlyphIdx = g.glyphIndex;
    }

    glPopMatrix();
}

} // namespace framework
#endif
