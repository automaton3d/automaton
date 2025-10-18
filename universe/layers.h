/*
 * layers.h
 *
 * Manages the list of 4D layers displayed in the visualization window.
 * Each layer corresponds to a W-slice of the cellular automaton lattice.
 */
#ifndef LAYERS_H_
#define LAYERS_H_

#include <vector>
#include <cstdio>
#include "radio.h"
#include "model/simulation.h"
#include "GLutils.h"
#include "slider.h"

#define LAYERS 25  // Maximum number of visible layers at once

namespace automaton
{
  extern unsigned CENTER;
  extern unsigned W_DIM;
  extern std::vector<Cell> lattice_curr;
}

namespace framework
{
  using namespace std;
  using namespace automaton;

  extern void drawString8(string s, int x, int y);
  extern LayerSlider slider;

  class LayerList
  {
    public:
      LayerList(unsigned wDim) : wDim(wDim), lastFirstIndex(-1)
      {
        lastPositions.resize(wDim);

        // Initialize lastPositions with zeros
        for (unsigned w = 0; w < wDim; w++)
        {
          lastPositions[w][0] = 0;
          lastPositions[w][1] = 0;
          lastPositions[w][2] = 0;
        }

        // Create a radio button for each layer
        char s[100];
        for (unsigned w = 0; w < wDim; w++)
        {
          sprintf(s, "Layer %2d", w);
          layers.push_back(Radio(1700, 100 + 25 * w, s));
        }
        layers[0].setSelected(true);
      }

      ~LayerList() {}

      // Disable copy operations to avoid pointer duplication issues
      LayerList(const LayerList&) = delete;
      LayerList& operator=(const LayerList&) = delete;

      /**
       * @brief Updates the list display.
       *
       * Draws the coordinates of the central cell for each visible layer
       * and changes its color if the coordinates have changed since
       * the last update (magenta if updated, yellow if static).
       */
      void update()
      {
        for (unsigned w = 0; w < wDim && w < LAYERS; w++)
        {
          Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
          if (cell.x[0] != lastPositions[w][0] ||
              cell.x[1] != lastPositions[w][1] ||
              cell.x[2] != lastPositions[w][2])
          {
            glColor3f(1.0f, 0.0f, 1.0f);
          }
          else
          {
            glColor3f(1.0f, 1.0f, 0.0f);
          }
          char s[100];
          sprintf(s, "(%u, %u, %u)", cell.x[0], cell.x[1], cell.x[2]);
          drawString8(s, 1780, 100 + 25 * w);
          lastPositions[w][0] = cell.x[0];
          lastPositions[w][1] = cell.x[1];
          lastPositions[w][2] = cell.x[2];
        }
      }

      /**
       * @brief Renders all visible radio buttons for the layers.
       */
      void render()
      {
        int first = slider.getFirstIndex(wDim);
        glDisable(GL_DEPTH_TEST);
        setOrthographicProjection();
        glPointSize(1);
        glPushMatrix();
        for (int i = startLimit; i < endLimit; ++i)
        {
          int logicalIndex = i + first;
          if (logicalIndex >= (int)wDim) break;
          int displayY = 100 + 25 * i;
          layers[logicalIndex].drawAt(1680, displayY);
        }
        glFlush();
        glPopMatrix();
        resetPerspectiveProjection();
      }

      /**
       * @brief Handles mouse interaction with the layer list.
       *
       * The poll() function determines whether the user clicked on a specific
       * visible layer or scrolled using the slider. It ensures that:
       *
       * - If a layer button is clicked, it becomes the new active layer.
       * - If the slider moves and reveals a new first visible layer, that layer
       *   is automatically selected when the previous one scrolls off-screen.
       * - If neither a click nor slider movement occurs, the current selection
       *   is maintained.
       */
      void poll(int xpos, int ypos)
      {
        Radio *clickedOption = nullptr;
        int first = slider.getFirstIndex(wDim);

        // Step 1: Identify the currently selected layer (globally)
        int selectedIndex = this->getSelected();
        Radio *currentSelected = (selectedIndex != -1) ? &layers[selectedIndex] : nullptr;

        // Step 2: Check only visible layers for clicks
        for (int i = startLimit; i < endLimit; ++i)
        {
          unsigned logicalIndex = i + first;
          if (logicalIndex >= wDim) break;
          int displayY = 100 + 25 * i;

          // Detect click in visible area
          if (xpos >= 1700 - 35 && xpos <= 1700 + 100 &&
              ypos >= displayY && ypos <= displayY + 40)
          {
            clickedOption = &layers[logicalIndex];
          }
        }

        // Step 3: Update selection logic
        if (clickedOption)
        {
          // User clicked a layer different from the current one
          if (clickedOption != currentSelected)
          {
            if (currentSelected) currentSelected->setSelected(false);
            clickedOption->setSelected(true);
          }
          this->lastFirstIndex = first;
        }
        else if (first != this->lastFirstIndex)
        {
          // Slider moved: select the first visible layer
          if (first >= 0 && (unsigned)first < wDim && first != selectedIndex)
          {
            if (currentSelected) currentSelected->setSelected(false);
            layers[first].setSelected(true);
          }
          this->lastFirstIndex = first;
        }
      }

      /**
       * @brief Returns the index of the currently selected layer.
       */
      int getSelected()
      {
        int w = -1;
        for (unsigned i = 0; i < wDim; i++)
        {
          if (layers[i].isSelected())
          {
            w = i;
            break;
          }
        }
        return w;
      }

      int startLimit = 0;
      int endLimit = LAYERS;

    private:
      unsigned wDim;
      std::vector<Radio> layers;
      std::vector<std::array<unsigned, 3>> lastPositions;
      int lastFirstIndex;
  };
}

#endif /* LAYERS_H_ */
