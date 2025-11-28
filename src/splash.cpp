/*
 * splash.cpp
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <map>

#include "button.h"
#include "ca_window.h"
#include "draw_utils.h"
#include "dropdown.h"
#include "globals.h"
#include "shader_utils.h"
#include "logo.h"
#include "text_renderer.h"
#include "tickbox.h"

// Simple color shader (from test_button.cpp)
const char* colorVertexShader = R"(
  #version 460 core
  layout (location = 0) in vec2 aPos;
  uniform mat4 uMVP;
  void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
  }
)";

const char* colorFragmentShader = R"(
  #version 460 core
  out vec4 FragColor;
  uniform vec3 uColor;
  void main() {
    FragColor = vec4(uColor, 1.0);
  }
)";

// Text shader (Unified with shader_utils)
const char* textVertexShader = R"(
  #version 460 core
  layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
  out vec2 TexCoords;
  uniform mat4 projection;
  void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
  }
)";

const char* textFragmentShader = R"(
  #version 460 core
  in vec2 TexCoords;
  out vec4 color;
  uniform sampler2D text;
  uniform vec3 textColor;
  void main() {
    float alpha = texture(text, TexCoords).r;
    color = vec4(textColor, alpha);
  }
)";

constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

namespace splash {

    int lattice_size = 21;
    int numLayers    = 10;
    int selection    = -1;
    bool shouldExit  = false;
    bool helpHover   = false;

    // UI widgets - all pointers to forward-declared types
    Button* simBtn      = nullptr;
    Button* statBtn     = nullptr;
    Button* replayBtn   = nullptr;
    Button* helpLink    = nullptr;
    Tickbox* startPausedBox = nullptr;
    Dropdown* sizeDropdown  = nullptr;
    Dropdown* layerDropdown = nullptr;
    Dropdown* scenarioDropdown = nullptr;
    framework::Logo* logo_splash   = nullptr;  // Use full namespace qualifier
    std::vector<std::string> scenarioOptions = {
        "Scenario 1: Wave Propagation",
        "Scenario 2: Particle Collision",
        "Scenario 3: Pattern Formation",
        // Adicione mais conforme necessário
    };

    static GLFWwindow* g_window = nullptr;
    void setWindow(GLFWwindow* w) { g_window = w; }
}
     
// ---------------------------------------------------------------------
// Utility: draw raised panel
// ---------------------------------------------------------------------
void drawRaisedPanel(float x, float y, float w, float h, int winW, int winH) {
    drawQuad2D(x, y, x+w, y+h,
               glm::vec3(0.85f,0.85f,0.9f),
               winW, winH);

    drawLineLoop2D({{x,y},{x+w,y},{x+w,y+h},{x,y+h}},
                   glm::vec3(0.7f,0.7f,0.8f),
                   winW, winH, 1.0f);
}

// ---------------------------------------------------------------------
// Title drawing
// ---------------------------------------------------------------------
void drawTitle(int w, int h) {
    if (!textRenderer) return;

    const std::string title = "It from bit: a concrete attempt";
    float scale = 0.6f;

    float textWidth = textRenderer->measureTextWidth(title, scale);
    float tx = (w - textWidth) / 2.0f;

    // Place 50 pixels below the top edge
    float ty = h - 50.0f;

    textRenderer->RenderText(title, tx, ty, scale,
                             glm::vec3(0.4f, 0.7f, 1.0f),
                             w, h);
}

// ---------------------------------------------------------------------
// Display function
// ---------------------------------------------------------------------
void display(GLFWwindow* window) {
    glClearColor(0.95f, 0.95f, 0.97f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawTitle(WINDOW_WIDTH, WINDOW_HEIGHT);

    if (splash::logo_splash)
        splash::logo_splash->draw((WINDOW_WIDTH - 200) / 2, 80, 1.0f);

    drawRaisedPanel(30, 125, WINDOW_WIDTH - 50, 180, WINDOW_WIDTH, WINDOW_HEIGHT);
    drawRaisedPanel(WINDOW_WIDTH - 260, 210, 220, 80, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (textRenderer) {
        if (splash::simBtn)
            splash::simBtn->draw(colorProgram2D, *textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
        if (splash::statBtn)
            splash::statBtn->draw(colorProgram2D, *textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
        if (splash::replayBtn)
            splash::replayBtn->draw(colorProgram2D, *textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

        if (splash::helpLink)
            splash::helpLink->drawAsHyperlink(*textRenderer,
                                              splash::helpHover,
                                              WINDOW_WIDTH, WINDOW_HEIGHT);

    splash::sizeDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    splash::layerDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    splash::scenarioDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    splash::startPausedBox->setFontScale(0.3f);
    splash::startPausedBox->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Draw all dropdowns *again* (only open ones will draw their list,
    // which puts them on top of everything else)
    if (splash::sizeDropdown && splash::sizeDropdown->isOpen_)
        splash::sizeDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (splash::layerDropdown && splash::layerDropdown->isOpen_)
        splash::layerDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (splash::scenarioDropdown && splash::scenarioDropdown->isOpen_)
        splash::scenarioDropdown->draw(*textRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);


    }

    glfwSwapBuffers(window);
}

// ---------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Invert Y coordinate (GLFW uses top-left, OpenGL uses bottom-left)
        int mouseX = (int)xpos;
        int mouseY = height - (int)ypos;

        // Check which dropdown was clicked (header or items)
        Dropdown* clickedDropdown = nullptr;
        bool clickedOnHeader = false;
        
        if (splash::sizeDropdown && splash::sizeDropdown->containsHeader(mouseX, mouseY, width, height)) {
            clickedDropdown = splash::sizeDropdown;
            clickedOnHeader = true;
        }
        else if (splash::layerDropdown && splash::layerDropdown->containsHeader(mouseX, mouseY, width, height)) {
            clickedDropdown = splash::layerDropdown;
            clickedOnHeader = true;
        }
        else if (splash::scenarioDropdown && splash::scenarioDropdown->containsHeader(mouseX, mouseY, width, height)) {
            clickedDropdown = splash::scenarioDropdown;
            clickedOnHeader = true;
        }
        
        // If clicking a header, close all OTHER dropdowns first
        if (clickedOnHeader) {
            if (splash::sizeDropdown && splash::sizeDropdown != clickedDropdown)
                splash::sizeDropdown->close();
            if (splash::layerDropdown && splash::layerDropdown != clickedDropdown)
                splash::layerDropdown->close();
            if (splash::scenarioDropdown && splash::scenarioDropdown != clickedDropdown)
                splash::scenarioDropdown->close();
        }
        
        // Now handle the actual dropdown clicks
        bool dropdownHandled = false;
        
        if (splash::sizeDropdown) {
            if (splash::sizeDropdown->handleClick(mouseX, mouseY, width, height))
                dropdownHandled = true;
        }
        if (splash::layerDropdown) {
            if (splash::layerDropdown->handleClick(mouseX, mouseY, width, height))
                dropdownHandled = true;
        }
        if (splash::scenarioDropdown) {
            if (splash::scenarioDropdown->handleClick(mouseX, mouseY, width, height))
                dropdownHandled = true;
        }

        // If a dropdown item was selected, we're done
        if (dropdownHandled) {
            return;
        }

        // If click was outside all dropdowns, close them all
        bool clickedInsideAnyDropdown = false;
        if (splash::sizeDropdown && splash::sizeDropdown->isMouseOver(mouseX, mouseY, width, height))
            clickedInsideAnyDropdown = true;
        if (splash::layerDropdown && splash::layerDropdown->isMouseOver(mouseX, mouseY, width, height))
            clickedInsideAnyDropdown = true;
        if (splash::scenarioDropdown && splash::scenarioDropdown->isMouseOver(mouseX, mouseY, width, height))
            clickedInsideAnyDropdown = true;

        if (!clickedInsideAnyDropdown) {
            if (splash::sizeDropdown) splash::sizeDropdown->close();
            if (splash::layerDropdown) splash::layerDropdown->close();
            if (splash::scenarioDropdown) splash::scenarioDropdown->close();
        }

        // Check buttons
        if (splash::simBtn && splash::simBtn->contains(mouseX, mouseY)) {
            automaton::calculateParameters(splash::lattice_size, splash::numLayers);
            if (!automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
                // show error message
            }
            else {
              splash::selection = 0;
              splash::shouldExit = true;
            }
        }
        else if (splash::statBtn && splash::statBtn->contains(mouseX, mouseY)) {
            automaton::calculateParameters(splash::lattice_size, splash::numLayers);
            if (!automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
                // show error message
            }
            else {
              splash::selection = 2;
              scenario = 7; // full simulation for statistics
              splash::shouldExit = true;
            }
        }
        else if (splash::replayBtn && splash::replayBtn->contains(mouseX, mouseY)) {
            automaton::calculateParameters(splash::lattice_size, splash::numLayers);
            if (!automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
              // show error message
            }
            else {
              splash::selection = 1;
              splash::shouldExit = true;
            }
        }
        else if (splash::helpLink && splash::helpLink->contains(mouseX, mouseY)) {
            // Platform-independent URL opening
#if defined(_WIN32)
            std::system("start https://github.com/automaton3d/automaton/blob/master/help.md");
#elif defined(__APPLE__)
            std::system("open https://github.com/automaton3d/automaton/blob/master/help.md");
#else
            std::system("xdg-open https://github.com/automaton3d/automaton/blob/master/help.md");
#endif
        }

        // Check tickbox
        if (splash::startPausedBox)
            splash::startPausedBox->onMouseButton(mouseX, mouseY, true);
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Handle dropdown scrolling if any dropdown is open
    if (splash::sizeDropdown && splash::sizeDropdown->isOpen_)
        splash::sizeDropdown->scroll(yoffset > 0 ? -1 : 1);
    if (splash::layerDropdown && splash::layerDropdown->isOpen_)
        splash::layerDropdown->scroll(yoffset > 0 ? -1 : 1);
    if (splash::scenarioDropdown && splash::scenarioDropdown->isOpen_)
        splash::scenarioDropdown->scroll(yoffset > 0 ? -1 : 1);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            splash::selection = -1;
            splash::shouldExit = true;
        }
        else if (key == GLFW_KEY_ENTER) {
            automaton::calculateParameters(splash::lattice_size, splash::numLayers);
            if (!automaton::tryAllocate(splash::lattice_size, splash::numLayers)) {
                // show error message
            } else {
                splash::selection = 0;
                splash::shouldExit = true;
            }
        }
    }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    gViewport[2] = width;
    gViewport[3] = height;
}

void closeCallback(GLFWwindow* window) {
    splash::selection = -1;
    splash::shouldExit = true;
}

void passiveMotionCallback(GLFWwindow* window, double xpos, double ypos) {
    splash::helpHover = splash::helpLink && splash::helpLink->contains((int)xpos, (int)ypos);
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    // Update dropdown hover states
    if (splash::sizeDropdown)
        splash::sizeDropdown->updateHover((int)xpos, (int)ypos, width, height);
    if (splash::layerDropdown)
        splash::layerDropdown->updateHover((int)xpos, (int)ypos, width, height);
    if (splash::scenarioDropdown)
        splash::scenarioDropdown->updateHover((int)xpos, (int)ypos, width, height);
}

// --- Shader Compilation Utility (Add this after the sources) ---

unsigned int compileShader(const char* vSrc, const char* fSrc) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, nullptr);
    glCompileShader(vs);

    // TODO: Add error checking for vs compilation
    
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, nullptr);
    glCompileShader(fs);

    // TODO: Add error checking for fs compilation

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------
int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    std::cout << "GLFW init OK" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Toy Universe", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "Window created OK" << std::endl;

    glfwMakeContextCurrent(window);
    //glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "GLAD init OK" << std::endl;

    glEnable(GL_MULTISAMPLE);
    glDisable(GL_DEPTH_TEST); 
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    gViewport[2] = width;
    gViewport[3] = height;

    // 1. Compile Texture Shader (Used by Logo, assuming framework::compileTextureShader() is available)
    textureProgram2D = framework::compileTextureShader();
    textureMvpLoc    = glGetUniformLocation(textureProgram2D, "uMVP");
    textureSamplerLoc= glGetUniformLocation(textureProgram2D, "uTexture");
    
    // 2. Compile Color Shader (FIX: Used by Button/Tickbox/Dropdown background)
    colorProgram2D  = compileShader(colorVertexShader, colorFragmentShader); 
    colorMvpLoc2D   = glGetUniformLocation(colorProgram2D, "uMVP");
    colorColorLoc2D = glGetUniformLocation(colorProgram2D, "uColor");
    
    // 3. Compile Text Shader (FIX: Used by TextRenderer)
    textProgram = framework::compileTextShader();
    
    splash::setWindow(window);

    // Register callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowCloseCallback(window, closeCallback);
    glfwSetCursorPosCallback(window, passiveMotionCallback);

    textRenderer = new TextRenderer();
    bool fontLoaded = false;

    // Candidate font paths (platform‑aware fallbacks)
    const char* fontPaths[] = {
        "fonts/arial.ttf",
        "arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        nullptr
    };

    for (int i = 0; fontPaths[i] != nullptr; ++i) {
        // FIX: Pass the compiled text program to the init function
        if (textRenderer->init(fontPaths[i], 48, textProgram)) {
            std::cout << "Font loaded successfully: " << fontPaths[i] << std::endl;
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        std::cerr << "WARNING: Could not load any font! Text will not render.\n"
                  << "Please place Arial.ttf in a 'fonts/' folder next to the executable.\n";
    }

    int xpos = WINDOW_WIDTH - 250;
    splash::simBtn    = new Button(xpos, 220, 200, 40, "Simulation");
    splash::statBtn   = new Button(xpos, 140, 200, 40, "Statistics");
    splash::replayBtn = new Button(xpos, 70, 200, 40, "Replay");

    // Desired dimensions
    int helpW = 100;
    int helpH = 20;

    // Center horizontally
    int helpX = (WINDOW_WIDTH - helpW) / 2;

    // Place 50 pixels above the bottom
    int helpY = 25;  // measured from bottom

    splash::helpLink = new Button(helpX, helpY, helpW, helpH, "Help");

    std::vector<std::string> sizeOptions = {
      "5", "7", "9", "11", "13", "15", "17", "19", "21", "23", "25", "27", "29", "31",
      "35", "37", "39", "41", "43", "45", "47", "49", "51", "53", "55", "57", "59",
      "61", "63", "65", "67", "69", "71", "73", "75", "77", "79", "81", "83", "85", "87", "89"
    };

    splash::sizeDropdown = new Dropdown(50, 253, 200, 30, sizeOptions, "Lattice Size");

    std::vector<std::string> layerOptions = {
      "10", "12", "14", "16", "18", "20", "24", "28", "32", "38", "44",
      "52", "60", "70", "82", "96", "112", "130", "150", "174",
      "200", "230", "264", "300", "320", "340", "360", "364"
    };

    splash::layerDropdown = new Dropdown(50, 203, 200, 30, layerOptions, "Layers");

    std::vector<std::string> scenarioOptions =
    {
      "Wrapping test",
      "Relocate test",
      "Orphan test",
      "Contraction test",
      "Hunting test",
      "Reissue test",
      "Dispersion test",
      "Full simulation"
    };
 
    splash::scenarioDropdown = new Dropdown(50, 153, 200, 30, scenarioOptions, "Scenario");

    splash::startPausedBox = new Tickbox(xpos, 263, "Start Paused");
    float labelCol[3] = {0.0f, 0.0f, 0.0f};
    splash::startPausedBox->setColor(nullptr, labelCol, nullptr, nullptr);
    // Initialize logo
    splash::logo_splash = new framework::Logo("logo_bar.png");

    // Main loop
    while (!glfwWindowShouldClose(window) && !splash::shouldExit) {
        display(window);
        glfwPollEvents();
    }

    // Cleanup
    delete splash::simBtn;
    delete splash::statBtn;
    delete splash::replayBtn;
    delete splash::helpLink;
    delete splash::startPausedBox;
    delete splash::sizeDropdown;
    delete splash::layerDropdown;
    delete splash::scenarioDropdown;
    delete splash::logo_splash;
    delete textRenderer;

    glfwDestroyWindow(window);
    glfwTerminate();

    // Handle selection
    switch (splash::selection) {
        case -1: 
            return 0;
        case 0:  
            return runSimulation(splash::scenarioDropdown->getSelectedIndex(),
                                 splash::startPausedBox->getState());
        case 1:  
            return runReplay();
        case 2:  
            return runStatistics();
        default: 
            return 0;
    }
}