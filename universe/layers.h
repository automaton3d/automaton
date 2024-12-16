/*
 * layers.h
 *
 *  Created on: 15 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef LAYERS_H_
#define LAYERS_H_

#include <vector>
#include <cstdio>
#include "radio.h"
#include "model/simulation.h"
#include "GLutils.h"

#define LAYERS	25	// max layers shown

namespace framework
{
  using namespace std;
  using namespace automaton;

  extern void drawString8(string s, int x, int y);

  class LayerList
  {
    public:
    LayerList()
    {
      char s[100];
      for (unsigned w = 0; w < W_DIM; w++)
      {
        sprintf(s, "Layer %2d", w);
        layers.push_back(Radio(1700, 100 + 25 * w, s));
      }
      layers[0].setSelected(true);
    }

    void update()
    {
      // Update positions
	  for (unsigned w = 0; w < W_DIM && w < LAYERS; w++)
	  {
	    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
	    if (cell.pos[0] != lastPos[w][0] || cell.pos[1] != lastPos[w][1] || cell.pos[2] != lastPos[w][2])
	    {
	      glColor3f(1.0f, 0.0f, 1.0f);
	    }
	    else
	    {
	      glColor3f(1.0f, 1.0f, 0.0f);
	    }
        char s[100];
	    sprintf(s, "(%u, %u, %u)", cell.pos[0], cell.pos[1], cell.pos[2]);
	    drawString8(s, 1800, 100 + 25 * w);
	    lastPos[w][0] = cell.pos[0];
	    lastPos[w][1] = cell.pos[1];
	    lastPos[w][2] = cell.pos[2];
	  }
    }

    void render()
    {
      glDisable(GL_DEPTH_TEST);
      setOrthographicProjection();
      glPointSize(1);
      glPushMatrix();
      for (int i = startLimit; i < endLimit; ++i)
          layers[i].draw();
      glFlush();
      glPopMatrix();
      resetPerspectiveProjection();
    }

    void poll(int xpos, int ypos)
    {
      Radio *option = nullptr;
      Radio *last = nullptr;
      // Identify the selected option and the previously selected layer
      for (Radio& layer : layers)
      {
        if (xpos >= layer.getX()-35 && xpos <= layer.getX() + 100 &&
            ypos >= layer.getY() && ypos <= layer.getY() + 40)
        {
          option = &layer; // Found the layer at the given position
        }
        if (layer.isSelected())
        {
          last = &layer; // Track the currently selected layer
        }
      }
      // Update selection if a new option is selected
      if (option && option != last)
      {
        if (last) // Deselect the previously selected layer, if any
          last->setSelected(false);
        option->setSelected(true); // Select the new option
      }
    }

    int getSelected()
    {
      // Find the selected layer
      int w = -1; // Initialize to -1 to indicate no layer selected
      for (unsigned i = 0; i < W_DIM; i++)
      {
        if (layers[i].isSelected())
        {
          w = i; // Set the selected layer
          break; // Exit loop once the layer is found
        }
      }
      return w;
    }

    private:
      int startLimit = 0;  // Starting index (inclusive)
      int endLimit = LAYERS; // Ending index (exclusive)
      std::vector<Radio> layers;
      unsigned lastPos[W_DIM][3];

  };

}
#endif /* LAYERS_H_ */
