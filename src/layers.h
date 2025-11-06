/*
 * layers.h
 *
 * Manages the list of 4D layers displayed in the visualization window.
 * Each layer corresponds to a W-slice of the cellular automaton lattice.
 */

#ifndef LAYERS_H_
#define LAYERS_H_

#include <text.h>
#include <vector>
#include <cstdio>
#include <array>
#include "vslider.h"
#include "radio.h"
#include "model/simulation.h"

namespace automaton
{
  extern unsigned CENTER;
  extern unsigned W_USED;
  extern std::vector<Cell> lattice_curr;
}

namespace framework
{
  using namespace std;
  using namespace automaton;

  extern void drawString8(string s, int x, int y);
  extern VSlider vslider;

  class LayerList
  {
    public:
      LayerList(unsigned wDim, int visibleLayers = 25)
        : wDim(wDim), visibleLayers(visibleLayers), lastFirstIndex(-1)
      {
        lastPositions.resize(wDim);
        for (unsigned w = 0; w < wDim; w++)
        {
          lastPositions[w][0] = 0;
          lastPositions[w][1] = 0;
          lastPositions[w][2] = 0;
        }

        char s[100];
        for (unsigned w = 0; w < wDim; w++)
        {
          sprintf(s, "Layer %2d", w);
          layers.push_back(Radio(1700, 120 + 25 * w, s));
        }
        if (!layers.empty())
          layers[0].setSelected(true);
      }

      ~LayerList() = default;

      LayerList(const LayerList&) = delete;
      LayerList& operator=(const LayerList&) = delete;

      // -------------------------------------------------------------
      // Return index of selected layer (-1 if none)
      // -------------------------------------------------------------
      int getSelected() const
      {
          for (size_t i = 0; i < layers.size(); ++i)
          {
              if (layers[i].isSelected())
                  return static_cast<int>(i);
          }
          return -1;
      }

      // -------------------------------------------------------------
      // Update cell position display (with change highlighting)
      // -------------------------------------------------------------
      void update()
      {
        int endLimit = std::min<int>(visibleLayers, wDim);
        for (int w = 0; w < endLimit; w++)
        {
          Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
          bool changed = (cell.x[0] != lastPositions[w][0] ||
                          cell.x[1] != lastPositions[w][1] ||
                          cell.x[2] != lastPositions[w][2]);

          glColor3f(changed ? 1.0f : 1.0f,
                    changed ? 0.0f : 1.0f,
                    changed ? 1.0f : 0.0f);

          char s[100];
          sprintf(s, "(%u, %u, %u)", cell.x[0], cell.x[1], cell.x[2]);
          drawString8(s, 1780, 120 + 25 * w);

          lastPositions[w][0] = cell.x[0];
          lastPositions[w][1] = cell.x[1];
          lastPositions[w][2] = cell.x[2];
        }
      }

      // -------------------------------------------------------------
      // Render visible portion of the layer list (with scrolling)
      // -------------------------------------------------------------
      void render()
      {
        const int first = vslider.getFirstIndex(wDim, visibleLayers);

        glDisable(GL_DEPTH_TEST);
        setOrthographicProjection();
        glPushMatrix();

        const int endLimit = std::min<int>(visibleLayers, static_cast<int>(wDim));
        for (int i = 0; i < endLimit; ++i)
        {
            const int logicalIndex = i + first;
            if (logicalIndex >= static_cast<int>(wDim)) break;

            const int displayY = 120 + 25 * i;
            layers[logicalIndex].drawAt(1680, displayY);
        }

        glPopMatrix();
        resetPerspectiveProjection();
      }

      // -------------------------------------------------------------
      // Poll mouse click – returns true if a radio was clicked
      // -------------------------------------------------------------
      bool poll(int xpos, int ypos)
      {
          Radio* clickedOption = nullptr;
          const int first = vslider.getFirstIndex(wDim, visibleLayers);

          const int selectedIndex = getSelected();
          Radio* currentSelected = (selectedIndex != -1) ? &layers[selectedIndex] : nullptr;

          const int endLimit = std::min<int>(visibleLayers, static_cast<int>(wDim));
          for (int i = 0; i < endLimit; ++i)
          {
              const int logicalIndex = i + first;
              if (logicalIndex >= static_cast<int>(wDim)) break;

              const int displayY = 120 + 25 * i;

              // Use on-screen draw position for accurate hit-testing
              if (layers[logicalIndex].clickedAt(xpos, ypos, 1680, displayY))
              {
                  clickedOption = &layers[logicalIndex];
                  break;
              }
          }

          if (clickedOption)
          {
              if (clickedOption != currentSelected)
              {
                  if (currentSelected) currentSelected->setSelected(false);
                  clickedOption->setSelected(true);
              }
              lastFirstIndex = first;
          }
          else if (first != lastFirstIndex)
          {
              // Auto-select first visible layer when scrolling
              if (first >= 0 && static_cast<unsigned>(first) < wDim && first != selectedIndex)
              {
                  if (currentSelected) currentSelected->setSelected(false);
                  layers[first].setSelected(true);
              }
              lastFirstIndex = first;
          }

          return clickedOption != nullptr;
      }

    private:
      unsigned wDim;
      int visibleLayers;
      std::vector<Radio> layers;
      std::vector<std::array<unsigned, 3>> lastPositions;
      int lastFirstIndex;
  };
}

#endif /* LAYERS_H_ */
