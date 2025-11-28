/*
 * projection.h (adapted for GLFW + GLAD)
 */

#ifndef PROJECTION_H_
#define PROJECTION_H_

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace framework
{
  struct AxisThumb {
    bool active = false;
    int axis = -1;
    float position = 0.0f;
    bool dragging = false;
    float initialPosition = 0.0f;
    int startOffset[3] = {0, 0, 0};
  };

  extern AxisThumb thumb;

  // Configura projeção ortográfica
  void setOrthographicProjection();

  // Restaura projeção perspectiva
  void resetPerspectiveProjection();

  // Projeta um ponto 3D para espaço de janela
  bool projectPoint(const float obj[3],
                    const GLfloat modelview[16],
                    const GLfloat projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY);

  // Converte mouse para mundo e restringe ao eixo ativo
  void map3DTo2D(const glm::vec3& pt,
                 double mouseX, double mouseY,
                 float depth,
                 float axisLength);

  // Distância 2D de ponto a segmento (hit-testing de eixo)
  float distanceToSegment(float px, float py,
                          float ax, float ay,
                          float bx, float by);

  // Ponto mais próximo em segmento para um raio
  float closestPointOnSegmentToRay(const glm::vec3& rayO, const glm::vec3& rayD,
                                   const glm::vec3& segA, const glm::vec3& segB,
                                   float& outDist);

} // namespace framework

#endif /* PROJECTION_H_ */
