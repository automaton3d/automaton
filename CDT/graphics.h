/*
 * gavin.h
 *
 *  Created on: 26 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <windows.h>

#define TRACKBALLSIZE  (0.7f)
#define RENORMCOUNT 97

HWND CreateCheckBox(HWND hwndParent, int x, int y, int width, int height, int id, LPCWSTR text);
void trackball(float q[4], float p1x, float p1y, float p2x, float p2y);
void mul(float *r, float *a, float *b);
void normalize_quat(float q[4]);
void rotateVector(float *rotated, float *rotation, float *object);
void add_quats(float q1[4], float q2[4], float dest[4]);
void keyboard(UINT msg, WPARAM wparam, LPARAM lparam);
void updateBuffer();
float tb_project_to_sphere(float, float, float);
void scaleQuat(float quat[4]);

#endif /* GRAPHICS_H_ */
