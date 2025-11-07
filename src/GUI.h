/*
 * GUIrenderer.h
 *
 * Declares the OpenGL rendering routines.
 */

#ifndef RSMZ_RENDEREROPENGL1_H
#define RSMZ_RENDEREROPENGL1_H

#include "renderer.h"
#include <GL/glut.h>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <array>
#include <map>
#include <memory>

#define DEBUG
// Inclusões GLM (Seu projeto)
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtc/matrix_transform.hpp> // perspective

// Inclusões de headers internos/do projeto
#include "model/simulation.h"
#include "radio.h"
#include "layers.h"
#include <text.h>
#include "tickbox.h"
#include "progress.h"

#define WIDTH	480     // graph width

namespace framework
{
  using namespace std;

extern ProgressBar *progress;

extern bool replayFrames;
extern unsigned long long replayTimer;

class GUIrenderer : public Renderer
{

private:
  void enhanceVoxel();
  std::vector<std::array<float, 2>> screenPositions;
  std::map<std::pair<int,int>, int> counts;

public:
  GUIrenderer();
  virtual ~GUIrenderer();

  void init();
  virtual void render();
  void renderAxes();
  void renderCenter();
  void renderClear();
  void renderCube();
  void renderPlane();
  void renderObjects();
  void renderWavefront();
  void renderCounts();
  void renderMomentum();
  void renderSpin();
  void renderSineMask();
  void renderHunting();
  void renderCenters();
  void renderCenterBox(const char* text);
  void resize(int width, int height);
  bool projectPoint(const float obj[3],
                    const GLdouble modelview[16],
                    const GLdouble projection[16],
                    const GLint viewport[4],
                    float &winX, float &winY);
  void setProjection(const glm::mat4& proj) { mProjection = proj; }
  void renderUI();
  void renderElapsedTime();
  void renderSimulationStats();
  void renderLayerInfo();
  void renderHelpText();
  void renderSliders();
  void renderTomoControls();
  void renderPauseOverlay();
  void renderSectionLabels();
  void renderCheckboxes();
  void renderDelays();
  void renderViewpointRadios();
  void renderProjectionRadios();
  void renderTomoRadios();
  void renderHyperlink();
  void renderSlice();
  void renderScenarioHelpPane();
  inline bool isVoxelVisible(unsigned x, unsigned y, unsigned z);
  void renderTomoPlane();
  void drawPanel(int x, int y, int width, int height);
  void clearVoxels();

protected:
    glm::mat4 mProjection;
    std::vector<std::array<unsigned, 3>> lastPositions;

};

void drawString8(string s, int x, int y);
void drawString12(const string& text, int x, int y);
void drawBoldText(const string& text, int x, int y, float offset = 0.5f);
void render2Dstring(float x, float y, void *font, const char *string);
void render3Dstring(float x, float y, float z, void *font, const char *string);
void blinkText(unsigned long long timer);
void triggerEvent(unsigned long long timer);

extern bool active;
extern std::vector<Tickbox> data3D;
extern std::unique_ptr<LayerList> list;
extern std::vector<Tickbox> delays;
extern std::vector<Radio> views;
extern std::vector<Radio> projection;

void initText();
void reshape(GLFWwindow* window, int width, int height);

// In the framework namespace, with other extern declarations:
extern bool helpHover;

} // end namespace framework

#endif // RSMZ_RENDEREROPENGL1_H
