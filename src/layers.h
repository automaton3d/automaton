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
#include <array>
#include "vslider.h"
#include "radio.h"
#include "model/simulation.h"
#include "text.h"
#include "projection.h"

namespace automaton
{
  extern unsigned CENTER;
  extern unsigned W_USED;
  extern std::vector<Cell> lattice_curr;
  extern std::vector<std::array<unsigned, 3>> lcenters;
}

namespace framework
{
  using namespace std;
  using namespace automaton;

  extern VSlider vslider;

  class LayerList
  {
  public:
    LayerList(unsigned wDim, int visibleLayers = 25)
      : wDim_(wDim), visibleLayers_(visibleLayers), lastFirstIndex_(-1)
    {
      lastCellCoordinates_.resize(wDim_);
      for (unsigned w = 0; w < wDim_; w++)
      {
    	lastCellCoordinates_[w][0] = CENTER;
    	lastCellCoordinates_[w][1] = CENTER;
    	lastCellCoordinates_[w][2] = CENTER;
      }
      char s[100];
      for (unsigned w = 0; w < wDim_; w++)
      {
        sprintf(s, "Layer %2d", w);
        layers_.push_back(Radio(1700, 120 + 25 * w, s));
      }
      if (!layers_.empty())
        layers_[0].setSelected(true);
    }

    ~LayerList() = default;

    LayerList(const LayerList&) = delete;
    LayerList& operator=(const LayerList&) = delete;

    // Return index of selected layer (-1 if none)
    int getSelected() const
    {
      for (size_t i = 0; i < layers_.size(); ++i)
      {
        if (layers_[i].isSelected())
          return static_cast<int>(i);
      }
      return -1;
    }

    // Update cell position display (with change highlighting)
    void update()
    {
      int endLimit = std::min<int>(visibleLayers_, wDim_);
      for (int w = 0; w < endLimit; w++)
      {

   	    const auto& center = automaton::lcenters[w];
    	Cell &cell = getCell(lattice_curr, center[0], center[1], center[2], w);
    	bool changed = (cell.x[0] != lastCellCoordinates_[w][0] ||
                        cell.x[1] != lastCellCoordinates_[w][1] ||
                        cell.x[2] != lastCellCoordinates_[w][2]);
        glColor3f(changed ? 1.0f : 1.0f,
                  changed ? 0.0f : 1.0f,
                  changed ? 1.0f : 0.0f);

        char s[100];
        sprintf(s, "(%u, %u, %u)", cell.x[0], cell.x[1], cell.x[2]);
        drawString(s, 1780, 120 + 25 * w, 8);
        lastCellCoordinates_[w][0] = cell.x[0];
        lastCellCoordinates_[w][1] = cell.x[1];
        lastCellCoordinates_[w][2] = cell.x[2];
      }
    }

    // Render visible portion of the layer list (with scrolling)
    void render()
    {
      const int first = vslider.getFirstIndex(wDim_, visibleLayers_);
      glDisable(GL_DEPTH_TEST);
      setOrthographicProjection();
      glPushMatrix();
      const int endLimit = std::min<int>(visibleLayers_, static_cast<int>(wDim_));
      for (int i = 0; i < endLimit; ++i)
      {
        const int logicalIndex = i + first;
        if (logicalIndex >= static_cast<int>(wDim_)) break;
        const int displayY = 120 + 25 * i;
        layers_[logicalIndex].drawAt(1680, displayY);
      }
      glPopMatrix();
      resetPerspectiveProjection();
    }

    // Poll mouse click – returns true if a radio was clicked
    bool poll(int xpos, int ypos)
    {
      Radio* clickedOption = nullptr;
      const int first = vslider.getFirstIndex(wDim_, visibleLayers_);
      const int selectedIndex = getSelected();
      Radio* currentSelected = (selectedIndex != -1) ? &layers_[selectedIndex] : nullptr;
      const int endLimit = std::min<int>(visibleLayers_, static_cast<int>(wDim_));
      for (int i = 0; i < endLimit; ++i)
      {
        const int logicalIndex = i + first;
        if (logicalIndex >= static_cast<int>(wDim_)) break;
        const int displayY = 120 + 25 * i;
        if (layers_[logicalIndex].clickedAt(xpos, ypos, 1680, displayY))
        {
          clickedOption = &layers_[logicalIndex];
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
        lastFirstIndex_ = first;
      }
      else if (first != lastFirstIndex_)
      {
        // Auto-select first visible layer when scrolling
        if (first >= 0 && static_cast<unsigned>(first) < wDim_ && first != selectedIndex)
        {
          if (currentSelected) currentSelected->setSelected(false);
            layers_[first].setSelected(true);
        }
        lastFirstIndex_ = first;
      }
      return clickedOption != nullptr;
    }

  private:
    unsigned wDim_;
    int visibleLayers_;
    std::vector<Radio> layers_;
    std::vector<std::array<unsigned, 3>> lastCellCoordinates_;

    int lastFirstIndex_;
  };

}

#endif /* LAYERS_H_ */
