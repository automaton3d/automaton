/*
 * GLutils.cpp
 *
 * Auxiliary text draing routines.
 */
#include "GUIrenderer.h"

namespace framework
{
  using namespace std;

  void renderCenterBox(const char* text)
  {
      GLint viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);

      int viewportWidth = viewport[2];
      int viewportHeight = viewport[3];

      // Calculate text dimensions dynamically
      int textWidth = strlen(text) * 10;
      int textHeight = 10;

      // Center the text in the viewport
      int textX = (viewportWidth - textWidth) / 2;
      int textY = (viewportHeight + textHeight) / 2;

      // Padding for the rectangle
      int paddingX = 15;
      int paddingY = 10;

      // Rectangle dimensions and position
      int rectX = textX - paddingX;
      int rectY = textY - textHeight - paddingY;
      int rectWidth = textWidth + 2 * paddingX;
      int rectHeight = textHeight + 2 * paddingY;

      // Draw the text
      glColor3f(1.0f, 1.0f, 1.0f); // White color for text
      drawString8(text, textX, textY); // Render the text at the calculated position

      // Draw the rectangle around the text
      glColor3f(1.0f, 1.0f, 1.0f); // White color for rectangle
      glLineWidth(2);
      glBegin(GL_LINE_LOOP);
      glVertex2i(rectX, rectY);
      glVertex2i(rectX + rectWidth, rectY);
      glVertex2i(rectX + rectWidth, rectY + rectHeight);
      glVertex2i(rectX, rectY + rectHeight);
      glEnd();
  }

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

}
