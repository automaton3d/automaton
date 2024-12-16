/*
 * renderer.h
 *
 * Declares the top level renderer routines.
 * (GUIrenderer is subordinated to it)
 */

#ifndef RSMZ_RENDERER_H
#define RSMZ_RENDERER_H

#include "camera.h"
#include <GL/gl.h>

namespace framework
{
  class Renderer
  {
    public:
      Renderer();
      virtual ~Renderer();

      virtual void render() = 0;

      void setCamera(Camera *c);
      const Camera* getCamera();

    protected:
      Camera *mCamera;

  }; // end class Renderer

} // end namespace rsmz

#endif // RSMZ_RENDERER_H
