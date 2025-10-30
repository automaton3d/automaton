/*
 * GLutils.h
 *
 *  Created on: 15 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef GLUTILS_H_
#define GLUTILS_H_

#include <string>
#include <GL/gl.h>

namespace framework
{
  void setOrthographicProjection();
  void resetPerspectiveProjection();

  // Helper function: manually project a 3D point to screen space
  bool projectPoint(const float obj[3],
                    const GLdouble modelview[16],
                    const GLdouble projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY);

  double toGLY(double ypos, int windowHeight);

  void drawString8(std::string s, int x, int y);
  void drawString12(const std::string& text, int x, int y);
}


#endif /* GLUTILS_H_ */
