/*
 * GUI_Panels.cpp (old)
 */

#include "GUI.h"

namespace framework
{

  extern vector<Radio> tomoDirs;

  void GUIrenderer::render3Dboxes()
  {
    for (Tickbox& checkbox : data3D)
      checkbox.draw();
  }

  void GUIrenderer::renderDelays()
  {
    for (Tickbox& checkbox : delays)
      checkbox.draw();
  }

  void GUIrenderer::renderViewpointRadios()
  {
    for (Radio& radio : views)
      radio.draw();
  }

  void GUIrenderer::renderProjectionRadios()
  {
    for (Radio& radio : projection)
      radio.draw();
  }

  void GUIrenderer::renderTomoRadios()
  {
    for (Radio& radio : tomoDirs)
      radio.draw();
  }

}
