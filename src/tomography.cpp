// ============================================================================
// tomography.cpp - Tomography System Implementation (Using Existing Globals)
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

extern AppContext ctx;
extern TextRenderer* textRenderer;
extern GLuint colorProgram3D;
extern GLint colorMvpLoc3D, colorColorLoc3D;

namespace framework {
    extern glm::mat4 mProjection_;
    extern int vis_dx;
    extern int vis_dy;
    extern int vis_dz;

}

namespace tomography {

// ============================================================================
// Tomography State (using existing globals for controls)
// ============================================================================
struct TomoState {
    float sliderPosition = 0.5f;

    // OpenGL resources for slice rendering
    GLuint pointVAO = 0;
    GLuint pointVBO = 0;
    GLuint planeVAO = 0;
    GLuint planeVBO = 0;

    // --- NOVOS CAMPOS PARA O SHADER TRANSPARENTE ---
    GLuint transparentProgram = 0;
    GLint transparentMvpLoc = -1;
    GLint transparentColorLoc = -1;
    GLint transparentAlphaLoc = -1;

    bool initialized = false;
} state;

// ============================================================================
// Initialization
// ============================================================================
void init() {
    if (state.initialized) return;

    // Posições iniciais dos slices
    tomo_x = automaton::EL / 2;
    tomo_y = automaton::EL / 2;
    tomo_z = automaton::EL / 2;
    state.sliderPosition = 0.5f;

    // Cria recursos OpenGL para renderização de pontos (não usado neste exemplo, mas mantido)
    glGenVertexArrays(1, &state.pointVAO);
    glGenBuffers(1, &state.pointVBO);

    glBindVertexArray(state.pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.pointVBO);
    size_t maxPoints = automaton::EL * automaton::EL * 4;
    glBufferData(GL_ARRAY_BUFFER, maxPoints * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Cria recursos OpenGL para o plano (VAO e VBO)
    glGenVertexArrays(1, &state.planeVAO);
    glGenBuffers(1, &state.planeVBO);

    glBindVertexArray(state.planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.planeVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // --- COMPILA O SHADER TRANSPARENTE ESPECÍFICO PARA O PLANO ---
    state.transparentProgram = compileTransparentShader();
    if (state.transparentProgram) {
        state.transparentMvpLoc = glGetUniformLocation(state.transparentProgram, "uMVP");
        state.transparentColorLoc = glGetUniformLocation(state.transparentProgram, "uColor");
        state.transparentAlphaLoc = glGetUniformLocation(state.transparentProgram, "uAlpha");
    } else {
        std::cerr << "ERROR: Failed to compile transparent shader for tomography plane\n";
    }

    state.initialized = true;
}

// ============================================================================
// Update (call when slider moves or direction changes)
// ============================================================================
void update() {
    if (!state.initialized) return;

    if (!tomoDirs.empty()) {
        // Convert slider [0,1] directly to lattice index
        unsigned sliceIndex = static_cast<unsigned>(state.sliderPosition * (automaton::EL - 1) + 0.5f);

        if (tomoDirs[0].isSelected()) {
            tomo_z = sliceIndex;  // Store as index, not normalized
        } else if (tomoDirs[1].isSelected()) {
            tomo_x = sliceIndex;
        } else if (tomoDirs[2].isSelected()) {
            tomo_y = sliceIndex;
        }
    }
}

// ============================================================================
// Visibility Check (normalized plane coordinate, clamped to test bounds)
// ============================================================================
bool isVoxelVisible(unsigned x, unsigned y, unsigned z) {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState()) return true;
    if (tomoDirs.empty()) return true;

    unsigned sliceIndex = static_cast<unsigned>(
        state.sliderPosition * (automaton::EL - 1) + 0.5f
    );

    if (tomoDirs[0].isSelected()) {       // XY → fixa Z
        return z == sliceIndex;
    } else if (tomoDirs[1].isSelected()) { // YZ → fixa X
        return x == sliceIndex;
    } else if (tomoDirs[2].isSelected()) { // ZX → fixa Y
        return y == sliceIndex;
    }

    return true;
}

static void drawQuads(const std::vector<glm::vec3>& verts,
                      const glm::vec3& color,
                      const glm::mat4& mvp)
{
  if (verts.empty()) return;
  glUseProgram(colorProgram3D);
  glUniformMatrix4fv(colorMvpLoc3D, 1, GL_FALSE, glm::value_ptr(mvp));
  glUniform3f(colorColorLoc3D, color.r, color.g, color.b);

  GLuint vao=0, vbo=0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3), verts.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
  glEnableVertexAttribArray(0);
  // Each face provided as 4 vertices → draw with TRIANGLE_FAN per face
  // Here we assume verts.size() is multiple of 4 and faces are contiguous
  for (size_t i = 0; i + 3 < verts.size(); i += 4) {
    glDrawArrays(GL_TRIANGLE_FAN, (GLint)i, 4);
  }
  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
}

inline unsigned mod(int a, unsigned b) {
    return ((a % (int)b) + (int)b) % (int)b;
}

void renderSlice() {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState()) return;


    const int CENTER_INT = automaton::EL / 2;  // Integer, not float
    const float CELL_SPACING = 0.5f / automaton::EL;
    const float VOXEL_SIZE   = CELL_SPACING / 4.0f;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glm::mat4 view = ctx.camera.GetViewMatrix();
    glm::mat4 projection = framework::mProjection_;
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;
    int count = 0;
    for (unsigned x = 0; x < automaton::EL; ++x) {
        for (unsigned y = 0; y < automaton::EL; ++y) {
            for (unsigned z = 0; z < automaton::EL; ++z) {
                unsigned idx = x * automaton::EL*automaton::EL + y * automaton::EL + z;
                uint32_t color = voxels[idx];
                if (color == 0) continue;
                count++;
                float r = ((color >> 16) & 0xFF) / 255.0f;
                float g = ((color >> 8)  & 0xFF) / 255.0f;
                float b = ( color        & 0xFF) / 255.0f;
                float px = (mod((int)x + framework::vis_dx, automaton::EL) - CENTER_INT) * CELL_SPACING;
                float py = (mod((int)y + framework::vis_dy, automaton::EL) - CENTER_INT) * CELL_SPACING;
                float pz = (mod((int)z + framework::vis_dz, automaton::EL) - CENTER_INT) * CELL_SPACING;

                auto cube = makeCube(px, py, pz, VOXEL_SIZE);
                drawQuads(cube, glm::vec3(r, g, b), mvp);
            }
        }
    }
}

void renderTomoPlane() {
    if (!state.initialized || !tomoEnable || !tomoEnable->getState()) return;
    if (state.transparentProgram == 0) return;  // segurança

    const float PLANE_MIN = -0.25f;
    const float PLANE_MAX =  0.25f;

    unsigned sliceIndex = static_cast<unsigned>(
        state.sliderPosition * (automaton::EL - 1) + 0.5f
    );
    const float EL = static_cast<float>(automaton::EL);
    const float CELL_SPACING = 0.5f / EL;
    const float HALF = EL / 2.0f;

    float coord = (sliceIndex - HALF + 0.5f) * CELL_SPACING;
    coord = glm::clamp(coord, PLANE_MIN, PLANE_MAX);

    std::vector<glm::vec3> vertices;
    if (tomoDirs[0].isSelected()) {  // XY plane (fixed Z)
        vertices = {
            {PLANE_MIN, PLANE_MIN, coord},
            {PLANE_MAX, PLANE_MIN, coord},
            {PLANE_MAX, PLANE_MAX, coord},
            {PLANE_MIN, PLANE_MAX, coord}
        };
    } else if (tomoDirs[1].isSelected()) {  // YZ plane (fixed X)
        vertices = {
            {coord, PLANE_MIN, PLANE_MIN},
            {coord, PLANE_MAX, PLANE_MIN},
            {coord, PLANE_MAX, PLANE_MAX},
            {coord, PLANE_MIN, PLANE_MAX}
        };
    } else if (tomoDirs[2].isSelected()) {  // ZX plane (fixed Y)
        vertices = {
            {PLANE_MIN, coord, PLANE_MIN},
            {PLANE_MAX, coord, PLANE_MIN},
            {PLANE_MAX, coord, PLANE_MAX},
            {PLANE_MIN, coord, PLANE_MAX}
        };
    } else {
        return; // nenhum plano selecionado
    }

    // Atualiza VBO
    glBindVertexArray(state.planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.planeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(glm::vec3), vertices.data());

    // Matriz MVP
    glm::mat4 view = ctx.camera.GetViewMatrix();
    glm::mat4 projection = framework::mProjection_;
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    // Usa o shader transparente guardado no estado
    glUseProgram(state.transparentProgram);
    glUniformMatrix4fv(state.transparentMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(state.transparentColorLoc, 0.4f, 0.4f, 0.4f);  // cor cinza
    glUniform1f(state.transparentAlphaLoc, 0.5f);              // 50% transparente

    // Configura blending e depth test
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);   // desliga escrita no depth buffer

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Restaura estado
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

// ============================================================================
// Render UI Controls
// ============================================================================
void renderControls() {
    if (!state.initialized || !tomoEnable || !textRenderer) return;

    tomoEnable->setFontScale(0.35f);
    tomoEnable->draw(*textRenderer);

    // Render direction radios (only if tomography is enabled)
    if (tomoEnable->getState() && !tomoDirs.empty()) {
        // Render slice position indicator
        char sliceText[64];
        unsigned sliceIndex = static_cast<unsigned>(state.sliderPosition * (automaton::EL - 1));
        snprintf(sliceText, sizeof(sliceText), "Slice: %u / %u", sliceIndex, automaton::EL - 1);
        textRenderer->RenderText(sliceText, (gViewport[2] - 400.0f) * 0.5f, 83.0f, 0.35f, glm::vec3(1.0f),
                                gViewport[2], gViewport[3]);
    }
}

// ============================================================================
// Getters/Setters
// ============================================================================
float getSlicePosition() {
    return state.sliderPosition;
}

void setSlicePosition(float pos) {
    state.sliderPosition = glm::clamp(pos, 0.0f, 1.0f);
    update();
}

// ============================================================================
// Cleanup
// ============================================================================
void cleanup() {
    if (!state.initialized) return;

    if (state.pointVAO) glDeleteVertexArrays(1, &state.pointVAO);
    if (state.pointVBO) glDeleteBuffers(1, &state.pointVBO);
    if (state.planeVAO) glDeleteVertexArrays(1, &state.planeVAO);
    if (state.planeVBO) glDeleteBuffers(1, &state.planeVBO);

    // Don't delete tomo or tomoDirs - they're managed in your globals

    state.initialized = false;
}

} // namespace tomography
