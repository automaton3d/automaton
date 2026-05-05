#include "projection_manager.h"
#include <glm/gtc/matrix_transform.hpp>

// NÃO use gViewport aqui — isso já foi centralizado
// O estado agora vem do próprio ProjectionManager

void ProjectionManager::setViewport(int width, int height)
{
    width_ = width;
    height_ = height;
    aspect_ = (height_ > 0) ? (float)width_ / height_ : 1.0f;

    // Ortho 2D com origem no topo-esquerda
    ortho2D_ = glm::ortho(
        0.0f, (float)width_,
        (float)height_, 0.0f,
        -1.0f, 1.0f
    );
}

glm::mat4 ProjectionManager::get3DPerspective(float fov, float near, float far) const
{
    return glm::perspective(
        glm::radians(fov),
        aspect_,
        near,
        far
    );
}