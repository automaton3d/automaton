/*
 * trackball.h
 *
 *  Created on: 27 de fev. de 2023
 *      Author: Alexandre
 */

#ifndef TRACKBALL_H_
#define TRACKBALL_H_

/*
 * trackball.h
 * A virtual trackball implementation
 * Written by Gavin Bell for Silicon Graphics, November 1988.
 */

void vzero(float *v);

void vset(float *v, float x, float y, float z);

void vsub(const float *src1, const float *src2, float *dst);

void vcopy(const float *v1, float *v2);

void vcross(const float *v1, const float *v2, float *cross);

float vlength(const float *v);

void vscale(float *v, float div);

void vnormal(float *v);

float vdot(const float *v1, const float *v2);

void vadd(const float *src1, const float *src2, float *dst);


/*
 * Pass the x and y coordinates of the last and current positions of
 * the mouse, scaled so they are from (-1.0 ... 1.0).
 *
 * The resulting rotation is returned as a quaternion rotation in the
 * first paramater.
 */
void trackball(float q[4], float p1x, float p1y, float p2x, float p2y);

/*
 * Given two quaternions, add them together to get a third quaternion.
 * Adding quaternions to get a compound rotation is analagous to adding
 * translations to get a compound translation.  When incrementally
 * adding rotations, the first argument here should be the new
 * rotation, the second and third the total rotation (which will be
 * over-written with the resulting new total rotation).
 */
void add_quats(float q1[4], float q2[4], float dest[4]);

/*
 * A useful function, builds a rotation matrix in Matrix based on
 * given quaternion.
 */
void build_rotmatrix(float m[4][4], float q[4]);

/*
 * This function computes a quaternion based on an axis (defined by
 * the given vector) and an angle about which to rotate.  The angle is
 * expressed in radians.  The result is put into the third argument.
 */
void axis_to_quat(float a[3], float phi, float q[4]);


#endif /* TRACKBALL_H_ */
