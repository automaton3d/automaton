/*
 * GLutils.cpp
 *
 * Auxiliary text draing routines.
 */
#include <GUI.h>

namespace framework
{
  using namespace std;

  void render2Dstring(float x, float y, void *font, const char *string)
  {
    glRasterPos2f(x, y);  // Set raster position where text will start
    for (int i = 0; i < string[i]; i++)
      glutBitmapCharacter(font, string[i]);
  }

  /*
   * Renders 3D strings.
   */
  void render3Dstring(float x, float y, float z, void *font, const char *string)
  {
    glRasterPos3f(x, y, z);  // Set raster position in 3D space
    for (int i = 0; string[i] != '\0'; i++)  // Loop through the string
      glutBitmapCharacter(font, string[i]);
  }

  void drawString8(string s, int x, int y)
  {
    glRasterPos2f(x, y);
    for (int i = 0; s[i] != '\0'; ++i)
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
  }

  /*
   * Draw normal text.
   */
  void drawString12(const string& text, int x, int y)
  {
    glRasterPos2i(x, y);
    for (char c : text)
    {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c); // Use GLUT for bitmap fonts
    }
  }

  /*
   * Draw bold text.
   */
  void drawBoldText(const string& text, int x, int y, float offset)
  {
    for (float dx = -offset; dx <= offset; dx += offset)
    {
      for (float dy = -offset; dy <= offset; dy += offset)
      {
        glRasterPos2i(x + dx, y + dy);
        for (char c : text)
        {
          glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }
      }
    }
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

  // Helper function: manually project a 3D point to screen space
  bool projectPoint(const float obj[3],
                    const GLdouble modelview[16],
                    const GLdouble projection[16],
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

  double toGLY(double ypos, int windowHeight)
  {
    return windowHeight - ypos;
  }

}
