/*
 * layers.h
 *
 * Created on: 15 de dez. de 2024
 * Author: Alexandre
 */
#ifndef LAYERS_H_
#define LAYERS_H_
#include <vector>
#include <cstdio>
#include "radio.h"
#include "model/simulation.h"
#include "GLutils.h"
#include "slider.h"
#define LAYERS	25	// max layers shown

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

      // Inicializa lastPos com zeros
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
        layers.push_back(Radio(1700, 100 + 25 * w, s));
      }
      layers[0].setSelected(true);
    }

    ~LayerList()
    {
    }

    // Desabilita cópia para evitar problemas com ponteiros
    LayerList(const LayerList&) = delete;
    LayerList& operator=(const LayerList&) = delete;

    void update()
    {
	  for (unsigned w = 0; w < wDim && w < LAYERS; w++)
	  {
	    Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
	    if (cell.x[0] != lastPositions[w][0] || cell.x[1] != lastPositions[w][1] || cell.x[2] != lastPositions[w][2])
	    {
	      glColor3f(1.0f, 0.0f, 1.0f);
	    }
	    else
	    {
	      glColor3f(1.0f, 1.0f, 0.0f);
	    }
        char s[100];
	    sprintf(s, "(%u, %u, %u)", cell.x[0], cell.x[1], cell.x[2]);
	    drawString8(s, 1800, 100 + 25 * w);
	    lastPositions[w][0] = cell.x[0];
  	    lastPositions[w][1] = cell.x[1];
   	    lastPositions[w][2] = cell.x[2];
	  }
    }

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
        layers[logicalIndex].drawAt(1700, displayY);
      }
      glFlush();
      glPopMatrix();
      resetPerspectiveProjection();
    }

    // Rotina poll() CORRIGIDA: Usa getSelected() para rastrear o estado global.
    void poll(int xpos, int ypos)
    {
      Radio *clickedOption = nullptr;
      int first = slider.getFirstIndex(wDim);

      // PASSO 1: Encontra a layer atualmente selecionada (GLOBALMENTE)
      int selectedIndex = this->getSelected();
      Radio *currentSelected = (selectedIndex != -1) ? &layers[selectedIndex] : nullptr;

      // PASSO 2: Percorre APENAS as layers visíveis para encontrar o clique
      for (int i = startLimit; i < endLimit; ++i)
      {
        unsigned logicalIndex = i + first;
        if (logicalIndex >= wDim) break;
        int displayY = 100 + 25 * i;

        // Verifica se houve clique na área de visualização atual
        if (xpos >= 1700 - 35 && xpos <= 1700 + 100 &&
            ypos >= displayY && ypos <= displayY + 40)
        {
          clickedOption = &layers[logicalIndex];  // Layer clicada
        }
      }

      // PASSO 3: Lógica de Atualização da Seleção

      // Prioridade 1: O usuário clicou explicitamente em uma layer
      if (clickedOption)
      {
          // Atualiza a seleção apenas se a layer clicada for diferente da selecionada
          if (clickedOption != currentSelected)
          {
              if (currentSelected) currentSelected->setSelected(false);
              clickedOption->setSelected(true);
          }
          this->lastFirstIndex = first; // Reinicia o rastreador de arrasto
      }
      // Prioridade 2: O slider moveu (primeiro índice visível mudou)
      else if (first != this->lastFirstIndex)
      {
          // Se o novo índice principal (first) é diferente do selecionado
          if (first >= 0 && (unsigned)first < wDim && first != selectedIndex)
          {
              // Desseleciona a anterior (que pode estar fora da tela)
              if (currentSelected) currentSelected->setSelected(false);
              // Seleciona a nova primeira layer visível
              layers[first].setSelected(true);
          }
          // Atualiza o rastreador de posição do slider
          this->lastFirstIndex = first;
      }
      // Se não houve clique nem movimento do slider, a seleção é mantida.
    }

    int getSelected()
    {
      // Procura a layer selecionada em TODAS as layers (wDim)
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
      std::vector<std::array<unsigned, 3>> lastPositions; // Vector of arrays for dynamic last positions
      int lastFirstIndex;
  };
}
#endif /* LAYERS_H_ */
