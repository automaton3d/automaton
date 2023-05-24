/*
 * mouse.h
 *
 *  Created on: 19 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef MOUSE_H_
#define MOUSE_H_

void projectP(float x, float y, float *res);
void mulQuat(float *a, float *b, float *c);
void fromBetweenVector(float *a, float *b, float *c);
void mousedown(int x, int y);
void mousemove(int x, int y);
void mouseup();
void rotateVectorX(float *rotation, float *object, float *rotated);
void m4MultVec(float *vf, float mat[4][4], float *vi);
void projectxxxx(float *vec, float x, float y);
void mouse(UINT msg, WPARAM wparam, LPARAM lparam);
void project2(float *vec, float x, float y);

#endif /* MOUSE_H_ */
