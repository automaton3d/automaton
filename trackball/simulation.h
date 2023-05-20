/*
 * simulation.h
 *
 *  Created on: 01/09/2016
 *      Author: Alexandre
 */

#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <stdio.h>
#include "plot3d.h"
#include <windows.h>
#include <pthread.h>
#include <stdio.h>

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

/// Functions ///

void *AutomatonLoop();
void DeleteAutomaton();
void copy();
void update();
void initAutomaton();
void initScreen();
void *work(void * parm);
void model(int phase);

#endif /* SIMULATION_H_ */
