/*
 * rotation.cu
 */


#include "rotation.h"
#include <stdio.h>
#include <math.h>

#include "params.h"
#include "tuple.h"


__device__ static float rotationMatrix[4][4];
__device__ static float inputMatrix[4][1] = {{0.0}, {0.0}, {0.0}, {0.0}};
__device__ static float outputMatrix[4][1] = {{0.0}, {0.0}, {0.0}, {0.0}};

__device__ void multiplyMatrix()
{
	int i;
    for(i = 0; i < 4; i++ )
    {
	int j;
        for(j = 0; j < 1; j++)
        {
            outputMatrix[i][j] = 0;
            int k;
            for(k = 0; k < 4; k++)
                outputMatrix[i][j] += rotationMatrix[i][k] * inputMatrix[k][j];
        }
    }
}

__device__ void setUpRotationMatrix(float angle, float u, float v, float w)
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
 * Calculates spin rotation.
 */
__device__ void rotateSpin(Brick *t)
{
	int distance = (int) modTuple(&t->p2) % (2 * DIAMETER);
	float angle = 2 * M_PI * distance * t->p5 / (2 * DIAMETER);
    float u = (float) t->p2.x, v = (float) t->p2.y, w = (float) t->p2.z;
    inputMatrix[0][0] = t->p4.x;
    inputMatrix[1][0] = t->p4.y;
    inputMatrix[2][0] = t->p4.z;
    inputMatrix[3][0] = 1.0;
    setUpRotationMatrix(angle, u, v, w);
    multiplyMatrix();
    t->p4.x = outputMatrix[0][0];
    t->p4.y = outputMatrix[1][0];
    t->p4.z = outputMatrix[2][0];
}


