/*
 * trackball.h
 *
 *  Created on: 23 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef TRACKBALL_H_
#define TRACKBALL_H_

typedef struct Trackball
{
    float p[4];
    float q[4];
    float* drag_startVector;
    RECT box;
    int slideID;
    int dragged;
    int smooth;
    char* limitAxis;
    float angleChange;
    float* axis;
    float angle;
    float oldTime;
    float curTime;
    void (*events)(float **);

	int rotH, rotV;

	int beginx, beginy;
	float rotation[4];

} Trackball;

#endif /* TRACKBALL_H_ */
