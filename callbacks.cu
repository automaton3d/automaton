#pragma comment(lib, "C:\\GL\\GLUT\\lib\\x64\\freeglut.lib")

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#define GLEW_STATIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cuda_gl_interop.h>

#include "cglm/vec3.h"

#include "automaton.h"

extern unsigned int shaderProgram;
extern unsigned int vao;
extern vec3 colors[];
extern struct cudaGraphicsResource* cuda_resource;
extern cudaError_t cudaStatus;
extern struct Cell* dev_lattice;
extern float yaw, pitch;

int step = 0;

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        /* Exit on escape key press */
        case '\x1B':
        {
            closeApp();
            exit(EXIT_SUCCESS);
            break;
        }
        case 'A':
        case 'a':
            yaw += 5.0;
            updateCamera();
            break;
        case 'S':
        case 's':
            yaw -= 5.0;
            updateCamera();
            break;
        case 'W':
        case 'w':
            pitch += 3.0;
            updateCamera();
            break;
        case 'D':
        case 'd':
            pitch -= 3.0;
            updateCamera();
            break;
    }
}

void display()
{
    cudaStatus = cudaGraphicsMapResources(1, &cuda_resource, 0);
    if (cudaStatus != cudaSuccess)
    {
        puts("mapping error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    size_t num_bytes;
    void* dev_color;
    cudaStatus = cudaGraphicsResourceGetMappedPointer(&dev_color, &num_bytes, cuda_resource);
    if (cudaStatus != cudaSuccess)
    {
        puts("pointer error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    // "dev_color" points to the GPU memory data store of colorVBO maped by cuda_resource 
    //
    interop << <GRID2, BLOCK2 >> > (dev_lattice, (vec3*)dev_color);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("interop error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    cudaStatus = cudaGraphicsUnmapResources(1, &cuda_resource, 0);
    if (cudaStatus != cudaSuccess)
    {
        puts("unmapping error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_POINTS, 0, 6, SIDE3);
    glBindVertexArray(0);
    //
    // Draw text
    //
    glWindowPos2i(10, 600 - 30);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glColor3f(1, 0, 0);
    char s[12];
    sprintf(s, "Step: %d", step);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*) s);
    //
    glutSwapBuffers();
}

void animation()
{
    commute << <GRID2, BLOCK2 >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("commute error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    /*
    compare << <GRID2, BLOCK2 >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("compare error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    */
    replicate << <GRID2, BLOCK2 >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("replicate error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    /*
    interact << <GRID2, BLOCK2 >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("interact error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    */
    expand << <GRID1, BLOCK1 >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("expand error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    // Generate graphics
    //
    display();
    step++;
}

