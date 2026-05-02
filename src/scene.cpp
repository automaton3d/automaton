// scene.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

#include "glm/glm.hpp"           // ← use este
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GUI.h"
#include "scene.h"
#include "shader.h"
#include "app_context.h"
#include "globals.h"
#include "model/simulation.h"
#include "tomography.h"
#include "gui.h"
#include "GUI3D.h"

using namespace SceneConstants;

void initScene(AppContext& ctx) {
    tomography::init();
    
    ctx.shader = compileShader(vertexShaderSource, fragmentShaderSource);
    if (ctx.shader == 0) {
        throw std::runtime_error("Failed to compile scene shaders");
    }
}

void renderScene(AppContext& ctx) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Renderização principal 3D (frente esférica)
    if (ctx.shader != 0) {
        glUseProgram(ctx.shader);
        glm::mat4 model = glm::mat4(1.0f);
        GLint modelLoc = glGetUniformLocation(ctx.shader, "model");
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        }

        framework::render3DObjects();        // ← esta linha está causando erro
    }

    // Tomografia
    tomography::renderTomoPlane();
    tomography::renderSlice();

    // Restaura para GUI
    glEnable(GL_BLEND);
    if (ctx.shader != 0) glUseProgram(ctx.shader);
}

void cleanupScene(AppContext& ctx) {
    if (ctx.shader != 0) {
        glDeleteProgram(ctx.shader);
    }
}