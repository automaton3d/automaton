// ============================================================
// projection_manager.cpp - NEW FILE
// ============================================================
#include "projection_manager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void ProjectionManager::setViewport(int width, int height)
{
    if (height == 0) height = 1;

    width_ = width;
    height_ = height;
    aspect_ = static_cast<float>(width) / static_cast<float>(height);

    // Create 2D orthographic projection with TOP-LEFT origin
    // This is what UI developers expect: (0,0) = top-left
    ortho2D_ = glm::ortho(
        0.0f,                    // left
        static_cast<float>(width),   // right
        static_cast<float>(height),  // bottom (actually top in screen coords)
        0.0f                     // top (actually bottom in screen coords)
    );
}

glm::mat4 ProjectionManager::get3DPerspective(float fov, float near, float far) const
{
    return glm::perspective(glm::radians(fov), aspect_, near, far);
}

