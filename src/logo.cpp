/*
 * logo.cpp
 */

#include "logo.h"
#include <iostream>
#include "stb_image.h"

namespace framework
{
  Logo::Logo(const std::string& path)
  {
    int channels;
    unsigned char* data = stbi_load(path.c_str(), &mWidth_, &mHeight_, &channels, 4);
    if (!data)
    {
	  std::cerr << "Logo: failed to load '" << path << "'\n";
	  return;
	}
    glGenTextures(1, &mTexture_);
    glBindTexture(GL_TEXTURE_2D, mTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth_, mHeight_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
  }

  Logo::~Logo()
  {
	if (mTexture_) glDeleteTextures(1, &mTexture_);
  }

  void Logo::draw(int x, int y, float scale) const
  {
	if (!mTexture_) return;

	const int w = static_cast<int>(mWidth_  * scale);
	const int h = static_cast<int>(mHeight_ * scale);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, mTexture_);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
	    glTexCoord2f(0, 1); glVertex2i(x,     y);
	    glTexCoord2f(1, 1); glVertex2i(x + w, y);
	    glTexCoord2f(1, 0); glVertex2i(x + w, y + h);
	    glTexCoord2f(0, 0); glVertex2i(x,     y + h);
	glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
  }

} // namespace splash
