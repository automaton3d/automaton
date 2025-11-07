/*
 * logo.h
 */

#ifndef LOGO_H_
#define LOGO_H_

#include <GL/gl.h>
#include <string>
#include "stb_image.h"

namespace framework
{
  class Logo
  {
  public:
    explicit Logo(const std::string& path = "logo.png");
    ~Logo();

    Logo(const Logo&)            = delete;
    Logo& operator=(const Logo&) = delete;
    Logo(Logo&&)                 = default;
    Logo& operator=(Logo&&)      = default;

    void draw(int x, int y, float scale = 1.0f) const;
    void draw(float centerX, float centerY, float ndcSize) const;

    int width()  const noexcept { return mWidth_;  }
    int height() const noexcept { return mHeight_; }
    bool valid() const noexcept { return mTexture_ != 0; }

  private:
    GLuint mTexture_ = 0;
    int    mWidth_  = 0;
    int    mHeight_ = 0;
  };
} // namespace splash

#endif /* LOGO_H_ */
