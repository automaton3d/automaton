/*
 * camera.h
 *
 * Declares the camera routines.
 */

#pragma once

#include <glm/glm.hpp>

namespace framework
{

class Camera
{
public:
  Camera();
  ~Camera();

  const glm::mat4 & getMatrix();
  const float* getMatrixFlat();
  const glm::vec3 & getCenter();
  const glm::vec3 & getEye();
  const glm::vec3 & getUp();
    void reset();
  void setCenter(float x, float y, float z);
    void setCenter(const glm::vec3 & c);
  void setEye(float x, float y, float z);
    void setEye(const glm::vec3 & e);
  void setUp(float x, float y, float z);
    void setUp(const glm::vec3 & u);
  void update();

private:
  glm::vec3 mCenter_;
  glm::vec3 mEye_;
  glm::mat4 mMatrix_;
  glm::vec3 mUp_;
};

} // end namespace framework
