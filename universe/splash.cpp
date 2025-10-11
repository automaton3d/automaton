#include <GLFW/glfw3.h>
#include <GL/freeglut.h>
#include <iostream>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Button structure
struct Button
{
  float x, y, w, h;
  const char* label;
};

Button btn1 = { -0.5f, -0.25f - 0.30f, 1.0f, 0.25f, "Simulation" };
Button btn2 = { -0.5f, -0.55f - 0.30f, 1.0f, 0.25f, "Statistics" };

// Logo texture
GLuint logoTexture = 0;
int logoWidth = 0, logoHeight = 0;

// Global selection variable
volatile int selection = 0; // 1 for Simulation, 2 for Statistics
volatile bool shouldExit = false;

// Forward declarations
int runSimulation();
int runStatistics();

// Load PNG image and create texture
bool loadLogo(const char* filename)
{
  int channels;
  unsigned char* data = stbi_load(filename, &logoWidth, &logoHeight, &channels, 4);
  if (!data)
  {
    std::cerr << "Failed to load logo: " << filename << std::endl;
    return false;
  }
  glGenTextures(1, &logoTexture);
  glBindTexture(GL_TEXTURE_2D, logoTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logoWidth, logoHeight,
               0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);
  return true;
}

// Draw logo
void drawLogo()
{
  if (logoTexture == 0)
    return;
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, logoTexture);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  float size = 1.0f;
  float yPos = 0.4f - 0.15f;
  glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-size/2, yPos - size/2);
    glTexCoord2f(1, 1); glVertex2f( size/2, yPos - size/2);
    glTexCoord2f(1, 0); glVertex2f( size/2, yPos + size/2);
    glTexCoord2f(0, 0); glVertex2f(-size/2, yPos + size/2);
  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

// Draw a button
void drawButton(const Button& b, bool isDefault = false)
{
  // Button shadow
  glColor3f(0.1f, 0.1f, 0.1f);
  float offset = 0.02f;
  glBegin(GL_QUADS);
    glVertex2f(b.x + offset,     b.y - offset);
    glVertex2f(b.x + b.w + offset, b.y - offset);
    glVertex2f(b.x + b.w + offset, b.y + b.h - offset);
    glVertex2f(b.x + offset,     b.y + b.h - offset);
  glEnd();
  // Button background (highlight Simulation button if default)
  glColor3f(isDefault ? 0.3f : 0.2f, isDefault ? 0.7f : 0.6f, isDefault ? 0.9f : 0.8f);
  glBegin(GL_QUADS);
    glVertex2f(b.x,       b.y);
    glVertex2f(b.x + b.w,   b.y);
    glVertex2f(b.x + b.w,   b.y + b.h);
    glVertex2f(b.x,       b.y + b.h);
  glEnd();
  // Button border (thicker and brighter for default button)
  glColor3f(isDefault ? 0.5f : 0.3f, isDefault ? 0.9f : 0.7f, isDefault ? 1.0f : 0.9f);
  glLineWidth(isDefault ? 3.0f : 2.0f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(b.x,       b.y);
    glVertex2f(b.x + b.w,   b.y);
    glVertex2f(b.x + b.w,   b.y + b.h);
    glVertex2f(b.x,       b.y + b.h);
  glEnd();
  // Draw label text (centered)
  glColor3f(1.0f, 1.0f, 1.0f);
  const char* c = b.label;
  int textWidth = 0;
  while (*c)
  {
    textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c);
    c++;
  }
  float tx = b.x + b.w/2 - (textWidth * 1.0f / glutGet(GLUT_WINDOW_WIDTH));
  float ty = b.y + b.h/2 - 0.015f; // Adjusted for better vertical centering
  glRasterPos2f(tx, ty);
  c = b.label;
  while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c++);
}

// Draw title above the logo
void drawTitle()
{
  const char* title = "It from bit: a concrete attempt";
  glColor3f(0.4f, 0.7f, 1.0f);
  int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
  int textWidth = 0;
  const char* c = title;
  while (*c) textWidth += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c++);
  float tx = - (textWidth * 2.0f / windowWidth) / 2.0f;
  float ty = 0.85f;
  for (float dx = -0.001f; dx <= 0.001f; dx += 0.001f) {
    for (float dy = -0.001f; dy <= 0.001f; dy += 0.001f) {
      glRasterPos2f(tx + dx, ty + dy);
      c = title;
      while (*c) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c++);
    }
  }
}

// Display callback
void display()
{
  glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  drawTitle();
  drawLogo();
  drawButton(btn1, true); // Simulation button is default
  drawButton(btn2, false);
  glutSwapBuffers();
}

// Idle callback to check if we should exit
void idle()
{
  if (shouldExit)
  {
    glutLeaveMainLoop(); // Use FreeGLUT's proper exit function
    return;
  }

  glutPostRedisplay();
}

// Check if mouse click is inside button
bool insideButton(int mx, int my, int w, int h, const Button& b)
{
  float x = (float)mx / w * 2.0f - 1.0f;
  float y = 1.0f - (float)my / h * 2.0f;
  return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
}

void mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (insideButton(x, y, w, h, btn1))
    {
      selection = 1;
      shouldExit = true;
    }
    else if (insideButton(x, y, w, h, btn2))
    {
      selection = 2;
      shouldExit = true;
    }
  }
}

// Keyboard callback for Enter key
void keyboard(unsigned char key, int x, int y)
{
  if (key == 13) // Enter key
  {
    selection = 1; // Select Simulation (default button)
    shouldExit = true;
  }
}

// Callback to prevent resizing
void reshape(int w, int h)
{
  static bool isResizing = false;
  if (isResizing) return;

  if (w != 400 || h != 400)
  {
    isResizing = true;
    glutReshapeWindow(400, 400);
    isResizing = false;
  }

  glViewport(0, 0, 400, 400);
}

// Close callback
void closeFunc()
{
  shouldExit = true;
}

int main(int argc, char** argv)
{
  // Check for command line arguments to launch directly
  if (argc > 1)
  {
    if (strcmp(argv[1], "--simulation") == 0)
    {
      std::cout << "Starting simulation mode..." << std::endl;
      std::cout << "Current working directory: " << std::endl;

      char cwd[MAX_PATH];
      GetCurrentDirectory(MAX_PATH, cwd);
      std::cout << cwd << std::endl;

      // Set error callback before init
      glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
      });

      // Initialize GLFW
      std::cout << "Attempting to initialize GLFW..." << std::endl;
      int initResult = glfwInit();
      std::cout << "glfwInit() returned: " << initResult << std::endl;

      if (!initResult)
      {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        std::cerr << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
      }

      std::cout << "GLFW initialized successfully" << std::endl;

      int result = runSimulation();

      std::cout << "Simulation ended with result: " << result << std::endl;
      glfwTerminate();
      return result;
    }
    else if (strcmp(argv[1], "--statistics") == 0)
    {
      std::cout << "Starting statistics mode..." << std::endl;
      return runStatistics();
    }
  }

  // Otherwise show splash screen
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
  glutInitWindowSize(400, 400);

  // Center the window
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  int windowWidth = 400;
  int windowHeight = 400;
  glutInitWindowPosition(
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2
  );

  int window = glutCreateWindow("Toy Universe");
  glutReshapeFunc(reshape);

  // Enable transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Load logo
  if (!loadLogo("logo.png"))
  {
    std::cerr << "Warning: Could not load logo.png" << std::endl;
  }

  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard); // Register keyboard callback
  glutIdleFunc(idle);
  glutCloseFunc(closeFunc);

  // Run the GLUT main loop
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutMainLoop();

  // Destroy window explicitly after loop exits
  if (glutGetWindow() > 0)
  {
    glutDestroyWindow(window);
  }

  // Clean up GLUT completely
  std::cout << "GLUT loop exited. Selection: " << selection << std::endl;

  // Give time for window to close
  Sleep(100);

  glutInit(&argc, argv);

  // After glutMainLoop() returns, launch the selected application
  if (selection == 1)
  {
    std::cout << "Launching simulation in same process..." << std::endl;

    // Give GLUT time to fully clean up
    Sleep(500);

    // Initialize GLFW for simulation
    std::cout << "Initializing GLFW..." << std::endl;

    if (!glfwInit())
    {
      std::cerr << "Failed to initialize GLFW after GLUT cleanup" << std::endl;
      MessageBox(NULL, "Failed to initialize GLFW", "Error", MB_OK | MB_ICONERROR);
      return -1;
    }

    std::cout << "GLFW initialized, starting simulation..." << std::endl;
    int result = runSimulation();
    glfwTerminate();
    return result;
  }
  else if (selection == 2)
  {
    std::cout << "Launching statistics in same process..." << std::endl;

    // Statistics window - just call it directly
    // GLUT is already initialized from the splash
    return runStatistics();
  }

  return 0;
}
