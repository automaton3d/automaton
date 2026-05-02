/*
 * xxx.h
 *
 *  Created on: 28 de nov. de 2025
 *      Author: Alexandre
 */

#ifndef INCLUDE_MOUSE_HELPER_H_
#define INCLUDE_MOUSE_HELPER_H_

#include "camera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace framework
{

/*
  void computePointOnSphere(const glm::vec2 & point, glm::vec3 & result) {}
  void computeRotationBetweenVectors(const glm::vec3 & start,
                                     const glm::vec3 & stop,
                                     glm::quat & result) {}
                                     */
  void setScrollDirection(bool up) {}
  void setClickPoint(double x, double y) {}
  void setLeftClicked(bool value) {}
  void setMiddleClicked(bool value) {}
  void setRightClicked(bool value) {}
  void setSpeed(float s) {}
}

#endif /* INCLUDE_MOUSE_HELPER_H_ */
