/*
 * text.h
 */

#ifndef TEXT_H_
#define TEXT_H_

#include <string>
#include <GL/gl.h>

namespace framework
{
  using namespace std;

  void drawString(string s, int x, int y, int size);
  void drawBoldText(const string& text, int x, int y, float offset = 0.5f);
  void render2Dstring(float x, float y, void *font, const char *string);
  void render3Dstring(float x, float y, float z, void *font, const char *string);
}

#endif /* TEXT_H_ */
