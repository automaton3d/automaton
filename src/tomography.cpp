// ============================================================================
// tomography.cpp - Versão MÍNIMA para Debug (Crash Silencioso)
// ============================================================================
#include "tomography.h"
#include "shader.h"
#include "globals.h"
#include "model/simulation.h"
#include "tickbox.h"
#include "app_context.h"
#include "text_renderer.h"
#include <glad/glad.h>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>

extern std::mutex gVoxelBufferMutex; 
extern AppContext ctx;
extern TextRenderer* textRenderer;
extern GLuint colorProgram3D;
extern GLint colorMvpLoc3D, colorColorLoc3D;

namespace framework {
    extern glm::mat4 mProjection_;
}

namespace tomography {

struct TomoState {
    float sliderPosition = 0.5f;
    GLuint planeVAO = 0, planeVBO = 0;
    GLuint transparentProgram = 0;
    GLint transparentMvpLoc = -1, transparentColorLoc = -1, transparentAlphaLoc = -1;
    bool initialized = false;
    std::vector<VoxelSnapshot> snapshot;
    bool needsUpdate = true;
} state;

// mod segura
inline int mod(int a, int b) { return ((a % b) + b) % b; }

// ============================================================================
// init
// ============================================================================
void init() {
    if (state.initialized) return;

    // Plane VAO/VBO
    glGenVertexArrays(1, &state.planeVAO);
    glGenBuffers(1, &state.planeVBO);
    glBindVertexArray(state.planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.planeVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    state.transparentProgram = compileTransparentShader();
    if (state.transparentProgram) {
        state.transparentMvpLoc = glGetUniformLocation(state.transparentProgram, "uMVP");
        state.transparentColorLoc = glGetUniformLocation(state.transparentProgram, "uColor");
        state.transparentAlphaLoc = glGetUniformLocation(state.transparentProgram, "uAlpha");
    }

    state.initialized = true;
    std::cout << "[Tomo] init() OK\n";
}

void updateSnapshot() {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState() || tomoDirs.empty()) {
        state.snapshot.clear();
        state.needsUpdate = false;
        return;
    }

    // Só atualiza se o slider realmente mudou
    if (std::abs(state.sliderPosition - state.lastUpdatedPosition) < 0.001f && !state.needsUpdate) {
        return;
    }

    std::vector<VoxelSnapshot> nextSnapshot;
    nextSnapshot.reserve(static_cast<size_t>(automaton::EL) * automaton::EL);

    std::unique_lock<std::mutex> lock(gVoxelBufferMutex, std::defer_lock);
    if (!lock.try_lock()) {
        state.needsUpdate = true;
        return;
    }

    const int EL = automaton::EL;
    const int CENTER_INT = EL / 2;
    const float CELL_SPACING = 0.5f / static_cast<float>(EL);
    const size_t totalVoxels = voxels.size();

    unsigned sliceIndex = static_cast<unsigned>(state.sliderPosition * (EL - 1) + 0.5f);

    int plane = 0;
    if (tomoDirs[0].isSelected()) plane = 0;
    else if (tomoDirs[1].isSelected()) plane = 1;
    else plane = 2;

    for (unsigned a = 0; a < EL; ++a) {
        for (unsigned b = 0; b < EL; ++b) {
            unsigned x, y, z;
            if (plane == 0)      { x = a; y = b; z = sliceIndex; }
            else if (plane == 1) { x = sliceIndex; y = a; z = b; }
            else                 { x = b; y = sliceIndex; z = a; }

            size_t idx = (size_t)x * EL * EL + (size_t)y * EL + z;
            if (idx >= totalVoxels) continue;

            uint32_t color = voxels[idx];
            if (color != 0) {
                float px = (mod((int)x + gConfig.view.vis_dx, EL) - CENTER_INT) * CELL_SPACING;
                float py = (mod((int)y + gConfig.view.vis_dy, EL) - CENTER_INT) * CELL_SPACING;
                float pz = (mod((int)z + gConfig.view.vis_dz, EL) - CENTER_INT) * CELL_SPACING;
                nextSnapshot.push_back({{px, py, pz}, color});
            }
        }
    }

    state.snapshot.swap(nextSnapshot);
    state.lastUpdatedPosition = state.sliderPosition;
    state.needsUpdate = false;
}

// ============================================================================
// Outras funções (mínimas)
// ============================================================================
void requestUpdate() { state.needsUpdate = true; }

void update() {
    if (!state.initialized || tomoDirs.empty() || !tomoEnable || !tomoEnable->getState())
        return;

    unsigned sliceIndex = static_cast<unsigned>(state.sliderPosition * (automaton::EL - 1) + 0.5f);

    if (tomoDirs[0].isSelected())      tomo_z = sliceIndex;
    else if (tomoDirs[1].isSelected()) tomo_x = sliceIndex;
    else if (tomoDirs[2].isSelected()) tomo_y = sliceIndex;

    state.needsUpdate = true;
}

bool isVoxelVisible(unsigned, unsigned, unsigned) { return true; }

// ============================================================================
// renderSlice - Versão Autônoma
// ============================================================================
// Função auxiliar local (deve vir ANTES de renderSlice)
static void drawQuadsLocal(const std::vector<glm::vec3>& verts,
                           const glm::vec3& color,
                           const glm::mat4& mvp)
{
    if (verts.empty()) return;

    glUseProgram(colorProgram3D);
    glUniformMatrix4fv(colorMvpLoc3D, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(colorColorLoc3D, color.r, color.g, color.b);

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    for (size_t i = 0; i + 3 < verts.size(); i += 4) {
        glDrawArrays(GL_TRIANGLE_FAN, (GLint)i, 4);
    }

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// ============================================================================
// renderSlice
// ============================================================================
void renderSlice() {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState()) return;

    updateSnapshot();        // chama com filtro interno

    if (state.snapshot.empty()) return;

    const float CELL_SPACING = 0.5f / automaton::EL;
    const float VOXEL_SIZE = CELL_SPACING / 4.0f;
    glm::mat4 mvp = framework::mProjection_ * ctx.camera.GetViewMatrix() * glm::mat4(1.0f);

    for (const auto& voxel : state.snapshot) {
        float r = ((voxel.color >> 16) & 0xFF) / 255.0f;
        float g = ((voxel.color >> 8)  & 0xFF) / 255.0f;
        float b = ( voxel.color        & 0xFF) / 255.0f;

        auto cube = makeCube(voxel.pos.x, voxel.pos.y, voxel.pos.z, VOXEL_SIZE);
        drawQuadsLocal(cube, glm::vec3(r, g, b), mvp);
    }
}

void renderTomoPlane() {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState()) return;
    if (tomoDirs.empty() || state.transparentProgram == 0) return;

    const float PLANE_MIN = -0.25f;
    const float PLANE_MAX =  0.25f;
    const int EL = automaton::EL;
    const float HALF = static_cast<float>(EL) / 2.0f;
    const float CELL_SPACING = 0.5f / static_cast<float>(EL);

    unsigned sliceIndex = static_cast<unsigned>(state.sliderPosition * (EL - 1) + 0.5f);

    // Mesma lógica de posicionamento usada nos voxels
    float coord = (static_cast<float>(sliceIndex) - HALF + 0.5f) * CELL_SPACING;
    coord = glm::clamp(coord, PLANE_MIN, PLANE_MAX);

    int plane = 0;
    if (tomoDirs[0].isSelected()) plane = 0;
    else if (tomoDirs[1].isSelected()) plane = 1;
    else plane = 2;

    std::vector<glm::vec3> vertices;
    if (plane == 0) { // XY
        vertices = {{PLANE_MIN, PLANE_MIN, coord}, {PLANE_MAX, PLANE_MIN, coord},
                    {PLANE_MAX, PLANE_MAX, coord}, {PLANE_MIN, PLANE_MAX, coord}};
    } else if (plane == 1) { // YZ
        vertices = {{coord, PLANE_MIN, PLANE_MIN}, {coord, PLANE_MAX, PLANE_MIN},
                    {coord, PLANE_MAX, PLANE_MAX}, {coord, PLANE_MIN, PLANE_MAX}};
    } else { // ZX
        vertices = {{PLANE_MIN, coord, PLANE_MIN}, {PLANE_MAX, coord, PLANE_MIN},
                    {PLANE_MAX, coord, PLANE_MAX}, {PLANE_MIN, coord, PLANE_MAX}};
    }

    glBindVertexArray(state.planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.planeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(glm::vec3), vertices.data());

    glm::mat4 mvp = framework::mProjection_ * ctx.camera.GetViewMatrix() * glm::mat4(1.0f);

    glUseProgram(state.transparentProgram);
    glUniformMatrix4fv(state.transparentMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(state.transparentColorLoc, 0.6f, 0.3f, 0.8f);   // roxo visível
    glUniform1f(state.transparentAlphaLoc, 0.7f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void renderControls() {
    if (tomoEnable && textRenderer) tomoEnable->draw(*textRenderer);
}

float getSlicePosition() { return state.sliderPosition; }

void setSlicePosition(float pos) {
    state.sliderPosition = glm::clamp(pos, 0.0f, 1.0f);
    state.needsUpdate = true;
    requestUpdate();           // força flag
    update();                  // atualiza tomo_x/y/z
    std::cout << "[Tomo] Slider moved to " << state.sliderPosition << "\n"; // debug
}

void cleanup() { state.initialized = false; }

} // namespace tomography