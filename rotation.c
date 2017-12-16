/*
 * rotation.c
 *
 *  Created on: 19 de fev de 2017
 *      Author: Alexandre
 */


#include <stdio.h>
#include <math.h>

#include "params.h"
#include "tuple.h"

Tuple points;

float rotationMatrix[4][4];
float inputMatrix[4][1] = {{0.0}, {0.0}, {0.0}, {0.0}};
float outputMatrix[4][1] = {{0.0}, {0.0}, {0.0}, {0.0}};

void multiplyMatrix()
{
    for(int i = 0; i < 4; i++ )
        for(int j = 0; j < 1; j++)
        {
            outputMatrix[i][j] = 0;
            for(int k = 0; k < 4; k++)
                outputMatrix[i][j] += rotationMatrix[i][k] * inputMatrix[k][j];
        }
}

void setUpRotationMatrix(float angle, float u, float v, float w)
{
    float L = (u*u + v * v + w * w);
    angle = angle * M_PI / 180.0f; //converting to radian value
    float u2 = u * u;
    float v2 = v * v;
    float w2 = w * w;

    rotationMatrix[0][0] = (u2 + (v2 + w2) * cos(angle)) / L;
    rotationMatrix[0][1] = (u * v * (1 - cos(angle)) - w * sqrt(L) * sin(angle)) / L;
    rotationMatrix[0][2] = (u * w * (1 - cos(angle)) + v * sqrt(L) * sin(angle)) / L;
    rotationMatrix[0][3] = 0.0;

    rotationMatrix[1][0] = (u * v * (1 - cos(angle)) + w * sqrt(L) * sin(angle)) / L;
    rotationMatrix[1][1] = (v2 + (u2 + w2) * cos(angle)) / L;
    rotationMatrix[1][2] = (v * w * (1 - cos(angle)) - u * sqrt(L) * sin(angle)) / L;
    rotationMatrix[1][3] = 0.0;

    rotationMatrix[2][0] = (u * w * (1 - cos(angle)) - v * sqrt(L) * sin(angle)) / L;
    rotationMatrix[2][1] = (v * w * (1 - cos(angle)) + u * sqrt(L) * sin(angle)) / L;
    rotationMatrix[2][2] = (w2 + (u2 + v2) * cos(angle)) / L;
    rotationMatrix[2][3] = 0.0;

    rotationMatrix[3][0] = 0.0;
    rotationMatrix[3][1] = 0.0;
    rotationMatrix[3][2] = 0.0;
    rotationMatrix[3][3] = 1.0;
}

/*
 * Function called by main program.
 */
void rotateSpin(Tuple *t, int n)
{
    float angle = 2 * M_PI / SIDE;
    float u = (float) t->x, v = (float) t->y, w = (float) t->z;
    puts("Enter the initial point you want to transform:");
    points.x = 1; points.y = 1; points.z = 1;
    inputMatrix[0][0] = points.x;
    inputMatrix[1][0] = points.y;
    inputMatrix[2][0] = points.z;
    inputMatrix[3][0] = 1.0;

    puts("Enter axis vector: ");
    u = 1; v = 2; w = 3;

    puts("Enter the rotating angle in degree: ");
    angle = 0.2;

    setUpRotationMatrix(angle, u, v, w);
    multiplyMatrix();
    t->x = outputMatrix[0][0];
    t->y = outputMatrix[1][0];
    t->z = outputMatrix[2][0];
}
