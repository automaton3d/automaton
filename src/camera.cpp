/*
 * camera.cpp (adapted)
 *
 * Implements the camera routines.
 */

#include "camera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp> // lookAt
#include <glm/gtc/type_ptr.hpp>         // value_ptr

namespace framework
{

  Camera::Camera()
  {
    reset();
  }

  Camera::~Camera() = default;

  const glm::vec3 & Camera::getCenter()
  {
    return mCenter_;
  }

  const glm::vec3 & Camera::getEye()
  {
    return mEye_;
  }

  const glm::mat4 & Camera::getMatrix()
  {
    return mMatrix_;
  }

  const float* Camera::getMatrixFlat()
  {
    return glm::value_ptr(mMatrix_);
  }

  const glm::vec3 & Camera::getUp()
  {
    return mUp_;
  }

  void Camera::reset()
  {
    mEye_    = glm::vec3(0.f, 0.f, 1.f);
    mCenter_ = glm::vec3(0.f, 0.f, 0.f);
    mUp_     = glm::vec3(0.f, 1.f, 0.f);

    update();
  }

  void Camera::setEye(float x, float y, float z)
  {
    mEye_ = glm::vec3(x, y, z);
  }

  void Camera::setEye(const glm::vec3 & e)
  {
    mEye_ = e;
  }

  void Camera::setCenter(float x, float y, float z)
  {
    mCenter_ = glm::vec3(x, y, z);
  }

  void Camera::setCenter(const glm::vec3 & c)
  {
    mCenter_ = c;
  }

  void Camera::setUp(float x, float y, float z)
  {
    mUp_ = glm::vec3(x, y, z);
  }

  void Camera::setUp(const glm::vec3 & u)
  {
    mUp_ = u;
  }

  void Camera::update()
  {
    mMatrix_ = glm::lookAt(mEye_, mCenter_, mUp_);
  }

} // end namespace framework
