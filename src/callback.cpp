/*
 * callback.cpp - Com detecção de clique/hover no gizmo e cubo visual
*/

#include "GUI.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <mouse_helper.h>
#include <windows.h>
#include "hslider.h"
#include "vslider.h"
#include "radio.h"
#include "tickbox.h"
#include "projection.h"
#include "text_renderer.h"
#include "globals.h"
#include "menubar.h"
#include "model/simulation.h"
#include "animator.h"
#include "replay_progress.h"
#include "tomography.h"
#include "app_context.h"
#include "render_pipeline.h"

void toggleFullscreen(GLFWwindow* window);

// ============================================================================
// DEFINIÇÕES DAS VARIÁVEIS DO CUBO E HOVER (namespace framework)
// ============================================================================
namespace framework {
    bool showDragCube = false;
    float dragCubeX = 0.0f;
    float dragCubeY = 0.0f;
    int gizmoHoverAxis = -1;   // -1 = nenhum, 0=X,1=Y,2=Z
}

namespace framework
{
    extern bool showExitDialog;
    extern bool showHelp;
    extern HSlider hslider;
    extern VSlider vslider;
    extern Tickbox *scenarioHelpToggle;
    extern bool showAboutDialog;
    extern float axisLength;
    extern ReplayProgressBar *replayProgress;

    void onDelayToggled(Tickbox* toggled);
    void updateProjection();
    bool isIn3DZone(double xpos, double ypos, int windowWidth, int windowHeight);

    void setViewFromRadio(OrbitCamera& cam, int viewIndex) {
        switch (viewIndex) {
            case 0: // Isometric
                cam.yaw = 45.0f;
                cam.pitch = 30.0f;
                break;
            case 1: // XY
                cam.yaw = 0.0f;
                cam.pitch = 90.0f;
                break;
            case 2: // YZ
                cam.yaw = 90.0f;
                cam.pitch = 0.0f;
                break;
            case 3: // ZX
                cam.yaw = 0.0f;
                cam.pitch = 0.0f;
                break;
        }
    }

    void sizeCallback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
        ProjectionManager::instance().setViewport(width, height);
        gViewport[0] = 0;
        gViewport[1] = 0;
        gViewport[2] = width;
        gViewport[3] = height;

        resize(width, height);
        setScreenSize(width, height);

        if (replayProgress)
            replayProgress->setPosition(width, height, 100);

        if (menuBar)
            menuBar->Resize((float)width, (float)height);

        updateProjection();
    }

    void buttonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (showExitDialog) return;
    AppContext* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    switch (action)
    {
        case GLFW_PRESS:
            switch (button)
            {
                case GLFW_MOUSE_BUTTON_LEFT:
                {
                    // Menu bar (mantém prioridade)
                    if (menuBar && (ypos <= 30.0 || menuBar->IsMenuOpen())) {
                        menuBar->HandleMouse(xpos, ypos, gViewport[2], gViewport[3], true);
                        if (menuBar->IsMenuOpen() || ypos <= 30.0) return;
                    }

                    // ============================================================
                    // GIZMO THUMB DETECTION (prioridade máxima)
                    // ============================================================
                    if (showGizmo && (pause || currentMode == REPLAY))
                    {
                        float mx = static_cast<float>(xpos);
                        float my = static_cast<float>(ypos);
                        const float hitRadius = 15.0f;  // raio de clique (bolha + margem)
                        int bestAxis = -1;
                        float bestDist = hitRadius;
                        for (int axis = 0; axis < 3; ++axis)
                        {
                            float dx = mx - gGizmoProj.ex[axis];
                            float dy = my - gGizmoProj.ey[axis];
                            float dist = std::hypot(dx, dy);
                            if (dist < bestDist)
                            {
                                bestDist = dist;
                                bestAxis = axis;
                            }
                        }
                        if (bestAxis != -1)
                        {
                            // Ativa o arrasto do gizmo
                            thumb.active         = true;
                            thumb.axis           = bestAxis;
                            thumb.dragging       = true;
                            thumb.startMouseX    = xpos;
                            thumb.startMouseY    = ypos;
                            thumb.initialPosition = 0.5f; // não usado, mas mantido
                            thumb.position        = 0.5f * gGizmoProj.radius;
                            thumb.startOffset[0] = gConfig.view.vis_dx;
                            thumb.startOffset[1] = gConfig.view.vis_dy;
                            thumb.startOffset[2] = gConfig.view.vis_dz;
                            thumb.startOffsetF[0] = (float)gConfig.view.vis_dx;
                            thumb.startOffsetF[1] = (float)gConfig.view.vis_dy;
                            thumb.startOffsetF[2] = (float)gConfig.view.vis_dz;
                            return;   // clique consumido pelo gizmo
                        }
                    }

                    // ============================================================
                    // Demais controles UI (sliders, tickboxes, etc.)
                    // ============================================================
                    vslider.onMouseButton(button, action, xpos, ypos);
                    hslider.onMouseButton(button, action, xpos, ypos);
                    if (hslider.isDragging() || vslider.isDragging()) {
                        float sliderValue = hslider.getValue();
                        tomography::setSlicePosition(sliderValue);
                        tomography::update();
                        return;
                    }

                    int mouseX = static_cast<int>(xpos);
                    int mouseY = static_cast<int>(ypos);

                    for (Tickbox& cb : data3D) {
                        cb.onClick(mouseX, mouseY);
                        if (cb.contains(mouseX, mouseY)) return;
                    }
                    for (Tickbox& cb : delays) {
                        if (cb.contains(mouseX, mouseY)) {
                            cb.onClick(mouseX, mouseY);
                            onDelayToggled(&cb);
                            return;
                        }
                    }
                    if (scenarioHelpToggle->contains(mouseX, mouseY)) {
                        scenarioHelpToggle->onClick(mouseX, mouseY);
                        return;
                    }
                    for (size_t i = 0; i < views.size(); ++i) {
                        if (views[i].contains(mouseX, mouseY)) {
                            for (Radio& r : views) r.setSelected(false);
                            views[i].setSelected(true);
                            if (ctx) setViewFromRadio(ctx->camera, (int)i);
                            return;
                        }
                    }
                    if (layerList) layerList->poll(mouseX, mouseY);
                    for (size_t i = 0; i < projectRads.size(); ++i) {
                        if (projectRads[i].contains(mouseX, mouseY)) {
                            for (Radio& r : projectRads) r.setSelected(false);
                            projectRads[i].setSelected(true);
                            updateProjection();
                            return;
                        }
                    }
                    // Tomography controls
                    if (tomoEnable && tomoEnable->contains(mouseX, mouseY)) {
                        tomoEnable->onClick(mouseX, mouseY);
                        if (!tomoEnable->getState()) {
                            setPipelineState(RenderPipelineState::FULL_VOLUME);
                            return;
                        }
                        for (Radio& r : tomoDirs) r.setSelected(false);
                        if (!tomoDirs.empty()) {
                            tomoDirs[0].setSelected(true);
                            setPipelineState(RenderPipelineState::TOMOGRAPHY_XY);
                        }
                        return;
                    }
                    else if (tomoEnable && tomoEnable->getState()) {
                        for (size_t i = 0; i < tomoDirs.size(); ++i) {
                            if (tomoDirs[i].contains(mouseX, mouseY)) {
                                for (Radio& r : tomoDirs) r.setSelected(false);
                                tomoDirs[i].setSelected(true);
                                switch (i) {
                                    case 0: setPipelineState(RenderPipelineState::TOMOGRAPHY_XY); break;
                                    case 1: setPipelineState(RenderPipelineState::TOMOGRAPHY_YZ); break;
                                    case 2: setPipelineState(RenderPipelineState::TOMOGRAPHY_ZX); break;
                                }
                                return;
                            }
                        }
                    }
                    if (gHelpLink) {
                        int mx = (int)xpos, my = (int)ypos;
                        int flippedY = height - my;
                        if (gHelpLink->contains(mx, flippedY, height)) {
                            system("start https://github.com/automaton3d/automaton/blob/master/help.md");
                            return;
                        }
                    }
                    // Nada mais capturou → orbita a câmera
                    if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3])) {
                        setLeftClicked(true);
                        setClickPoint(xpos, ypos);
                    }
                    if (showAboutDialog) {
                        int closeX = gViewport[2]/2 - 40, closeY = gViewport[3] - 180;
                        if (xpos >= closeX && xpos <= closeX+100 && ypos >= closeY-30 && ypos <= closeY+10)
                            showAboutDialog = false;
                    }
                    break;
                }
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    if (isIn3DZone(xpos, ypos, gViewport[2], gViewport[3]) &&
                        !hslider.isDragging() && !vslider.isDragging())
                    {
                        setMiddleClicked(true);
                        setClickPoint(xpos, ypos);
                    }
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    if (isIn3DZone(xpos, ypos, gViewport[2], gViewport[3]) &&
                        !hslider.isDragging() && !vslider.isDragging())
                    {
                        setRightClicked(true);
                        setClickPoint(xpos, ypos);
                    }
                    break;
            }
            break;

        case GLFW_RELEASE:
            switch (button)
            {
                case GLFW_MOUSE_BUTTON_LEFT:
                {
                    bool wasSliderDragging = hslider.isDragging() || vslider.isDragging();
                    hslider.onMouseButton(button, action, xpos, ypos);
                    vslider.onMouseButton(button, action, xpos, ypos);
                    if (!wasSliderDragging) setLeftClicked(false);
                    thumb.active = false;
                    thumb.dragging = false;
                    framework::showDragCube = false;   // desliga o cubo
                    break;
                }
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    setMiddleClicked(false);
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    setRightClicked(false);
                    break;
            }
            break;
    }
}

void moveCallback(GLFWwindow* window, double xpos, double ypos)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Sliders (não interferem no gizmo)
    hslider.onMouseDrag((int)xpos, (int)ypos);
    hslider.onMouseMove((int)xpos, (int)ypos);
    if (hslider.isDragging() && tomoEnable && tomoEnable->getState()) {
        float sliderValue = hslider.getValue();
        tomography::setSlicePosition(sliderValue);
        tomography::update();
    }

    if (gHelpLink) {
        int mx = (int)xpos, my = (int)ypos;
        int flippedY = height - my;
        helpHover = gHelpLink->contains(mx, flippedY, height);
    }

    if (!hslider.isDragging() && !vslider.isDragging() &&
        isIn3DZone(xpos, ypos, width, height))
    {
        setClickPoint(xpos, ypos);
    }

    // ============================================================
    // GIZMO THUMB DRAGGING (atualiza offsets e mostra cubo)
    // ============================================================
    if ((pause || currentMode == REPLAY) && thumb.active && thumb.dragging && showGizmo)
    {
        float mx = (float)xpos, my = (float)ypos;
        float dMx = mx - (float)thumb.startMouseX;
        float dMy = my - (float)thumb.startMouseY;
        float dAx = gGizmoProj.ex[thumb.axis] - gGizmoProj.cx;
        float dAy = gGizmoProj.ey[thumb.axis] - gGizmoProj.cy;
        float fullProjLength = std::hypot(dAx, dAy);
        if (fullProjLength > 1e-6f)
        {
            float projDistance = (dMx * dAx + dMy * dAy) / fullProjLength;
            float cellDelta = (projDistance / fullProjLength) * (float)automaton::EL;
            float newOffset = (float)thumb.startOffset[thumb.axis] + cellDelta;
            int newVisOffset = static_cast<int>(std::round(newOffset));
            if (thumb.axis == 0)      gConfig.view.vis_dx = newVisOffset;
            else if (thumb.axis == 1) gConfig.view.vis_dy = newVisOffset;
            else if (thumb.axis == 2) gConfig.view.vis_dz = newVisOffset;
        }
        // Mostra o cubo durante o arrasto (na posição do mouse)
        framework::showDragCube = true;
        framework::dragCubeX = (float)xpos;
        framework::dragCubeY = (float)ypos;
        framework::gizmoHoverAxis = thumb.axis;
    }
    else
    {
        // ============================================================
        // HOVER DETECTION (cubo na ponta do eixo, sem arrastar)
        // ============================================================
        if (showGizmo && (pause || currentMode == REPLAY))
        {
            float mx = (float)xpos, my = (float)ypos;
            const float hoverRadius = 14.0f;
            int bestAxis = -1;
            float bestDist = hoverRadius;
            for (int axis = 0; axis < 3; ++axis)
            {
                float dx = mx - gGizmoProj.ex[axis];
                float dy = my - gGizmoProj.ey[axis];
                float dist = std::hypot(dx, dy);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestAxis = axis;
                }
            }
            framework::gizmoHoverAxis = bestAxis;
            if (bestAxis != -1)
            {
                // Mostra o cubo na ponta do eixo (sem arrastar)
                framework::showDragCube = true;
                framework::dragCubeX = gGizmoProj.ex[bestAxis];
                framework::dragCubeY = gGizmoProj.ey[bestAxis];
            }
            else
            {
                framework::showDragCube = false;
            }
        }
        else
        {
            framework::gizmoHoverAxis = -1;
            framework::showDragCube = false;
        }
    }

    // Remove completamente qualquer cursor de redimensionamento
    glfwSetCursor(window, nullptr);
}

    void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        if (!hslider.isDragging() && !vslider.isDragging() &&
            isIn3DZone(mouseX, mouseY, gViewport[2], gViewport[3]))
        {
            setScrollDirection((xoffset + yoffset) > 0);
        }
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        static double lastKeyTime = 0.0;
        double now = glfwGetTime();
        if (now - lastKeyTime < 0.2)
            return;
        lastKeyTime = now;

        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                {
#ifdef _WIN32
                    int result = MessageBoxW(
                        nullptr,
                        L"Do you really want to exit?",
                        L"Exit Confirmation",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SYSTEMMODAL
                    );
                    if (result == IDYES)
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
#else
                    framework::showExitDialog = true;
#endif
                    break;
                }
                case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                    setSpeed(5.f);
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    setSpeed(0.1f);
                    break;
                case GLFW_KEY_T:
                    if (tomoEnable)
                    {
                        tomoEnable->toggle();
                        if (!tomoEnable->getState())
                        {
                            setPipelineState(RenderPipelineState::FULL_VOLUME);
                        }
                        else
                        {
                            for (auto& r : tomoDirs)
                                r.setSelected(false);
                            if (!tomoDirs.empty())
                            {
                                tomoDirs[0].setSelected(true);
                                setPipelineState(RenderPipelineState::TOMOGRAPHY_XY);
                            }
                        }
                    }
                    break;
                case GLFW_KEY_M:
                    sound(true);
                    break;
                case GLFW_KEY_P:
                    if (action == GLFW_PRESS) {
                        pause = !pause;
                        showGizmo = !showGizmo;
                        if (!pause && currentMode == SIMULATION) {
                            gConfig.view.vis_dx = 0;
                            gConfig.view.vis_dy = 0;
                            gConfig.view.vis_dz = 0;
                        }
                        glfwPostEmptyEvent();
                    }
                    break;
                case GLFW_KEY_F1:
                    if (action == GLFW_PRESS) {
                        showHelp = !showHelp;
                    }
                    break;
                case GLFW_KEY_F8:
                    if (action == GLFW_PRESS) {
                        toggleFullscreen(window);
                    }
                    break;
                default:
                    break;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            switch (key)
            {
                case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    setSpeed(1.f);
                    break;
                default:
                    break;
            }
        }
    }

    void errorCallback(int error, const char* description)
    {
        std::cerr << "[GLFW Error " << error << "] " << description << std::endl;
    }
}