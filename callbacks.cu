#pragma comment(lib, "C:\\GL\\GLUT\\lib\\x64\\freeglut.lib")

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#define GLEW_STATIC

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cuda_gl_interop.h>

#include "cglm/vec3.h"
#include "automaton.cuh"

extern unsigned int shaderProgram, axesProgram;
extern unsigned int gridVAO, axesVAO;
extern struct cudaGraphicsResource* cuda_resource;
extern cudaError_t cudaStatus;
extern Cell *host_lattice, *dev_lattice;
extern DWORD start;
int step = 0;

boolean flag;
int sublattice = FLOOR;

void display()
{
    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //
    glUseProgram(axesProgram);
    glBindVertexArray(axesVAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArraysInstanced(GL_LINES, 0, 12, SIDE3);
    //
    glUseProgram(shaderProgram);
    glBindVertexArray(gridVAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, SIDE3);
    glBindVertexArray(0);
    // 
    // Draw text
    //
    glWindowPos2i(10, 800 - 30);
    char s[100];
    DWORD time = GetTickCount() - start;
    sprintf(s, "step=%d, time=%0.1f light=%d floor=%d", step, time/1000.0, step/LIGHT, sublattice);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*) s);
    //
    glutSwapBuffers();
}

void printResults(bool full)
{
    cudaMemcpy(host_lattice, dev_lattice, 2 * SIDE2 * SIDE3 * sizeof(Cell), cudaMemcpyDeviceToHost);
    Cell* cell = host_lattice;
    cell += SIDE2 * SIDE3;
    cell = host_lattice;
    for (int v = 0; v < SIDE2; v++)
    {
        for (int z = 0; z < SIDE; z++)
            for (int y = 0; y < SIDE; y++)
                for (int x = 0; x < SIDE; x++)
                {
                    if(cell->f)
                        printf("t=%d: [%d, %d, %d] floor=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] f=%d\n", 
                            cell->t, x, y, z, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->f);
                    cell++;
                }
    }
    printf("step %d\n", step);
    fflush(stdout);
}

void updateVoxels()
{
    cudaStatus = cudaGraphicsMapResources(1, &cuda_resource, 0);
    if (cudaStatus != cudaSuccess)
    {
        puts("map resources error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    size_t num_bytes;
    void* dev_color;
    cudaStatus = cudaGraphicsResourceGetMappedPointer(&dev_color, &num_bytes, cuda_resource);
    if (cudaStatus != cudaSuccess)
    {
        puts("get pointer error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    if (flag)
        interop << <GRID, BLOCK >> > (dev_lattice, (vec3*)dev_color, -1);
    else
        interop << <GRID, BLOCK >> > (dev_lattice, (vec3*)dev_color, sublattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("interop error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    //
    cudaGraphicsUnmapResources(1, &cuda_resource, 0);
   
}

void animation()
{
    commute << <GRID, BLOCK >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("commute error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    compare << <GRID, BLOCK >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("compare error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    replicate << <GRID, BLOCK >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("replicate error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    interact << <GRID, BLOCK >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("interact error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    expand << <GRID, BLOCK >> > (dev_lattice);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("expand error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    int h_count;
    int* d_count;
    cudaMalloc(&d_count, sizeof(int));
    poincare << <GRID, BLOCK >> > (dev_lattice, d_count);
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess)
    {
        puts("Poincaré error");
        perror(cudaGetErrorString(cudaStatus));
        exit(1);
    }
    cudaMemcpy(&h_count, d_count, sizeof(int), cudaMemcpyDeviceToHost);
    //printf("BINGO! %d\n", h_count);

    if (h_count == 0)
        printf("Poincareh cycle: %ld\n", step);
    cudaFree(d_count);
    //
    //printResults(false);
    //
    // Generate graphics
    //
    updateVoxels();
    display();
    //
    //Sleep(100);
    step++;
}

