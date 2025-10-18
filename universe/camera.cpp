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
    return mCenter;
  }

  const glm::vec3 & Camera::getEye()
  {
    return mEye;
  }

  const glm::mat4 & Camera::getMatrix()
  {
    return mMatrix;
  }

  const float* Camera::getMatrixFlat()
  {
    return glm::value_ptr(mMatrix);
  }

  const glm::vec3 & Camera::getUp()
  {
    return mUp;
  }

  void Camera::reset()
  {
    mEye.x = 0.f;
    mEye.y = 0.f;
    mEye.z = 1.f;
    mCenter.x = 0.f;
    mCenter.y = 0.f;
    mCenter.z = 0.f;
    mUp.x = 0.f;
    mUp.y = 1.f;
    mUp.z = 0.f;

      update();
  }

  void Camera::setEye(float x, float y, float z)
  {
    mEye.x = x;
    mEye.y = y;
    mEye.z = z;
  }

  void Camera::setEye(const glm::vec3 & e)
  {
      mEye = e;
  }

  void Camera::setCenter(float x, float y, float z)
  {
    mCenter.x = x;
    mCenter.y = y;
    mCenter.z = z;
  }

  void Camera::setCenter(const glm::vec3 & c)
  {
      mCenter = c;
  }

  void Camera::setUp(float x, float y, float z)
  {
    mUp.x = x;
    mUp.y = y;
    mUp.z = z;
  }

  void Camera::setUp(const glm::vec3 & u)
  {
    mUp = u;
  }

  void Camera::update()
  {
    mMatrix = glm::lookAt(mEye, mCenter, mUp);
  }

} // end namespace framework
