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
"layout(location = 1) in vec3 aOffset;\n"
"layout(location = 2) in vec3 aColor;\n"
"out vec3 fColor;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"void main()\n"
"{\n"
"	gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);\n"
"	fColor = aColor;\n"
"}\0";

unsigned int vertexShader;

const char* fragmentShaderSource = "#version 460 core\n"
"in vec3 fColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"	if(fColor==vec3(0.6,0.6,0.7))"
"		FragColor = vec4(0.6,0.6,0.7,0.05);\n"
"	else\n"
"		FragColor = vec4(fColor, 1);\n"
"}\0";

unsigned int shaderProgram;
unsigned int vao;

mat4 model = GLM_MAT4_IDENTITY_INIT;
mat4 view = GLM_MAT4_IDENTITY_INIT;
mat4 projection;

// Camera

float yaw = -90;
float pitch = 0;

vec3 cameraPos;
vec3 cameraFront;
vec3 cameraUp = { 0.0f, 1.0f, 0.0f };

Cell* host_lattice;
Cell* dev_lattice;

struct cudaGraphicsResource* cuda_resource;
unsigned int colorVBO;
vec3 colors[SIDE3];
size_t num_bytes;

cudaError_t cudaStatus;
DWORD start;

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
	hologram << <GRID1, BLOCK1 >> > (dev_lattice);
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
	//
	// Initialize common point
	//
	vec3 pointVertices = { 0, 0, 0 };
	//
	// Initialize instance translations
	//
	vec3 translations[SIDE3];
	int index = 0;
	for (int z = 0; z < SIDE; z++)
	{
		for (int y = 0; y < SIDE; y++)
		{
			for (int x = 0; x < SIDE; x++)
			{
				translations[index][0] = x / (float)SIDE - 0.5f;
				translations[index][1] = y / (float)SIDE - 0.5f;
				translations[index][2] = z / (float)SIDE - 0.5f;
				index++;
			}
		}
	}
	//
	// Initialize instance colors
	//
	index = 0;
	for (int z = 0; z < SIDE; z++)
	{
		for (int y = 0; y < SIDE; y++)
		{
			for (int x = 0; x < SIDE; x++)
			{
				colors[index][0] = 0.5f;
				colors[index][1] = 0.5f;
				colors[index][2] = 0.8f;
				index++;
			}
		}
	}
	glUseProgram(shaderProgram);
	//
	// Create vbos
	//
	unsigned int pointVBO, positionVBO, colorVBO;
	glGenBuffers(1, &pointVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3), pointVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * SIDE3, &translations[0], GL_STATIC_DRAW);
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
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	//
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	glVertexAttribDivisor(1, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	glVertexAttribDivisor(2, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//
	// Voxel size
	//
	glPointSize(4);	
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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//
	// Upadte uniforms
	//
	int loc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, projection[0]);
	vec3 v = { 0, 0, 0 };
	glm_translate(model, v);
	loc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(loc, 1, GL_FALSE, model[0]);
	updateCamera();
	//
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
	glutKeyboardFunc(&keyboard);
	glutIdleFunc(&animation);
	//
	// Draw first frame 
	//
	glUseProgram(shaderProgram);
	//
	// Set display function
	//
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
