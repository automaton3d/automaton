/*
 * GUI_Panels.cpp (corrigido)
 *
 * Renderização dos grupos de tickboxes e radios.
 */

#include "GUI.h"
#include "globals.h"
#include <vector>

namespace framework
{
  extern Tickbox* tomo;
  extern std::vector<Radio> tomoDirs;

  /**
   * Renderiza os tickboxes de dados 3D.
   */
  void GUIrenderer::render3Dboxes()
  {
    for (Tickbox& checkbox : data3D)
    {
      checkbox.setFontScale(0.6f);
      checkbox.draw(*textRenderer, gViewport[2], gViewport[3]);
    }
  }

  /**
   * Renderiza os tickboxes de delays.
   */
  void GUIrenderer::renderDelays()
  {
    for (Tickbox& checkbox : delays)
    {
      checkbox.setFontScale(0.6f);
      checkbox.draw(*textRenderer, gViewport[2], gViewport[3]);
    }
  }

  /**
   * Renderiza os radios de viewpoints.
   */
  void GUIrenderer::renderViewpointRadios()
  {
    for (Radio& radio : views)
    {
      radio.setFontScale(0.6f);
      radio.draw(*textRenderer, gViewport[2], gViewport[3]);
    }
  }

  /**
   * Renderiza os radios de projeção.
   */
  void GUIrenderer::renderProjectionRadios()
  {
    for (Radio& radio : projection)
    {
      radio.setFontScale(0.6f);
      radio.draw(*textRenderer, gViewport[2], gViewport[3]);
    }
  }

  /**
   * Renderiza os radios de tomografia.
   */
  void GUIrenderer::renderTomoRadios()
  {
    if (tomo && tomo->getState()) {
        for (Radio& radio : tomoDirs) {
            radio.setFontScale(0.6f);
            radio.draw(*textRenderer, gViewport[2], gViewport[3]);
        }
    }
  }

} // namespace framework