/*
 * projection.cpp (old)
 */

/*
 * projection.cpp
 */

#include <GUI.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "projection.h"

namespace framework
{
  using namespace std;

  AxisThumb thumb;

  void map3DTo2D(const glm::vec3& pt,
                   double mouseX, double mouseY,
                   float depth,
                   float axisLength)
    {
        GLfloat model[16], proj[16];
        GLint viewport[4];
        glGetFloatv(GL_MODELVIEW_MATRIX, model);
        glGetFloatv(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, viewport);

        glm::mat4 modelMat = glm::make_mat4(model);
        glm::mat4 projMat  = glm::make_mat4(proj);
        glm::vec4 viewportVec(viewport[0], viewport[1], viewport[2], viewport[3]);

        // Unproject mouse position (mouseY must be bottom-origin)
        glm::vec3 world = glm::unProject(glm::vec3(mouseX, mouseY, depth),
                                         modelMat, projMat, viewportVec);

        if (thumb.axis == 0)
            thumb.position = glm::clamp(world.x, 0.0f, axisLength);
        else if (thumb.axis == 1)
            thumb.position = glm::clamp(world.y, 0.0f, axisLength);
        else if (thumb.axis == 2)
            thumb.position = glm::clamp(world.z, 0.0f, axisLength);
    }

  /**
   * Sets 2D projection.
   */
  void setOrthographicProjection()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport[2], 0, viewport[3], -1, 1);
    glScalef(1, -1, 1);
    glTranslatef(0, -viewport[3], 0);
    glMatrixMode(GL_MODELVIEW);
  }

  /**
   * Sets 3D projection.
   */
  void resetPerspectiveProjection()
  {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
  }

  bool projectPoint(const float obj[3],
                    const GLfloat modelview[16],       // ← GLfloat
                    const GLfloat projection[16],      // ← GLfloat
                    const GLint viewport[4],
                    float &winX, float &winY)
  {
    // Transform to eye space
    double eye[4] =
    {
      modelview[0]*obj[0] + modelview[4]*obj[1] + modelview[8]*obj[2] + modelview[12],
      modelview[1]*obj[0] + modelview[5]*obj[1] + modelview[9]*obj[2] + modelview[13],
      modelview[2]*obj[0] + modelview[6]*obj[1] + modelview[10]*obj[2] + modelview[14],
      modelview[3]*obj[0] + modelview[7]*obj[1] + modelview[11]*obj[2] + modelview[15]
    };

    // Transform to clip space
    double clip[4] =
    {
      projection[0]*eye[0] + projection[4]*eye[1] + projection[8]*eye[2] + projection[12]*eye[3],
      projection[1]*eye[0] + projection[5]*eye[1] + projection[9]*eye[2] + projection[13]*eye[3],
      projection[2]*eye[0] + projection[6]*eye[1] + projection[10]*eye[2] + projection[14]*eye[3],
      projection[3]*eye[0] + projection[7]*eye[1] + projection[11]*eye[2] + projection[15]*eye[3]
    };

    if (clip[3] == 0.0) return false; // Behind camera

    // Perspective division
    clip[0] /= clip[3];
    clip[1] /= clip[3];

    // Convert to window coordinates
    winX = (clip[0] * 0.5f + 0.5f) * viewport[2] + viewport[0];
    winY = (clip[1] * 0.5f + 0.5f) * viewport[3] + viewport[1];
    return true;
  }

  float distanceToSegment(float px, float py, float ax, float ay, float bx, float by) {
      float dx = bx - ax;
      float dy = by - ay;
      float denom = dx*dx + dy*dy;
      if (denom == 0.0f) return sqrtf((px - ax)*(px - ax) + (py - ay)*(py - ay));
      float t = ((px - ax) * dx + (py - ay) * dy) / denom;
      t = std::max(0.0f, std::min(1.0f, t));
      float cx = ax + t * dx;
      float cy = ay + t * dy;
      return sqrtf((px - cx)*(px - cx) + (py - cy)*(py - cy));
  }

  // Compute closest point between ray and segment.
  // Returns t in [0,1] along the segment, and sets outDist to the distance.
  float closestPointOnSegmentToRay(const glm::vec3& rayO, const glm::vec3& rayD,
                                   const glm::vec3& segA, const glm::vec3& segB,
                                   float& outDist)
  {
      glm::vec3 u = rayD;           // ray direction
      glm::vec3 v = segB - segA;    // segment direction
      glm::vec3 w0 = rayO - segA;

      float A = glm::dot(u,u);      // = 1 if rayD normalized
      float B = glm::dot(u,v);
      float C = glm::dot(v,v);
      float D = glm::dot(u,w0);
      float E = glm::dot(v,w0);

      float denom = A*C - B*B;
      float sc, tc;
      if (denom < 1e-8f) {
          // Nearly parallel
          sc = 0.0f;
          tc = glm::clamp(E / (C + 1e-8f), 0.0f, 1.0f);
      } else {
          sc = (B*E - C*D) / denom;
          tc = glm::clamp((A*E - B*D) / denom, 0.0f, 1.0f);
      }

      glm::vec3 Pc = rayO + sc*u;
      glm::vec3 Qc = segA + tc*v;
      outDist = glm::length(Pc - Qc);
      return tc;
  }

}

