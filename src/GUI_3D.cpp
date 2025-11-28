/*
 * GUI_3D.cpp
 */

#include "GUI.h"
#include "model/simulation.h"
#include "color_utils.h"
#include "layers.h"
#include "text_renderer.h"
#include "dropdown.h"
#include "globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <cmath>

AxisProjection gAxisProj[3];
bool gAxisProjValid = false;

namespace automaton {
  extern unsigned EL;
  extern unsigned L2;
  extern unsigned W_USED;
  extern unsigned CENTER;
  extern std::vector<std::array<unsigned,3>> lcenters;
  extern std::vector<Cell> lattice_curr;
}

namespace framework {
  extern int GUImode;
  extern bool MULTICUBE_MODE;
  extern Tickbox *tomo;
  extern std::unique_ptr<LayerList> layerList;
  extern std::vector<Tickbox> data3D;

  // From core (shared color shader)
  extern GLuint colorProgram3D;
  extern GLint colorMvpLoc3D, colorColorLoc3D;

  // Variáveis de visualização (declaradas como extern, definidas em GUI.cpp)
  extern int vis_dx;
  extern int vis_dy;
  extern int vis_dz;

  using namespace automaton;

  // ---------------------------------------------------------------------
  // Helpers: shader-based primitive drawing
  // ---------------------------------------------------------------------
  static void drawPoints(const std::vector<glm::vec3>& pts,
                         const glm::vec3& color,
                         const glm::mat4& mvp,
                         float size = 2.0f)
  {
    if (pts.empty()) return;
    glUseProgram(colorProgram3D);
    glUniformMatrix4fv(colorMvpLoc3D, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(colorColorLoc3D, color.r, color.g, color.b);

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pts.size()*sizeof(glm::vec3), pts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, (GLsizei)pts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
  }

  static void drawLines(const std::vector<glm::vec3>& verts,
                        const glm::vec3& color,
                        const glm::mat4& mvp,
                        float width = 1.0f)
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
    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, (GLsizei)verts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
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

  // ---------------------------------------------------------------------
  // Legacy functions modernized
  // ---------------------------------------------------------------------

  void GUIrenderer::renderMomentum()
  {
    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;x++)
      for (unsigned y=0;y<EL;y++)
        for (unsigned z=0;z<EL;z++) {
          int wx=(x+vis_dx+EL)%EL;
          int wy=(y+vis_dy+EL)%EL;
          int wz=(z+vis_dz+EL)%EL;
          if (getCell(lattice_curr,wx,wy,wz,layerList->getSelected()).pB) {
            float px,py,pz;
            if (GUImode==REPLAY) {
              auto& c=lcenters[layerList->getSelected()];
              px=(int)(x-c[0])*GRID_SIZE;
              py=(int)(y-c[1])*GRID_SIZE;
              pz=(int)(z-c[2])*GRID_SIZE;
            } else {
              px=(int)(x-CENTER_INT)*GRID_SIZE;
              py=(int)(y-CENTER_INT)*GRID_SIZE;
              pz=(int)(z-CENTER_INT)*GRID_SIZE;
            }
            pts.emplace_back(px,py,pz);
          }
        }

    // MVP is set by outer matrices (mProjection_ * camera), here we draw in local space
    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(1.0f,1.0f,0.0f), mvp, 4.0f);
  }

  void GUIrenderer::renderSpin()
  {
    const float GRID_SIZE=0.5f/EL;
    const int CENTER_INT=EL/2;
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;x++)
      for (unsigned y=0;y<EL;y++)
        for (unsigned z=0;z<EL;z++) {
          int wx=(x+vis_dx+EL)%EL;
          int wy=(y+vis_dy+EL)%EL;
          int wz=(z+vis_dz+EL)%EL;
          if (getCell(lattice_curr,wx,wy,wz,layerList->getSelected()).pB) {
            float px=(int)(x-CENTER_INT)*GRID_SIZE;
            float py=(int)(y-CENTER_INT)*GRID_SIZE;
            float pz=(int)(z-CENTER_INT)*GRID_SIZE;
            pts.emplace_back(px,py,pz);
          }
        }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(0.0f,1.0f,1.0f), mvp, 4.0f);
  }

  void GUIrenderer::renderSineMask()
  {
    const float GRID_SIZE=0.5f/EL;
    const int CENTER_INT=EL/2;
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;x++)
      for (unsigned y=0;y<EL;y++)
        for (unsigned z=0;z<EL;z++) {
          if (!isVoxelVisible(x,y,z)) continue;
          int wx=(x+vis_dx+EL)%EL;
          int wy=(y+vis_dy+EL)%EL;
          int wz=(z+vis_dz+EL)%EL;
          if (getCell(lattice_curr,wx,wy,wz,layerList->getSelected()).pB) {
            float px=(int)(x-CENTER_INT)*GRID_SIZE;
            float py=(int)(y-CENTER_INT)*GRID_SIZE;
            float pz=(int)(z-CENTER_INT)*GRID_SIZE;
            pts.emplace_back(px,py,pz);
          }
        }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(1.0f,1.0f,0.0f), mvp, 1.0f);
  }

  void GUIrenderer::renderHunting()
  {
    const float GRID_SIZE=0.5f/EL;
    const int CENTER_INT=EL/2;
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;x++)
      for (unsigned y=0;y<EL;y++)
        for (unsigned z=0;z<EL;z++) {
          int wx=(x+vis_dx+EL)%EL;
          int wy=(y+vis_dy+EL)%EL;
          int wz=(z+vis_dz+EL)%EL;
          if (getCell(lattice_curr,wx,wy,wz,layerList->getSelected()).pB) {
            float px=(int)(x-CENTER_INT)*GRID_SIZE;
            float py=(int)(y-CENTER_INT)*GRID_SIZE;
            float pz=(int)(z-CENTER_INT)*GRID_SIZE;
            pts.emplace_back(px,py,pz);
          }
        }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(1.0f,1.0f,0.0f), mvp, 5.0f);
  }

  void GUIrenderer::renderWavefront()
  {
    const float GRID_SIZE=0.5f/EL;
    const int CENTER_INT=EL/2;
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;x++)
      for (unsigned y=0;y<EL;y++)
        for (unsigned z=0;z<EL;z++) {
          uint32_t color=voxels[x*automaton::L2+y*EL+z];
          if (color==0) continue;
          float px=((int)((x+vis_dx)%EL)-CENTER_INT)*GRID_SIZE;
          float py=((int)((y+vis_dy)%EL)-CENTER_INT)*GRID_SIZE;
          float pz=((int)((z+vis_dz)%EL)-CENTER_INT)*GRID_SIZE;
          pts.emplace_back(px,py,pz);
        }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(1.0f,0.5f,0.0f), mvp, 3.0f);
    enhanceVoxel();
  }

  void GUIrenderer::renderCenters()
  {
    screenPositions_.clear();

    const float GRID_SIZE = 0.5f / EL;
    const int CENTER_INT = EL / 2;
    std::vector<glm::vec3> pts;
    std::vector<glm::vec3> colors;

    for (unsigned w = 0; w < W_USED; w++)
    {
      const auto& center = automaton::lcenters[w];
      unsigned cx = center[0];
      unsigned cy = center[1];
      unsigned cz = center[2];
      Cell &cell = getCell(lattice_curr, cx, cy, cz, w);

      float px = ((int)((int)cell.x[0] + vis_dx) - CENTER_INT) * GRID_SIZE;
      float py = ((int)((int)cell.x[1] + vis_dy) - CENTER_INT) * GRID_SIZE;
      float pz = ((int)((int)cell.x[2] + vis_dz) - CENTER_INT) * GRID_SIZE;

      float r = 0.7f + (w & 1) * 0.3f;
      float g = 0.7f + ((w >> 1) & 1) * 0.3f;
      float b = 0.7f + ((w >> 2) & 1) * 0.3f;

      pts.emplace_back(px,py,pz);
      colors.emplace_back(r,g,b);
    }

    // Draw centers per color
    glm::mat4 mvp = glm::mat4(1.0f);
    for (size_t i=0;i<pts.size();++i) {
      drawPoints(std::vector<glm::vec3>{pts[i]}, colors[i], mvp, 8.0f);
    }
  }

  /*
  void GUIrenderer::renderAxes()
  {
    const float fullSize = 0.5f;
    const float axisLength = fullSize * 0.75f;
    const glm::vec3 colX(0.6f, 0.0f, 0.0f);
    const glm::vec3 colY(0.0f, 0.6f, 0.0f);
    const glm::vec3 colZ(0.3f, 0.3f, 0.8f);

    std::vector<glm::vec3> linesX = { {0.f,0.f,0.f}, {axisLength,0.f,0.f} };
    std::vector<glm::vec3> linesY = { {0.f,0.f,0.f}, {0.f,axisLength,0.f} };
    std::vector<glm::vec3> linesZ = { {0.f,0.f,0.f}, {0.f,0.f,axisLength} };

    //glm::mat4 mvp = glm::mat4(1.0f);
    //drawLines(linesX, colX, mvp, 2.0f);
    //drawLines(linesY, colY, mvp, 2.0f);
    //drawLines(linesZ, colZ, mvp, 2.0f);

    // Labels (approximate; requires proper projection)
    //if (textRenderer) {
    //  textRenderer->RenderText("X", axisLength + 0.02f, -0.02f, 0.7f, 
    //                           glm::vec3(1.f,0.f,0.f), w, h);
    //  textRenderer->RenderText("Y", -0.02f, axisLength + 0.02f, 0.7f, 
    //                           glm::vec3(0.f,1.f,0.f), w, h);
    //  textRenderer->RenderText("Z", 0.0f, -0.02f, 0.7f, 
    //                           glm::vec3(0.3f,0.3f,0.8f), w, h);
    //}
   glm::mat4 mvp = glm::mat4(1.0f);
   drawLines(linesX, colX, mvp, 2.0f);
   drawLines(linesY, colY, mvp, 2.0f);
   drawLines(linesZ, colZ, mvp, 2.0f);
    // Labels: render in screen space to avoid occlusion
    if (textRenderer) {
      glDisable(GL_DEPTH_TEST);
      textRenderer->RenderText("X", gViewport[2]-40, gViewport[3]/2, 0.7f,
                               glm::vec3(1.f,0.f,0.f), gViewport[2], gViewport[3]);
      textRenderer->RenderText("Y", gViewport[2]/2, gViewport[3]-40, 0.7f,
                               glm::vec3(0.f,1.f,0.f), gViewport[2], gViewport[3]);
      textRenderer->RenderText("Z", 40, gViewport[3]/2, 0.7f,
                               glm::vec3(0.3f,0.3f,0.8f), gViewport[2], gViewport[3]);
      glEnable(GL_DEPTH_TEST);
    }

    // Update axis projections for picking
    gAxisProj[0].x0 = gAxisProj[1].x0 = gAxisProj[2].x0 = 0.0f;
    gAxisProj[0].y0 = gAxisProj[1].y0 = gAxisProj[2].y0 = 0.0f;
    gAxisProj[0].x1 = axisLength; gAxisProj[0].y1 = 0.0f;
    gAxisProj[1].x1 = 0.0f;       gAxisProj[1].y1 = axisLength;
    gAxisProj[2].x1 = 0.0f;       gAxisProj[2].y1 = axisLength;
    gAxisProjValid = true;
  }
    */

    /**
 * Renders the cartesian positive axes with labels.
 */
void GUIrenderer::renderAxes()
{
    const float fullSize = 0.5f;
    const float axisLength = fullSize * 0.75f;
    const float labelOffset = 0.02f;

    // --- Modern OpenGL axis lines ---
    struct Vertex { glm::vec3 pos; glm::vec3 color; };
    std::vector<Vertex> axisVerts = {
        // X-axis
        {{0.0f, 0.0f, 0.0f}, {0.6f, 0.0f, 0.0f}},
        {{axisLength, 0.0f, 0.0f}, {0.6f, 0.0f, 0.0f}},
        // Y-axis
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.6f, 0.0f}},
        {{0.0f, axisLength, 0.0f}, {0.0f, 0.6f, 0.0f}},
        // Z-axis
        {{0.0f, 0.0f, 0.0f}, {0.3f, 0.3f, 0.8f}},
        {{0.0f, 0.0f, axisLength}, {0.3f, 0.3f, 0.8f}}
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, axisVerts.size() * sizeof(Vertex),
                 axisVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glLineWidth(2);
    glDrawArrays(GL_LINES, 0, (GLsizei)axisVerts.size());
    glLineWidth(1);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    // --- Axis labels (logic unchanged, still using your text renderer) ---
    textRenderer_.RenderText("x", axisLength + labelOffset, -labelOffset, 0.7f,
                             glm::vec3(1.0f,0.0f,0.0f), gViewport[2], gViewport[3]);
    textRenderer_.RenderText("y", -labelOffset, axisLength + labelOffset, 0.7f,
                             glm::vec3(0.0f,1.0f,0.0f), gViewport[2], gViewport[3]);
    textRenderer_.RenderText("z", 0.0f, -labelOffset + axisLength, 0.7f,
                             glm::vec3(0.3f,0.3f,0.8f), gViewport[2], gViewport[3]);

    // --- Thumb point (modernized) ---
    if (thumb.active) {
        glm::vec3 pos;
        if (thumb.axis == 0) pos = {thumb.position, 0.0f, 0.0f};
        if (thumb.axis == 1) pos = {0.0f, thumb.position, 0.0f};
        if (thumb.axis == 2) pos = {0.0f, 0.0f, thumb.position};

        GLuint vao2, vbo2;
        glGenVertexArrays(1, &vao2);
        glGenBuffers(1, &vbo2);
        glBindVertexArray(vao2);
        glBindBuffer(GL_ARRAY_BUFFER, vbo2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &pos, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glPointSize(10.0f);
        // set color via uniform in your shader, or add color attribute
        glDrawArrays(GL_POINTS, 0, 1);

        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo2);
        glDeleteVertexArrays(1, &vao2);
    }

    // === Projection caching (unchanged) ===
    GLfloat model[16], proj[16];
    GLint viewport[4];
    glGetFloatv(GL_MODELVIEW_MATRIX, model);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);

    float ox, oy;
    projectPoint((float[3]){0.0f, 0.0f, 0.0f}, model, proj, viewport, ox, oy);
    gAxisProj[0].x0 = gAxisProj[1].x0 = gAxisProj[2].x0 = ox;
    gAxisProj[0].y0 = gAxisProj[1].y0 = gAxisProj[2].y0 = oy;

    projectPoint((float[3]){axisLength, 0.0f, 0.0f}, model, proj, viewport,
                 gAxisProj[0].x1, gAxisProj[0].y1);
    projectPoint((float[3]){0.0f, axisLength, 0.0f}, model, proj, viewport,
                 gAxisProj[1].x1, gAxisProj[1].y1);
    projectPoint((float[3]){0.0f, 0.0f, axisLength}, model, proj, viewport,
                 gAxisProj[2].x1, gAxisProj[2].y1);

    gAxisProjValid = true;
}


  void GUIrenderer::renderCube()
  {
    const float a = 0.25f;
    std::vector<glm::vec3> faces;

    // Right face
    faces.push_back({ a,-a,-a}); faces.push_back({ a, a,-a});
    faces.push_back({ a, a, a}); faces.push_back({ a,-a, a});
    // Left face
    faces.push_back({-a,-a,-a}); faces.push_back({-a, a,-a});
    faces.push_back({-a, a, a}); faces.push_back({-a,-a, a});
    // Top face
    faces.push_back({-a, a,-a}); faces.push_back({ a, a,-a});
    faces.push_back({ a, a, a}); faces.push_back({-a, a, a});
    // Bottom face
    faces.push_back({-a,-a,-a}); faces.push_back({ a,-a,-a});
    faces.push_back({ a,-a, a}); faces.push_back({-a,-a, a});
    // Front face
    faces.push_back({-a,-a, a}); faces.push_back({ a,-a, a});
    faces.push_back({ a, a, a}); faces.push_back({-a, a, a});
    // Back face
    faces.push_back({-a,-a,-a}); faces.push_back({ a,-a,-a});
    faces.push_back({ a, a,-a}); faces.push_back({-a, a,-a});

    glm::mat4 mvp = glm::mat4(1.0f);
    drawQuads(faces, glm::vec3(0.45f,0.13f,0.13f), mvp);
  }

  void GUIrenderer::renderPlane()
  {
    const float GRID_SIZE = 0.5f / EL;
    const float half = EL * GRID_SIZE / 2.0f;
    const float eps = -1e-4f;

    std::vector<glm::vec3> grid;
    for (unsigned i = 0; i <= EL; ++i)
    {
      float p = -half + i * GRID_SIZE;
      grid.push_back({ p, eps, -half });
      grid.push_back({ p, eps,  half });
      grid.push_back({ -half, eps, p });
      grid.push_back({  half, eps, p });
    }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawLines(grid, glm::vec3(1.0f,1.0f,1.0f), mvp, 1.0f);
  }

  void GUIrenderer::enhanceVoxel()
  {
    unsigned w = layerList->getSelected();
    const float GRID_SIZE = 0.5f / EL;

    float cx, cy, cz;

    if (GUImode == REPLAY)
    {
      const auto& center = automaton::lcenters[w];
      cx = ((center[0] + vis_dx) - EL / 2) * GRID_SIZE;
      cy = ((center[1] + vis_dy) - EL / 2) * GRID_SIZE;
      cz = ((center[2] + vis_dz) - EL / 2) * GRID_SIZE;
    }
    else
    {
      Cell &cell = getCell(lattice_curr, CENTER, CENTER, CENTER, w);
      cx = ((cell.x[0] + vis_dx) - EL / 2) * GRID_SIZE;
      cy = ((cell.x[1] + vis_dy) - EL / 2) * GRID_SIZE;
      cz = ((cell.x[2] + vis_dz) - EL / 2) * GRID_SIZE;
    }

    const int POINT_COUNT = 12;
    const float MAX_OFFSET = 0.005f;

    std::vector<glm::vec3> pts;
    pts.reserve(POINT_COUNT);
    for (int i = 0; i < POINT_COUNT; ++i)
    {
      float dx = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      float dy = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      float dz = ((std::rand() / (float)RAND_MAX) * 2.0f - 1.0f) * MAX_OFFSET;
      pts.emplace_back(cx + dx, cy + dy, cz + dz);
    }

    glm::mat4 mvp = glm::mat4(1.0f);
    drawPoints(pts, glm::vec3(0.70f,0.70f,0.70f), mvp, 2.0f);
  }

  /**
   * Renders 3D objects
   * A transformação MVP deve ser gerenciada pelo GUIrenderer antes de chamar esta função
   */
  void GUIrenderer::renderObjects()
  {
    glEnable(GL_DEPTH_TEST);
/*
    if (scenario >= 0)
    {
      if (data3D.size() > 0 && data3D[0].getState()) renderWavefront();
      if (data3D.size() > 1 && data3D[1].getState()) renderMomentum();
      if (data3D.size() > 2 && data3D[2].getState()) renderSpin();
      if (data3D.size() > 3 && data3D[3].getState()) renderSineMask();
      if (data3D.size() > 4 && data3D[4].getState()) renderHunting();
      if (data3D.size() > 5 && data3D[5].getState()) renderCenters();
    }
    if (data3D.size() > 6 && data3D[6].getState())
    {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      renderCube();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
      */
    if (data3D.size() > 7 && data3D[7].getState()) renderAxes();
    /*
    if (data3D.size() > 8 && data3D[8].getState()) renderPlane();
    if (tomo && tomo->getState()) renderTomoPlane();
    */
  }

} // namespace framework