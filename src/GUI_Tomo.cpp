/*
 * GUI_Tomo.cpp (modernized, legacy structure preserved)
 *
 * Provides GUIrenderer methods for tomography slice and plane rendering.
 * Immediate mode drawing replaced with shader-based helpers.
 */

#include "GUI.h"
#include "hslider.h"
#include "tickbox.h"
#include "radio.h"
#include "model/simulation.h"
#include "color_utils.h"
#include "globals.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace automaton {
  extern unsigned EL;
  extern unsigned L2;
}

namespace framework {
  extern Tickbox *tomo;
  extern HSlider hslider;
  extern std::vector<Radio> tomoDirs;
  extern unsigned tomo_x, tomo_y, tomo_z;

  // Helpers: shader-based drawing
  extern GLuint colorProgram3D;
  extern GLint colorMvpLoc3D, colorColorLoc3D;

  static void drawPoints(const std::vector<glm::vec3>& pts,
                         const glm::vec3& color,
                         float size = 3.0f)
  {
    if (pts.empty()) return;
    glUseProgram(colorProgram3D);
    glm::mat4 mvp = glm::mat4(1.0f);
    glUniformMatrix4fv(colorMvpLoc3D, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(colorColorLoc3D, color.r, color.g, color.b);

    GLuint vao=0,vbo=0;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,pts.size()*sizeof(glm::vec3),pts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(0);
    glPointSize(size);
    glDrawArrays(GL_POINTS,0,(GLsizei)pts.size());
    glBindVertexArray(0);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
  }

  static void drawQuad(const std::vector<glm::vec3>& verts,
                       const glm::vec3& color,
                       float alpha = 0.2f)
  {
    if (verts.size()!=4) return;
    glUseProgram(colorProgram3D);
    glm::mat4 mvp = glm::mat4(1.0f);
    glUniformMatrix4fv(colorMvpLoc3D,1,GL_FALSE,glm::value_ptr(mvp));
    glUniform3f(colorColorLoc3D,color.r,color.g,color.b);

    GLuint vao=0,vbo=0;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(glm::vec3),verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN,0,4);
    glBindVertexArray(0);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
  }

  // ---------------------------------------------------------------------
  // Legacy functions modernized
  // ---------------------------------------------------------------------

  void GUIrenderer::renderSlice()
  {
    if (!tomo || !tomo->getState()) return;

    const float GRID_SIZE = 0.5f / EL;
    unsigned sliceIndex = hslider.getSliceIndex(EL);
    std::vector<glm::vec3> pts;

    for (unsigned x=0;x<EL;++x)
      for (unsigned y=0;y<EL;++y)
        for (unsigned z=0;z<EL;++z) {
          bool match=false;
          if (tomoDirs[0].isSelected()) match=(z==sliceIndex);
          else if (tomoDirs[1].isSelected()) match=(x==sliceIndex);
          else if (tomoDirs[2].isSelected()) match=(y==sliceIndex);
          if (!match) continue;

          uint32_t color = voxels[x*automaton::L2+y*EL+z];
          if (color==0) continue;

          float r,g,b;
          decodeColor(color,r,g,b);
          float px=(x-EL/2)*GRID_SIZE;
          float py=(y-EL/2)*GRID_SIZE;
          float pz=(z-EL/2)*GRID_SIZE;
          pts.emplace_back(px,py,pz);
        }

    drawPoints(pts, glm::vec3(1.0f,1.0f,0.0f), 3.0f);
  }

  void GUIrenderer::renderTomoControls()
  {
    if (tomo && textRenderer)
    {
      tomo->setFontScale(0.6f);
      tomo->draw(*textRenderer, gViewport[2], gViewport[3]);
    }
  }

  void GUIrenderer::renderTomoPlane()
  {
    if (!tomo || !tomo->getState()) return;

    unsigned sliceIndex = hslider.getSliceIndex(EL);
    if (tomoDirs[0].isSelected()) tomo_z = sliceIndex;
    else if (tomoDirs[1].isSelected()) tomo_x = sliceIndex;
    else if (tomoDirs[2].isSelected()) tomo_y = sliceIndex;

    const float GRID_SIZE = 0.5f / EL;
    const float HALF = EL / 2.0f;
    float coord = (sliceIndex - HALF + 0.5f) * GRID_SIZE;

    std::vector<glm::vec3> quad;
    if (tomoDirs[0].isSelected()) {
      quad = { {-0.25f,-0.25f,coord}, {0.25f,-0.25f,coord},
               {0.25f,0.25f,coord},  {-0.25f,0.25f,coord} };
    } else if (tomoDirs[1].isSelected()) {
      quad = { {coord,-0.25f,-0.25f}, {coord,0.25f,-0.25f},
               {coord,0.25f,0.25f},   {coord,-0.25f,0.25f} };
    } else if (tomoDirs[2].isSelected()) {
      quad = { {-0.25f,coord,-0.25f}, {0.25f,coord,-0.25f},
               {0.25f,coord,0.25f},   {-0.25f,coord,0.25f} };
    }

    drawQuad(quad, glm::vec3(0.3f,0.8f,1.0f), 0.2f);

    // Draw marker point at slice center
    std::vector<glm::vec3> marker;
    if (tomoDirs[0].isSelected()) marker.push_back({0.0f,0.0f,coord});
    else if (tomoDirs[1].isSelected()) marker.push_back({coord,0.0f,0.0f});
    else if (tomoDirs[2].isSelected()) marker.push_back({0.0f,coord,0.0f});

    drawPoints(marker, glm::vec3(0.5f,1.0f,1.0f), 8.0f);
  }

} // namespace framework
