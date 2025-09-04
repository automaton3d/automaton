/*
 * Layer.h
 *
 *  Created on: 30 de abr. de 2025
 *      Author: Alexandre
 */

#ifndef MODEL_LAYER_H_
#define MODEL_LAYER_H_

#include "simulation.h"

#define RMAX	1

namespace automaton
{

  class Layer
  {
	unsigned charge;
	unsigned d[EL][EL][EL];
	unsigned p[RMAX];
	unsigned p_bar[RMAX];
  };

}

#endif /* MODEL_LAYER_H_ */
