// rect_renderer.h
/*
 * rect_renderer.h
 */

#pragma once
#include <glm/glm.hpp>

namespace framework
{
class RectRenderer
{
private:
  unsigned int VAO, VBO, shaderID;

public:
  ~RectRenderer();
  void init();
  void initGL();

  void draw(float x, float y, float width, float height, glm::vec3 color);
  void draw(float x, float y, float width, float height, glm::vec3 color, int screenWidth, int screenHeight);
};

}
