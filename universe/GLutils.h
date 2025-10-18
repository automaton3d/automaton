/*
 * GLutils.h
 *
 *  Created on: 15 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef GLUTILS_H_
#define GLUTILS_H_

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

}

#endif /* GLUTILS_H_ */
