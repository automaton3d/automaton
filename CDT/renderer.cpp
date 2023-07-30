#include "renderer.h"

namespace framework
{

Renderer::Renderer() : mCamera(0)
{
}

Renderer::~Renderer()
{
}

const Camera* Renderer::getCamera()
{
    return mCamera;
}

void Renderer::setCamera(Camera *c)
{
  mCamera = c;
}

} // end namespace rsmz
