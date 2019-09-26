/*
 * interaction.h
 *
 *  Created on: 20 de set de 2019
 *      Author: Alexandre
 */

#ifndef INTERACTION_H_
#define INTERACTION_H_

#include "brick.h"

void axiomUxG(Brick *U, Brick *G);
void axiomPxG(Brick *P, Brick *G);
void axiomUxU(Brick *U1, Brick *U2);
void axiomUxP(Brick *U, Brick *P);
void axiomPxP(Brick *P1, Brick *P2);

#endif /* INTERACTION_H_ */
