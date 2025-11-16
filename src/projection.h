/*
 * projection.h (old)
 */

#ifndef PROJECTION_H_
#define PROJECTION_H_

#include <string>
#include <GL/gl.h>

namespace framework
{

  struct AxisThumb {
    bool active = false;
    int axis = -1;      // 0=X,1=Y,2=Z
    float position = 0.0f; // normalized [0, axisLength]
  };
  extern AxisThumb thumb;

  void setOrthographicProjection();
  void resetPerspectiveProjection();

  // Project a 3D point to window space (accept GLfloat to match glGetFloatv)
  bool projectPoint(const float obj[3],
                    const GLfloat modelview[16],
                    const GLfloat projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY);

  // Unprojects mouse to world and clamps along active axis
  void map3DTo2D(const glm::vec3& pt,
                 double mouseX, double mouseY,
                 float depth,
                 float axisLength);

  // 2D distance from point to segment (used for axis hit-testing)
  float distanceToSegment(float px, float py,
                          float ax, float ay,
                          float bx, float by);

  float closestPointOnSegmentToRay(const glm::vec3& rayO, const glm::vec3& rayD,
                                   const glm::vec3& segA, const glm::vec3& segB,
                                   float& outDist);

}

#endif /* PROJECTION_H_ */
