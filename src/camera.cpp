/*
 * camera.cpp
 *
 * Implements the camera routines.
 */

#include "camera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp> // lookAt
#include <glm/gtc/type_ptr.hpp> // value_ptr

namespace framework
{

  Camera::Camera()
  {
    reset();
  }

  Camera::~Camera()
  {
  }

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
    mEye_.x = 0.f;
    mEye_.y = 0.f;
    mEye_.z = 1.f;
    mCenter_.x = 0.f;
    mCenter_.y = 0.f;
    mCenter_.z = 0.f;
    mUp_.x = 0.f;
    mUp_.y = 1.f;
    mUp_.z = 0.f;

      update();
  }

  void Camera::setEye(float x, float y, float z)
  {
    mEye_.x = x;
    mEye_.y = y;
    mEye_.z = z;
  }

  void Camera::setEye(const glm::vec3 & e)
  {
      mEye_ = e;
  }

  void Camera::setCenter(float x, float y, float z)
  {
    mCenter_.x = x;
    mCenter_.y = y;
    mCenter_.z = z;
  }

  void Camera::setCenter(const glm::vec3 & c)
  {
      mCenter_ = c;
  }

  void Camera::setUp(float x, float y, float z)
  {
    mUp_.x = x;
    mUp_.y = y;
    mUp_.z = z;
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
