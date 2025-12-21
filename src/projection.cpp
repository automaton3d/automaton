/*
 * projection.cpp (adaptado para GLAD + modern shaders)
 */

#include "GUI.h"
#include <glad/glad.h>
#include "glm_host_only.h"
#include "projection.h"
#include "projection_manager.h"
#include "globals.h"  // for gViewport

namespace framework
{
  extern AxisThumb thumb;
  extern glm::mat4 mProjection_;
  extern int vis_dx;
  extern int vis_dy;
  extern int vis_dz;

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

      // Unproject current mouse position at the depth of the axis
      glm::vec3 world = glm::unProject(glm::vec3(mouseX, viewport[3] - mouseY, depth),
                                       modelMat, projMat, viewportVec);

      float raw_pos = 0.0f;
      if (thumb.axis == 0)      raw_pos = world.x;
      else if (thumb.axis == 1) raw_pos = world.y;
      else if (thumb.axis == 2) raw_pos = world.z;

      // === THIS IS THE KEY CHANGE ===
      // Instead of clamping, compute total delta from initial click
      float delta_world = raw_pos - thumb.initialPosition;

      // Accumulate total offset (can be > axisLength or negative)
      thumb.position = thumb.initialPosition + delta_world;

      // Wrap only for visualization (the thumb dot jumps to the other end)
      float wrapped = std::fmod(thumb.position, axisLength);
      if (wrapped < 0.0f) wrapped += axisLength;

      // Store wrapped position for rendering the dot
      thumb.position = wrapped;

      // === Compute integer voxel shift (periodic) ===
      float laps = delta_world / axisLength;
      int delta_cells = static_cast<int>(std::round(laps * automaton::EL));

      if (thumb.axis == 0) vis_dx = thumb.startOffset[0] + delta_cells;
      if (thumb.axis == 1) vis_dy = thumb.startOffset[1] + delta_cells;
      if (thumb.axis == 2) vis_dz = thumb.startOffset[2] + delta_cells;

      // Optional: keep vis_d* in [0, EL) range
      auto wrap = [](int& v) {
          v = (v % automaton::EL + automaton::EL) % automaton::EL;
      };
      wrap(vis_dx); wrap(vis_dy); wrap(vis_dz);
  }

  /**
   * Projects a 3D point into 2D window coordinates.
   */
  bool projectPoint(const float obj[3],
                    const GLfloat modelview[16],
                    const GLfloat projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY)
  {
    glm::mat4 mv = glm::make_mat4(modelview);
    glm::mat4 pj = glm::make_mat4(projection);
    glm::vec4 vp(viewport[0], viewport[1], viewport[2], viewport[3]);

    glm::vec3 screen = glm::project(glm::vec3(obj[0], obj[1], obj[2]), mv, pj, vp);
    winX = screen.x;
    winY = screen.y;
    return screen.z >= 0.0f && screen.z <= 1.0f;
  }

  /**
   * Distance from point (px,py) to segment from (ax,ay) to (bx,by).
   */
  float distanceToSegment(float px, float py,
                          float ax, float ay,
                          float bx, float by)
  {
    float dx = bx - ax;
    float dy = by - ay;
    float denom = dx*dx + dy*dy;
    if (denom < 1e-8f) return sqrtf((px - ax)*(px - ax) + (py - ay)*(py - ay));

    float t = ((px - ax) * dx + (py - ay) * dy) / denom;
    t = std::max(0.0f, std::min(1.0f, t));
    float cx = ax + t * dx;
    float cy = ay + t * dy;
    return sqrtf((px - cx)*(px - cx) + (py - cy)*(py - cy));
  }

  /**
   * Compute closest point between ray and segment.
   * Returns t in [0,1] along the segment, and sets outDist to the distance.
   */
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

  /**
   * Updates the projection matrix based on radio selection
   */
  void updateProjection()
  {
	  // EARLY EXIT IF NOT INITIALIZED YET
	  if (projectRads.empty()) {
		  // Default to orthographic (safe fallback)
		  float ratio = gViewport[2] > 0 && gViewport[3] > 0
				  ? static_cast<float>(gViewport[2]) / gViewport[3]
	            : 1.0f;

		  const float orthoSize = 0.6f;
		  mProjection_ = glm::ortho(-orthoSize * ratio, orthoSize * ratio,
	                                  -orthoSize, orthoSize,
	                                  0.01f, 100.0f);
		  return;
	  }

      float ratio = static_cast<float>(gViewport[2]) / static_cast<float>(gViewport[3]);

      // Default orthographic scale
      const float orthoSize = 0.6f;  // Used now!

      if (projectRads[0].isSelected())
      {
          // Orthographic projection for 3D
          mProjection_ = glm::ortho(-orthoSize * ratio, orthoSize * ratio,
                                    -orthoSize, orthoSize,
                                    0.01f, 100.0f);
      }
      else if (projectRads[1].isSelected())
      {
          // Perspective projection
          mProjection_ = ProjectionManager::instance().get3DPerspective(65.0f, 0.01f, 100.0f);
      }
  }

}
