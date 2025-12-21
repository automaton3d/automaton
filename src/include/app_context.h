// app_context.h
#pragma once
#include <vector>
#include "camera.h"

struct AppContext {
    // Shader program
    unsigned int shader = 0;

    // Vertex Array Objects
    unsigned int cubeVAO = 0;
    unsigned int gridVAO = 0;
    unsigned int axesVAO = 0;
    unsigned int latticeVAO = 0;   // <-- novo para o cubo wireframe

    // Vertex Buffer Objects
    unsigned int cubeVBO = 0;
    unsigned int gridVBO = 0;
    unsigned int axesVBO = 0;
    unsigned int latticeVBO = 0;   // <-- novo para o cubo wireframe
    unsigned int cubeEBO = 0;
    unsigned int latticeEBO = 0;   // <-- novo para o cubo wireframe

    // Camera
    OrbitCamera camera;

    // Scene settings
    bool showAxes = true;

    // Grid data
    std::vector<float> gridVerts;

    // Input state
    bool pPressed = false;
};
