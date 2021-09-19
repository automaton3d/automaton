#pragma comment(lib, "C:\\GL\\GLUT\\lib\\x64\\freeglut.lib")

#define GLEW_STATIC

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <cuda_gl_interop.h>

#include "callbacks.h"
#include "automaton.h"
#include "cglm/mat4.h"
#include "cglm/affine.h"
#include "cglm/cglm.h"
#include "cglm/call.h"
#include "cglm/cam.h"
#include "cglm/vec3.h"

#include "automaton.h"

const char* vertexShaderSource = "#version 460 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aColor;\n"
"out vec4 fColor;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"

"void main()\n"
"{\n"
"	vec3 aOffset;"
#if ORDER==4
"	aOffset[0] = (gl_InstanceID & 15) * 8 - 64;\n"
"	aOffset[1] = ((gl_InstanceID >> 4) & 15) * 8 - 64;\n"
"	aOffset[2] = (gl_InstanceID >> 8) * 8 - 64;\n"
#else  if ORDER==5
"	aOffset[0] = (gl_InstanceID & 31) * 16 - 256;\n"
"	aOffset[1] = ((gl_InstanceID >> 5) & 31) * 16 - 256;\n"
"	aOffset[2] = (gl_InstanceID >> 10) * 16 - 256;\n"
#endif
"	gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);\n"
"	if(aColor.x==0.6 && aColor.y==0.6 && aColor.z==0.8)\n"
"		fColor = vec4(aColor, 0.2);\n"
"	else\n"
"		fColor = vec4(aColor, 1.0);\n"
"}\0";

unsigned int vertexShader;

const char* fragmentShaderSource = "#version 460 core\n"
"in vec4 fColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"	FragColor = fColor;\n"
"}\0";

unsigned int shaderProgram;
unsigned int vao;

mat4 model = GLM_MAT4_IDENTITY_INIT;
mat4 view = GLM_MAT4_IDENTITY_INIT;
mat4 projection;

// Camera

float yaw = -120;// -90;
float pitch = 45;// 0;

vec3 cameraPos;
vec3 cameraFront;
vec3 cameraUp = { 0.0f, 1.0f, 0.0f };

Cell* host_lattice;
Cell* dev_lattice;

struct cudaGraphicsResource* cuda_resource;
unsigned int colorVBO;
size_t num_bytes;

cudaError_t cudaStatus;
DWORD start;

vec3 colors[SIDE3];

GLfloat cubeVertices[] = 
{
	-1.0f,-1.0f,-1.0f, // triangle 1 : begin
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, // triangle 1 : end
	1.0f, 1.0f,-1.0f, // triangle 2 : begin
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f, // triangle 2 : end
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f
};

void initCuda()
{
	size_t heapsize = 2 * SIDE2 * SIDE3 * sizeof(Cell);
	printf("Program launched: SIDE=%d, sizeof=%zd, heap=%zd\n", SIDE, sizeof(Cell), heapsize); fflush(stdout);
	size_t free, total;
	cudaMemGetInfo(&free, &total);
	printf("global memory: \n\ttotal=%zd\n\tfree=%zd", total, free);
	//
	cudaStatus = cudaMalloc((void**)&dev_lattice, heapsize);
	if (cudaStatus != cudaSuccess)
	{
		perror("cudaMalloc failed");
		exit(1);
	}
	cudaMemGetInfo(&free, &total);
	printf("\n\tused=%zd\n", total - free);

	cudaDeviceProp prop;
	cudaGetDeviceProperties(&prop, 0);
	printf("ck=%d\n", prop.concurrentKernels);
	fflush(stdout);
	//
	// Host memory allocation
	//
	host_lattice = (Cell*)malloc(heapsize);
	if (host_lattice == NULL)
	{
		perror("host ram unavailable");
		exit(1);
	}
	hologram << <GRID, BLOCK >> > (dev_lattice);
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess)
	{
		puts("KERNEL error");
		perror(cudaGetErrorString(cudaStatus));
		exit(1);
	}
}

void closeApp()
{
	cudaStatus = cudaGraphicsUnregisterResource(cuda_resource);
	if (cudaStatus != cudaSuccess)
	{
		puts("unregister error");
		perror(cudaGetErrorString(cudaStatus));
		exit(1);
	}
	cudaFree(dev_lattice);
	cudaDeviceReset();
	free(host_lattice);
	printf("finished.\n");
}

void updateCamera()
{
	cameraFront[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
	cameraFront[1] = sin(glm_rad(pitch));
	cameraFront[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
	glm_normalize(cameraFront);
	cameraPos[0] = -SIDE * cameraFront[0];
	cameraPos[1] = -SIDE * cameraFront[1];
	cameraPos[2] = -SIDE * cameraFront[2];
	//
	// Assemble the view matrix
	//
	vec3 sum;
	glm_vec3_add(cameraPos, cameraFront, sum);
	glm_lookat(cameraPos, sum, cameraUp, view);
	//
	int loc = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(loc, 1, GL_FALSE, view[0]);
}

int initOpenGL(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	char s[] = "                              ";
	sprintf(s, "Automaton %dx%dx%dx%dx%d", SIDE, SIDE, SIDE, SIDE2, 2);
	glutCreateWindow(s);
	printf("\tGPU: %s\n", glGetString(GL_VERSION));
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("glew init %s\n", glewGetErrorString(err)); fflush(stdout);
		return -1;
	}
	//
	// Create shaders
	//
	int success;
	char infoLog[512];
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		perror("Vertex shader failed.\n");
		return -1;
	}
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		perror("Vertex shader failed.\n");
		return -1;
	}
	//
	// Link shaders
	//
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		perror("Linking error.\n");
		return -1;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	for (int i = 0; i < SIDE3; i++)
	{
		colors[i][0] = 0;
		colors[i][1] = 1;
		colors[i][2] = 1;
	}
	glUseProgram(shaderProgram);
	//
	// Create vbos
	//
	unsigned int cubeVBO;
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	//
	glGenBuffers(1, &colorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * SIDE3, &colors[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//
	// Create vao
	//
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	//
	// Add vbos
	//
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
	glVertexAttribDivisor(1, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//
	// Connect color vbo to cuda
	//
	cudaStatus = cudaGraphicsGLRegisterBuffer(&cuda_resource, colorVBO, cudaGraphicsMapFlagsNone);
	if (cudaStatus != cudaSuccess)
	{
		puts("connect error");
		perror(cudaGetErrorString(cudaStatus));
		exit(1);
	}
	//
	// Create the projection matrix
	//
	glUseProgram(shaderProgram);
	glm_ortho(-1, 1, -1, 1, -1.0f, 200, projection);
	//
	// Depth and transparency
	//
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDepthFunc(GL_LESS);
	//
	// Upadte uniforms
	//
	int loc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, projection[0]);
	#if ORDER == 4
	vec3 scale = { 0.008, 0.008, 0.008 };
	#else if ORDER == 5
	vec3 scale = { 0.003, 0.003, 0.003 };
	#endif
	glm_scale(model, scale);
	loc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(loc, 1, GL_FALSE, model[0]);
	//
	// Update camera
	//
	updateCamera();
	glClearColor(0.1, 0.2, 0.2, 1.0);
	glUseProgram(shaderProgram);
	//
	// Define callback routines
	//
	glutKeyboardFunc(&keyboard);
	glutSpecialFunc(&specialKeys);
	glutIdleFunc(&animation);
	glutDisplayFunc(&display);
	//
	return 0;
}

/* 
 * Program entry point.
 */
int main(int argc, char** argv)
{
	initCuda();
	//printResults(true);
	initOpenGL(argc, argv);
	start = GetTickCount();
	glutMainLoop();
	return EXIT_SUCCESS;
}
