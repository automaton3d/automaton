#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "automaton.h"

extern float yaw, pitch;
extern int sublattice;
extern boolean flag;

void specialKeys(int key, int x, int y)
{
    switch (key)
    {
        case GLUT_KEY_UP:
            sublattice++;
            break;
        case GLUT_KEY_DOWN:
            sublattice--;
            break;
    }
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        // Exit on escape key press
        //
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
        case 'X':
        case 'x':
            flag = !flag;
            break;
        case '0':
            yaw = -120;
            pitch = 45;
            updateCamera();
            break;
        case '1':
            yaw = -90;
            pitch = 0;
            updateCamera();
            break;
        case '2':
            yaw = -180;
            pitch = 0;
            updateCamera();
            break;
        case '3':
            yaw = -90;
            pitch = 90;
            updateCamera();
            break;
    }
}
