// scene.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include "glm_host_only.h"
#include "GUI.h"
#include "scene.h"
#include "shader.h"
#include "app_context.h"
#include "globals.h"
#include "simulation.h"
#include "tomography.h"
#include "gui.h"

using namespace SceneConstants;

void initScene(AppContext& ctx) {
	tomography::init();
    ctx.shader = compileShader(vertexShaderSource, fragmentShaderSource);
    if (ctx.shader == 0) {
        throw std::runtime_error("Failed to compile scene shaders");
    }
}

void renderScene(AppContext& ctx) {
    glUseProgram(ctx.shader);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(ctx.shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    framework::render3DObjects();

    // Restore GL state for overlays
    glEnable(GL_BLEND);
    glUseProgram(ctx.shader);

    // Render tomography overlays
    tomography::renderTomoPlane();  // semi-transparent cutting plane
    tomography::renderSlice();  // yellow points
}

void cleanupScene(AppContext& ctx) {
    glDeleteVertexArrays(1, &ctx.cubeVAO);
    glDeleteVertexArrays(1, &ctx.gridVAO);
    glDeleteVertexArrays(1, &ctx.axesVAO);

    glDeleteBuffers(1, &ctx.cubeVBO);
    glDeleteBuffers(1, &ctx.cubeEBO);
    glDeleteBuffers(1, &ctx.gridVBO);
    glDeleteBuffers(1, &ctx.axesVBO);

    if (ctx.shader != 0) {
        glDeleteProgram(ctx.shader);
    }

    glDeleteVertexArrays(1, &ctx.latticeVAO);
    glDeleteBuffers(1, &ctx.latticeVBO);
    glDeleteBuffers(1, &ctx.latticeEBO);
}
