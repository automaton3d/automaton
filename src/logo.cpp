/*
 * logo.cpp (clean version)
 */

#include "logo.h"
#include "stb_image.h"
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "globals.h"

namespace framework
{
  Logo::Logo(const std::string& path)
  {
    // Candidate paths to search
    std::vector<std::string> searchPaths = {
        path,
        "fonts/" + path,
        "../" + path,
        "../../assets/" + path,
        "../assets/" + path
    };

    unsigned char* data = nullptr;
    int channels = 0;
    for (const auto& tryPath : searchPaths) {
        data = stbi_load(tryPath.c_str(), &mWidth_, &mHeight_, &channels, 4);
        if (data) {
            std::cout << "Logo loaded from: " << tryPath << std::endl;
            break;
        }
    }

    if (!data) {
        std::cerr << "Logo: failed to load '" << path << "' from any location\n";
        for (const auto& p : searchPaths)
            std::cerr << "  - " << p << "\n";
        mTexture_ = 0;
        mWidth_   = 0;
        mHeight_  = 0;
        return;
    }

    glGenTextures(1, &mTexture_);
    glBindTexture(GL_TEXTURE_2D, mTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth_, mHeight_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    std::cout << "Logo texture created: ID=" << mTexture_
              << " size=" << mWidth_ << "x" << mHeight_ << std::endl;
  }

  Logo::~Logo()
  {
    if (mTexture_) glDeleteTextures(1, &mTexture_);
  }

  void Logo::draw(int x, int y, float scale) const
  {
    if (!mTexture_) {
        std::cerr << "Logo::draw() - texture not loaded\n";
        return;
    }

    if (!textureProgram2D) {
        std::cerr << "Logo::draw() - textureProgram2D is 0\n";
        return;
    }

    if (textureMvpLoc == -1 || textureSamplerLoc == -1) {
        std::cerr << "Logo::draw() - invalid uniform locations (MVP: "
                  << textureMvpLoc << ", Sampler: " << textureSamplerLoc << ")\n";
        return;
    }

    // Normalize to ~200px width
    const int targetW = 200;
    const int targetH = (mHeight_ * targetW) / mWidth_;

    const int w = static_cast<int>(targetW * scale);
    const int h = static_cast<int>(targetH * scale);

    struct Vertex { glm::vec2 pos; glm::vec2 uv; };
    std::vector<Vertex> verts = {
        {{x,     y},     {0.f, 1.f}},
        {{x + w, y},     {1.f, 1.f}},
        {{x + w, y + h}, {1.f, 0.f}},
        {{x,     y + h}, {0.f, 0.f}}
    };

    glUseProgram(textureProgram2D);

    // Top-origin coordinate system (y increases downward)
    glm::mat4 ortho = glm::ortho(
        0.0f, static_cast<float>(gViewport[2]),
        static_cast<float>(gViewport[3]), 0.0f,
        -1.0f, 1.0f
    );

    glUniformMatrix4fv(textureMvpLoc, 1, GL_FALSE, glm::value_ptr(ortho));
    glUniform1i(textureSamplerLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture_);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

} // namespace framework
