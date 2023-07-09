#ifndef RSMZ_RENDEREROPENGL1_H
#define RSMZ_RENDEREROPENGL1_H

#include "renderer.h"
#include <GL/gl.h>
#include <math.h>
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtc/matrix_transform.hpp> // perspective
#include <GLFW/glfw3.h>
#include <GL/glut.h>
#include "model/simulation.h"

namespace framework
{

class RendererOpenGL1 : public Renderer
{
public:
	RendererOpenGL1();
	virtual ~RendererOpenGL1();

	void init();
	virtual void render();
	void renderAxes();
    void renderCenter();
	void renderClear();
	void renderCube();
	void renderGrid();
	void renderObjects();
	void renderPoints();
	void renderText();
	void renderText2(const std::string& text, float x, float y);
	void resize(int width, int height);

protected:
    glm::mat4 mProjection;

};

void initText();
void drawString(const std::string& str);
void reshape(GLFWwindow* window, int width, int height);

} // end namespace rsmz

#endif // RSMZ_RENDEREROPENGL1_H
