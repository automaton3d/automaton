/*
 * callback.cpp - FIXED VERSION
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

void toggleFullscreen(GLFWwindow* window);

namespace framework
{
    extern bool showExitDialog;
    extern HSlider hslider;
    extern VSlider vslider;
    extern Tickbox *scenarioHelpToggle;
    extern bool showAboutDialog;
    extern float axisLength;
    extern int vis_dx;
    extern int vis_dy;
    extern int vis_dz;
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

        updateProjection();  // Now works due to ADL + namespace scope
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
                    	if (menuBar && (ypos <= 30.0 || menuBar->IsMenuOpen())) {
                    	                        menuBar->HandleMouse(xpos, ypos, gViewport[2], gViewport[3], true);  // Add true here
                    	                        if (menuBar->IsMenuOpen() || ypos <= 30.0) {
                    	                            return;
                    	                        }
                    	                    }
                        #ifdef DEBUG
                        debugClickX = xpos;
                        debugClickY = ypos;
                        showDebugClick = true;
                        #endif

                        // Sliders
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

                        // Data3D tickboxes
                        for (Tickbox& cb : data3D) {
                            cb.onClick(mouseX, mouseY);
                            if (cb.contains(mouseX, mouseY)) return;
                        }

                        // Delays tickboxes
                        for (Tickbox& cb : delays) {
                            if (cb.contains(mouseX, mouseY)) {
                                cb.onClick(mouseX, mouseY);
                                onDelayToggled(&cb);
                                return;
                            }
                        }

                        // Scenario help toggle
                        if (scenarioHelpToggle->contains(mouseX, mouseY)) {
                            scenarioHelpToggle->onClick(mouseX, mouseY);
                            return;
                        }

                        // View radios
                        for (size_t i = 0; i < views.size(); ++i)
                        {
                            if (views[i].contains(mouseX, mouseY))
                            {
                                for (Radio& r : views) r.setSelected(false);
                                views[i].setSelected(true);

                                // >>> Atualiza a câmera conforme o radio selecionado
                                if (ctx) {
                                    setViewFromRadio(ctx->camera, (int)i);
                                }

                                return;
                            }
                        }

                        // Layer list
                        if (layerList)
                            layerList->poll(mouseX, mouseY);

                        // Projection radios
                        for (size_t i = 0; i < projectRads.size(); ++i)
                        {
                            if (projectRads[i].contains(mouseX, mouseY))
                            {
                                for (Radio& r : projectRads) r.setSelected(false);
                                projectRads[i].setSelected(true);
                                updateProjection();
                                return;
                            }
                        }

                        // Tomography controls
                        if (tomoEnable && tomoEnable->contains(mouseX, mouseY)) {
                            tomoEnable->onClick(mouseX, mouseY);
                            for (Radio& radio : tomoDirs) radio.setSelected(false);

                            if (tomoEnable->getState()) {
                                if (!tomoDirs.empty()) {
                                    tomoDirs[0].setSelected(true);
                                    tomographyNeedsUpdate = true; // <-- flag instead of direct update
                                }
                            }
                            return;
                        }

                        else if (tomoEnable && tomoEnable->getState()) {
                            // Handle clicks on direction radios safely
                            for (Radio& radio : tomoDirs) {
                                if (radio.contains(mouseX, mouseY)) {
                                    for (Radio& r : tomoDirs) r.setSelected(false);
                                    radio.setSelected(true);
                                    tomography::update();
                                    return;
                                }
                            }
                        }

                        if (gHelpLink) {
                            int mx = (int)xpos;
                            int my = (int)ypos;
                            int flippedY = height - my;
                            if (gHelpLink->contains(mx, flippedY, height)) {
                                system("start https://github.com/automaton3d/automaton/blob/master/help.md");
                                return;
                            }
                        }

                        // Axis thumb detection
                        // === AXIS THUMB PICKING – FINAL WORKING VERSION ===
                        if ((pause || currentMode == REPLAY))
                        {
                            if (gAxisProjValid)
                            {
                                float mx = static_cast<float>(xpos);
                                float my = static_cast<float>(ypos);
                                const float threshold = 20.0f;

                                int   bestAxis = -1;
                                float bestT    = 0.0f;
                                float bestDist = threshold;

                                for (int axis = 0; axis < 3; ++axis)
                                {
                                    const AxisProjection& ap = gAxisProj[axis];
                                    float dx = ap.x1 - ap.x0;
                                    float dy = ap.y1 - ap.y0;
                                    float len2 = dx*dx + dy*dy;
                                    if (len2 < 1e-6f) continue;

                                    float t = ((mx - ap.x0)*dx + (my - ap.y0)*dy) / len2;
                                    t = glm::clamp(t, 0.0f, 1.0f);

                                    float px = ap.x0 + t * dx;
                                    float py = ap.y0 + t * dy;
                                    float dist = std::hypot(mx - px, my - py);

                                    if (dist < bestDist)
                                    {
                                        bestDist = dist;
                                        bestAxis = axis;
                                        bestT    = t;
                                    }
                                }

                                if (bestAxis != -1)
                                {
                                    thumb.active         = true;
                                    thumb.axis           = bestAxis;
                                    thumb.dragging       = true;
                                    thumb.startMouseX    = xpos;
                                    thumb.startMouseY    = ypos;

                                    // Store initial position as parametric t (0.0 to 1.0)
                                    thumb.initialPosition = bestT;
                                    thumb.position        = bestT * axisLength;  // For rendering

                                    // Store starting offsets as FLOATS for smooth interpolation
                                    thumb.startOffsetF[0] = vis_dx;  // or vis_offset_x if using floats
                                    thumb.startOffsetF[1] = vis_dy;
                                    thumb.startOffsetF[2] = vis_dz;

                                    thumb.startOffset[0] = vis_dx;
                                    thumb.startOffset[1] = vis_dy;
                                    thumb.startOffset[2] = vis_dz;

                                    return;
                                }                            }
                        }

                        // Normal orbit only if nothing was picked
                        if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3]))
                        {
                            setLeftClicked(true);
                            setClickPoint(xpos, ypos);
                        }
                        // === Normal 3D orbit controller (only if no axis was hit) ===
                        if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3]))
                        {
                            setLeftClicked(true);
                            setClickPoint(xpos, ypos);
                        }
                        // Activate 3D interactor
                        if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3])) {
                            setLeftClicked(true);
                            setClickPoint(xpos, ypos);
                        }

                        // About dialog close button
                        if (showAboutDialog)
                        {
                            int closeX = gViewport[2] / 2 - 40;
                            int closeY = gViewport[3] - 180;
                            int closeWidth = 100;
                            int closeHeight = 30;

                            if (xpos >= closeX && xpos <= closeX + closeWidth &&
                                ypos >= closeY - closeHeight && ypos <= closeY + 10)
                            {
                                showAboutDialog = false;
                                return;
                            }
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

                        if (!wasSliderDragging)
                            setLeftClicked(false);

                        thumb.active   = false;
                        thumb.dragging = false;
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

            default:
                break;
        }
    }

    void moveCallback(GLFWwindow* window, double xpos, double ypos)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Slider and general GUI handling
        hslider.onMouseDrag((int)xpos, (int)ypos);
        hslider.onMouseMove((int)xpos, (int)ypos);

        if (hslider.isDragging() && tomoEnable && tomoEnable->getState()) {
            float sliderValue = hslider.getValue();
            tomography::setSlicePosition(sliderValue);
            tomography::update();
        }

        if (gHelpLink) {
            int mx = (int)xpos;
            int my = (int)ypos;
            int flippedY = height - my;
            helpHover = gHelpLink->contains(mx, flippedY, height);
        }

        if (!hslider.isDragging() && !vslider.isDragging() &&
            isIn3DZone(xpos, ypos, width, height))
        {
            setClickPoint(xpos, ypos);
        }

        // === CORRECTED AXIS THUMB DRAGGING WITH FRACTIONAL PRECISION ===
        if ((pause || currentMode == REPLAY) && thumb.active && thumb.dragging)
        {
            if (!gAxisProjValid) return;

            const AxisProjection& ap = gAxisProj[thumb.axis];
            float mx = static_cast<float>(xpos);
            float my = static_cast<float>(ypos);

            // 1. Calculate screen space mouse delta since drag started
            float dMx = mx - (float)thumb.startMouseX;
            float dMy = my - (float)thumb.startMouseY;

            // 2. Calculate axis screen direction vector
            float dAx = ap.x1 - ap.x0;
            float dAy = ap.y1 - ap.y0;
            float fullProjLength = std::hypot(dAx, dAy);

            if (fullProjLength < 1e-6f) return;

            // 3. Project mouse movement onto axis direction (in screen space)
            float projDistance = (dMx * dAx + dMy * dAy) / fullProjLength;

            // 4. Convert screen distance to parametric delta (0 to 1)
            float tDelta = projDistance / fullProjLength;

            // 5. Calculate new parametric position
            float newT = thumb.initialPosition + tDelta;

            // 6. Clamp to [0, 1]
            newT = glm::clamp(newT, 0.0f, 1.0f);

            // 7. Calculate clamped delta
            float finalTDelta = newT - thumb.initialPosition;

            // 8. Update thumb.position for rendering (in axis length units)
            thumb.position = newT * axisLength;

            // 9. Convert parametric delta to FRACTIONAL cell delta
            // The axis represents only 0.75 of the grid (axisLength=0.375, gridExtent=0.5)
            // So the axis only spans 0.75 * EL cells, not the full EL
            float axisGridRatio = axisLength / 0.5f;  // = 0.375 / 0.5 = 0.75
            float cellDelta = finalTDelta * automaton::EL * axisGridRatio;

            // 10. Apply fractional cell offset directly
            // This gives smooth, continuous movement matching the centers
            float newOffset = (float)thumb.startOffset[thumb.axis] + cellDelta;
            int newVisOffset = static_cast<int>(std::round(newOffset));

            if (thumb.axis == 0) {
                vis_dx = newVisOffset;
            }
            else if (thumb.axis == 1) {
                vis_dy = newVisOffset;
            }
            else if (thumb.axis == 2) {
                vis_dz = newVisOffset;
            }
        }
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

            // In keyCallback() → ESC case (replace the old broken version)
            case GLFW_KEY_ESCAPE:
                if (action == GLFW_PRESS)
                {
            #ifdef _WIN32
                    // This is now safe because we removed the bad glfwPollEvents() after MessageBox
                    int result = MessageBoxW(
                        nullptr,
                        L"Do you really want to exit?",
                        L"Exit Confirmation",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SYSTEMMODAL
                    );

                    if (result == IDYES)
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
            #else
                    // On Linux/macOS: fall back to your custom dialog or just exit
                    framework::showExitDialog = true;
            #endif
                }
                break;

            case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                    setSpeed(5.f);
                    break;

                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    setSpeed(0.1f);
                    break;

                case GLFW_KEY_T:
                    // Toggle tomography (optional shortcut)
                    if (tomoEnable) {
                        tomoEnable->toggle();
                        if (tomoEnable->getState() && !tomoDirs.empty()) {
                            for (auto& r : tomoDirs) r.setSelected(false);
                            tomoDirs[0].setSelected(true);
                            tomography::update();
                        }
                    }
                    break;

                case GLFW_KEY_M:
                    sound(true);
                    break;

                    // In keyCallback() for GLFW_KEY_P:
                    case GLFW_KEY_P:
                        if (action == GLFW_PRESS) {
                            pause = !pause;
                            if (!pause && currentMode == SIMULATION) {
                                vis_dx = 0;
                                vis_dy = 0;
                                vis_dz = 0;
                            }
                            glfwPostEmptyEvent();
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
