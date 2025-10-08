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
  extern Cell lattice_curr[EL][EL][EL][W_DIM];
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
    LayerList()
    {
      char s[100];
      for (unsigned w = 0; w < W_DIM; w++)
      {
        sprintf(s, "Layer %2d", w);
        layers.push_back(Radio(1700, 100 + 25 * w, s));
      }
      layers[0].setSelected(true);

      // Inicializa o rastreador de posição do slider
      lastFirstIndex = -1;
    }

    void update()
    {
	  for (unsigned w = 0; w < W_DIM && w < LAYERS; w++)
	  {
	    Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];
	    if (cell.x[0] != lastPos[w][0] || cell.x[1] != lastPos[w][1] || cell.x[2] != lastPos[w][2])
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
	    lastPos[w][0] = cell.x[0];
	    lastPos[w][1] = cell.x[1];
	    lastPos[w][2] = cell.x[2];
	  }
    }

    void render()
    {
      int first = slider.getFirstIndex(W_DIM);
      glDisable(GL_DEPTH_TEST);
      setOrthographicProjection();
      glPointSize(1);
      glPushMatrix();
      for (int i = startLimit; i < endLimit; ++i)
      {
        int logicalIndex = i + first;
        if (logicalIndex >= W_DIM) break;
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
      int first = slider.getFirstIndex(W_DIM);

      // PASSO 1: Encontra a layer atualmente selecionada (GLOBALMENTE)
      int selectedIndex = this->getSelected();
      Radio *currentSelected = (selectedIndex != -1) ? &layers[selectedIndex] : nullptr;

      // PASSO 2: Percorre APENAS as layers visíveis para encontrar o clique
      for (int i = startLimit; i < endLimit; ++i)
      {
        int logicalIndex = i + first;
        if (logicalIndex >= W_DIM) break;
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
          if (first >= 0 && (unsigned)first < W_DIM && first != selectedIndex)
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
      // Procura a layer selecionada em TODAS as layers (W_DIM)
      int w = -1;
      for (unsigned i = 0; i < W_DIM; i++)
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
      std::vector<Radio> layers;
      unsigned lastPos[W_DIM][3];
      int lastFirstIndex; // Membro privado para rastrear o movimento do slider
  };
}
#endif /* LAYERS_H_ */
