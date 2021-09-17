#pragma comment(lib, "C:\\GL\\GLUT\\lib\\x64\\freeglut.lib")

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#define GLEW_STATIC

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cuda_gl_interop.h>

#include "cglm/vec3.h"
#include "automaton.h"

extern unsigned int shaderProgram;
extern unsigned int vao;
extern struct cudaGraphicsResource* cuda_resource;
extern cudaError_t cudaStatus;
extern Cell *host_lattice, *dev_lattice;
extern DWORD start;
int step = 0;

boolean flag;

void display()
{
    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, SIDE3);
    glBindVertexArray(0);
    //
    // Draw text
    //
    glWindowPos2i(10, 600 - 30);
    char s[25];
    DWORD time = GetTickCount() - start;
    sprintf(s, "step=%d, time=%0.1f", step, time/1000.0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*) s);
    //
    glutSwapBuffers();
}

void printResults(bool full)
{
    cudaMemcpy(host_lattice, dev_lattice, 2 * SIDE2 * SIDE3 * sizeof(Cell), cudaMemcpyDeviceToHost);
    Cell* cell = host_lattice;
    //printf("t=%d: (%c) [%d, %d, %d] v=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] pole=[%d,%d,%d] f=%d t2=%d syn=%d \t%s\n", cell->t, '?', 0, 0, 0, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->pole[0], cell->pole[1], cell->pole[2], cell->f, cell->t * cell->t, cell->synch, "??");
    cell += SIDE2 * SIDE3;
    //printf("t=%d: (%c) [%d, %d, %d] v=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] pole=[%d,%d,%d] f=%d t2=%d syn=%d \t%s\n", cell->t, '?', 0, 0, 0, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->pole[0], cell->pole[1], cell->pole[2], cell->f, cell->t * cell->t, cell->synch, "??");
    cell = host_lattice;
    for (int v = 0; v < SIDE2; v++)
    {
        for (int z = 0; z < SIDE; z++)
            for (int y = 0; y < SIDE; y++)
                for (int x = 0; x < SIDE; x++)
                {
                    char act = cell->active ? 'A' : ' ';
                    char* arrow = ISNULL(cell->p) ? "" : "<---";
                    if(!ISNULL(cell->p))
                        printf("t=%d: (%c) [%d, %d, %d] v=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] pole=[%d,%d,%d] f=%d t2=%d syn=%d \t%s\n", 
                            cell->t, act, x, y, z, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->pole[0], cell->pole[1], cell->pole[2], cell->f, cell->t * cell->t, cell->synch, arrow);
                    /*
                    if (full)
                    {
                        if (cell->f > 0)
                            printf("t=%d: (%c) [%d, %d, %d] v=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] pole=[%d,%d,%d] f=%d t2=%d syn=%d \t%s\n", cell->t, act, x, y, z, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->pole[0], cell->pole[1], cell->pole[2], cell->f, cell->t * cell->t, cell->synch, arrow);
                    }
                    else if (!cell->active && v == 60 && (!ISNULL(cell->p) || cell->f > 0))
                    {
                        printf("t=%d: (%c) [%d, %d, %d] v=%d, noise=%d p=[%d,%d,%d] o=[%d,%d,%d] pole=[%d,%d,%d] f=%d t2=%d syn=%d \t%s\n", cell->t, act, x, y, z, cell->b, cell->noise, cell->p[0], cell->p[1], cell->p[2], cell->o[0], cell->o[1], cell->o[2], cell->pole[0], cell->pole[1], cell->pole[2], cell->f, cell->t * cell->t, cell->synch, arrow);
                    }
                    */
                    cell++;
                }
    }
    printf("step %d\n", step);
    fflush(stdout);
}

int sublattice = 130;

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
    //printResults(false);
    //
    // Generate graphics
    //
    updateVoxels();
    display();
    //
    //Sleep(3000);
    step++;
}

