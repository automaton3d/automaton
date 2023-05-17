/*
 * arcball.h
 *
 *  Created on: 15 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef ARCBALL_H_
#define ARCBALL_H_

void initArcball();
void mousedown(int x, int y);
void mousemove(int x, int y);
void mouseup();
void draw();
void rotateVectorX(float *rotation, float *object, float *rotated);
void mulQuat(float *a, float *b, float *c);

#endif /* ARCBALL_H_ */
