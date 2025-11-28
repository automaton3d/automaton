/*
 * ca_callback.cpp
*/

#include <windows.h>
#include "ca_window.h"
#include "GUI.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "hslider.h"
#include "vslider.h"
#include "radio.h"
#include "tickbox.h"
#include "projection.h"
#include "text_renderer.h"
#include "globals.h"
#include "menubar.h"

namespace framework
{
    extern std::unique_ptr<MenuBar> menuBar;

    void CAWindow::buttonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        if (showExitDialog) return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        switch (action)
        {
            case GLFW_PRESS:
                switch (button)
                {
                    case GLFW_MOUSE_BUTTON_LEFT:
                    {
                        // Handle menu bar clicks FIRST (only on press, at top of screen)
                        if (menuBar && ypos <= 30.0) {
                            menuBar->HandleMouse(xpos, ypos, gViewport[2], gViewport[3]);
                            return;  // Don't process other UI if clicking menu bar
                        }

                        #ifdef DEBUG
                        debugClickX = xpos;
                        debugClickY = ypos;
                        showDebugClick = true;
                        #endif
                        
                        // Sliders
                        vslider.onMouseButton(button, action, xpos, ypos, gViewport[3]);
                        hslider.onMouseButton(button, action, xpos, ypos, gViewport[3]);
                        if (hslider.isDragging() || vslider.isDragging()) return;

                        int mouseX = static_cast<int>(xpos);
                        int mouseY = gViewport[3] - static_cast<int>(ypos); // inverter Y

                        for (Tickbox& cb : data3D)
                            if (cb.onMouseButton(mouseX, mouseY, true)) return;

                        for (Tickbox& cb : delays)
                            if (cb.onMouseButton(mouseX, mouseY, true)) {
                                instance().onDelayToggled(&cb);
                                return;
                            }

                        scenarioHelpToggle->onMouseButton(mouseX, mouseY, true);

                        // View radios
                        for (size_t i = 0; i < views.size(); ++i)
                        {
                            if (views[i].clicked(xpos, ypos, gViewport[3]))
                            {
                                for (Radio& r : views) r.setSelected(false);
                                views[i].setSelected(true);

                                glm::vec3 eye    = instance().mCamera_.getEye();
                                glm::vec3 center = instance().mCamera_.getCenter();
                                float length     = glm::length(eye - center);

                                glm::vec3 dir;
                                if (i == 0) { dir = glm::normalize(glm::vec3(1,1,1)); instance().mCamera_.setUp(glm::vec3(0,1,0)); }
                                else if (i == 1) { dir = glm::normalize(glm::vec3(0,0,1)); instance().mCamera_.setUp(glm::vec3(1,0,0)); }
                                else if (i == 2) { dir = glm::normalize(glm::vec3(1,0,0)); instance().mCamera_.setUp(glm::vec3(0,1,0)); }
                                else if (i == 3) { dir = glm::normalize(glm::vec3(0,1,0)); instance().mCamera_.setUp(glm::vec3(1,0,0)); }

                                instance().mCamera_.setEye(center + dir * length);
                                instance().mCamera_.update();
                                instance().mInteractor_.setCamera(&instance().mCamera_);
                                return;
                            }
                        }

                        // Layer list
                        layerList->poll(xpos, ypos);

                        // Projection radios
                        for (size_t i = 0; i < projection.size(); ++i)
                        {
                            if (projection[i].clicked(xpos, ypos, gViewport[3]))
                            {
                                for (Radio& r : projection) r.setSelected(false);
                                projection[i].setSelected(true);
                                instance().updateProjection();
                                return;
                            }
                        }

                        // Tomography
                        if (tomo->onMouseButton((int)xpos, gViewport[3] - (int)ypos, true)) {
                            for (Radio& radio : tomoDirs) radio.setSelected(false);
                            if (tomo->getState()) tomoDirs[0].setSelected(true);
                        } else if (tomo->getState()) {
                            for (Radio& radio : tomoDirs)
                                if (radio.clicked(xpos, ypos, gViewport[3])) {
                                    for (Radio& r : tomoDirs) r.setSelected(false);
                                    radio.setSelected(true);
                                    break;
                                }
                        }

                        // Help hyperlink
                        std::string linkText = "Help";
                        int textWidth = textRenderer->measureTextWidth(linkText, 1.0f);
                        int linkX = (gViewport[2] - textWidth) / 2;
                        int linkY = gViewport[3] - 30;
                        int linkHeight = 15;

                        if (xpos >= linkX && xpos <= linkX + textWidth &&
                            ypos >= linkY - linkHeight && ypos <= linkY + 5)
                        {
                            // Platform-independent URL opening
                            #if defined(_WIN32)
                            std::system("start https://github.com/automaton3d/automaton/blob/master/help.md");
                            #elif defined(__APPLE__)
                            std::system("open https://github.com/automaton3d/automaton/blob/master/help.md");
                            #else
                            std::system("xdg-open https://github.com/automaton3d/automaton/blob/master/help.md");
                            #endif
                            return;
                        }

                        // Axis thumb detection
                        if ((pause || GUImode == REPLAY))
                        {
                            if (!gAxisProjValid) {
                                if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3])) {
                                    instance().mInteractor_.setLeftClicked(true);
                                    instance().mInteractor_.setClickPoint(xpos, ypos);
                                }
                                break;
                            }

                            float clickX = (float)xpos;
                            float clickY = (float)(gViewport[3] - ypos);

                            auto distAndT = [](float px, float py, const AxisProjection& ap, float &tOut) {
                                float dx = ap.x1 - ap.x0, dy = ap.y1 - ap.y0;
                                float denom = dx*dx + dy*dy;
                                if (denom <= 0.0f) { tOut = 0.0f; return std::hypot(px - ap.x0, py - ap.y0); }
                                float t = ((px - ap.x0)*dx + (py - ap.y0)*dy) / denom;
                                t = glm::clamp(t, 0.0f, 1.0f);
                                tOut = t;
                                float cx = ap.x0 + t*dx, cy = ap.y0 + t*dy;
                                return std::hypot(px - cx, py - cy);
                            };

                            float tX=0.0f, tY=0.0f, tZ=0.0f;
                            float dX = distAndT(clickX, clickY, gAxisProj[0], tX);
                            float dY = distAndT(clickX, clickY, gAxisProj[1], tY);
                            float dZ = distAndT(clickX, clickY, gAxisProj[2], tZ);

                            const float thresholdPx = 12.0f;
                            int chosenAxis = -1;
                            float t = 0.0f;
                            float minDist = thresholdPx;

                            if (dX < minDist) { minDist = dX; chosenAxis = 0; t = tX; }
                            if (dY < minDist) { minDist = dY; chosenAxis = 1; t = tY; }
                            if (dZ < minDist) { minDist = dZ; chosenAxis = 2; t = tZ; }

                            if (chosenAxis != -1) {
                                thumb.active   = true;
                                thumb.axis     = chosenAxis;
                                thumb.position = t * axisLength;
                                thumb.initialPosition = t * axisLength;
                                thumb.dragging = true;

                                thumb.startOffset[0] = vis_dx;
                                thumb.startOffset[1] = vis_dy;
                                thumb.startOffset[2] = vis_dz;

                                return;
                            }
                        }

                        // Activate 3D interactor
                        if (!thumb.active && isIn3DZone(xpos, ypos, gViewport[2], gViewport[3])) {
                            instance().mInteractor_.setLeftClicked(true);
                            instance().mInteractor_.setClickPoint(xpos, ypos);
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
                            instance().mInteractor_.setMiddleClicked(true);
                            instance().mInteractor_.setClickPoint(xpos, ypos);
                        }
                        break;

                    case GLFW_MOUSE_BUTTON_RIGHT:
                        if (isIn3DZone(xpos, ypos, gViewport[2], gViewport[3]) &&
                            !hslider.isDragging() && !vslider.isDragging())
                        {
                            instance().mInteractor_.setRightClicked(true);
                            instance().mInteractor_.setClickPoint(xpos, ypos);
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
                        hslider.onMouseButton(button, action, xpos, ypos, gViewport[3]);
                        vslider.onMouseButton(button, action, xpos, ypos, gViewport[3]);

                        if (!wasSliderDragging)
                            instance().mInteractor_.setLeftClicked(false);

                        thumb.active   = false;
                        thumb.dragging = false;
                        break;
                    }
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        instance().mInteractor_.setMiddleClicked(false);
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        instance().mInteractor_.setRightClicked(false);
                        break;
                }
                break;

            default:
                break;
        }
    }

    void CAWindow::moveCallback(GLFWwindow* window, double xpos, double ypos)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Handle slider dragging
        hslider.onMouseDrag((int)xpos, (int)ypos, height);
        vslider.onMouseDrag((int)xpos, (int)ypos, height);
        hslider.onMouseMove((int)xpos, ypos, height);

        // Hover detection for help hyperlink
        std::string linkText = "Help";
        int textWidth = textRenderer->measureTextWidth(linkText, 1.0f);
        int linkX = (width - textWidth) / 2;
        int linkY = height - 30;
        int linkHeight = 15;

        helpHover = (xpos >= linkX && xpos <= linkX + textWidth &&
                     ypos >= linkY - linkHeight && ypos <= linkY + 5);

        // Trackball update if in 3D zone
        if (!hslider.isDragging() && !vslider.isDragging() &&
            isIn3DZone(xpos, ypos, width, height))
        {
            instance().mInteractor_.setClickPoint(xpos, ypos);
        }

        // Axis thumb dragging
        if ((pause || GUImode == REPLAY) && thumb.active && thumb.dragging)
        {
            if (!gAxisProjValid) return;

            float x0 = gAxisProj[thumb.axis].x0;
            float y0 = gAxisProj[thumb.axis].y0;
            float x1 = gAxisProj[thumb.axis].x1;
            float y1 = gAxisProj[thumb.axis].y1;

            float mx = static_cast<float>(xpos);
            float my = static_cast<float>(height - ypos);

            auto relPos = [](float px, float py, float ax, float ay, float bx, float by) {
                float dx = bx - ax, dy = by - ay;
                float denom = dx*dx + dy*dy;
                if (denom <= 0.0f) return 0.0f;
                float t = ((px - ax) * dx + (py - ay) * dy) / denom;
                return glm::clamp(t, 0.0f, 1.0f);
            };

            float t = relPos(mx, my, x0, y0, x1, y1);
            thumb.position = t * axisLength;

            float deltaPosition = thumb.position - thumb.initialPosition;
            int deltaOffset = static_cast<int>(round((deltaPosition / axisLength) * automaton::EL));

            if (thumb.axis == 0) vis_dx = thumb.startOffset[0] + deltaOffset;
            if (thumb.axis == 1) vis_dy = thumb.startOffset[1] + deltaOffset;
            if (thumb.axis == 2) vis_dz = thumb.startOffset[2] + deltaOffset;
        }
    }

    void CAWindow::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Continue with existing scroll handling
        if (!hslider.isDragging() && !vslider.isDragging() &&
            isIn3DZone(mouseX, mouseY, gViewport[2], gViewport[3]))
        {
            instance().mInteractor_.setScrollDirection((xoffset + yoffset) > 0);
        }
    }

    void CAWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        // Debounce timer to prevent double-trigger from GLFW key repeat
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
                    int result = MessageBox(
                        nullptr,
                        "Do you really want to exit?",
                        "Exit Confirmation",
                        MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
                    );

                    if (result == IDYES) {
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                    #else
                    // Fallback for non-Windows platforms
                    framework::requestExit();
                    #endif
                    break;
                }

                case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                    instance().mInteractor_.setSpeed(5.f);
                    break;

                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    instance().mInteractor_.setSpeed(0.1f);
                    break;

                case GLFW_KEY_F2:
                    instance().mAnimator_.setAnimation(Animator::ORBIT);
                    break;

                case GLFW_KEY_F5:
                    if (GUImode == SIMULATION && !replayFrames) {
                        if (!recordFrames) {
                            recorder.recordingEnabled_ = true;
                        }
                        recordFrames = !recordFrames;
                    }
                    break;

                case GLFW_KEY_F6:
                    if (GUImode == REPLAY)
                    {
                        replayFrames = !replayFrames;
                        replayIndex = 0;
                        recordFrames = false;

                        if (framework::replayProgress)
                            framework::replayProgress->update(0, recorder.frames.size());
                    }
                    break;

                case GLFW_KEY_F7:
                    saveReplay();
                    break;

                case GLFW_KEY_F8:
                    loadReplay();
                    break;

                case GLFW_KEY_C:
                    std::cout << "\nCamera view:"
                              << " (" << instance().mCamera_.getEye().x
                              << "," << instance().mCamera_.getEye().y
                              << "," << instance().mCamera_.getEye().z << ") "
                              << "(" << instance().mCamera_.getCenter().x
                              << "," << instance().mCamera_.getCenter().y
                              << "," << instance().mCamera_.getCenter().z << ") "
                              << "(" << instance().mCamera_.getUp().x
                              << "," << instance().mCamera_.getUp().y
                              << "," << instance().mCamera_.getUp().z << ")\n\n";
                    break;

                case GLFW_KEY_R:
                    instance().mCamera_.reset();
                    instance().mInteractor_.setCamera(&instance().mCamera_);
                    break;

                case GLFW_KEY_T:
                    if (instance().mInteractor_.getMotionRightClick() == TrackBallInteractor::FIRSTPERSON)
                        instance().mInteractor_.setMotionRightClick(TrackBallInteractor::PAN);
                    else
                        instance().mInteractor_.setMotionRightClick(TrackBallInteractor::FIRSTPERSON);
                    break;

                case GLFW_KEY_X:
                {
                    glm::vec3 eye    = instance().mCamera_.getEye();
                    glm::vec3 center = instance().mCamera_.getCenter();
                    float length     = glm::length(eye - center);

                    glm::vec3 dir = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
                    instance().mCamera_.setEye(center + dir * length);
                    instance().mCamera_.setUp(glm::vec3(0, 1, 0));

                    instance().mCamera_.update();
                    instance().mInteractor_.setCamera(&instance().mCamera_);
                }
                break;

                case GLFW_KEY_Y:
                {
                    glm::vec3 eye    = instance().mCamera_.getEye();
                    glm::vec3 center = instance().mCamera_.getCenter();
                    float length     = glm::length(eye - center);

                    glm::vec3 dir = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
                    instance().mCamera_.setEye(center + dir * length);
                    instance().mCamera_.setUp(glm::vec3(1, 0, 0));

                    instance().mCamera_.update();
                    instance().mInteractor_.setCamera(&instance().mCamera_);
                }
                break;

                case GLFW_KEY_Z:
                {
                    glm::vec3 eye    = instance().mCamera_.getEye();
                    glm::vec3 center = instance().mCamera_.getCenter();
                    float length     = glm::length(eye - center);

                    glm::vec3 dir = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
                    instance().mCamera_.setEye(center + dir * length);
                    instance().mCamera_.setUp(glm::vec3(1, 0, 0));

                    instance().mCamera_.update();
                    instance().mInteractor_.setCamera(&instance().mCamera_);
                }
                break;

                case GLFW_KEY_O:  // Isometric
                {
                    glm::vec3 eye    = instance().mCamera_.getEye();
                    glm::vec3 center = instance().mCamera_.getCenter();
                    float length     = glm::length(eye - center);

                    glm::vec3 dir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
                    instance().mCamera_.setEye(center + dir * length);
                    instance().mCamera_.setUp(glm::vec3(0, 1, 0));

                    instance().mCamera_.update();
                    instance().mInteractor_.setCamera(&instance().mCamera_);
                }
                break;

                case GLFW_KEY_M:
                    sound(true);
                    break;

                case GLFW_KEY_P:
                    pause = !pause;
                    if (!pause && GUImode == SIMULATION) {
                        vis_dx = 0;
                        vis_dy = 0;
                        vis_dz = 0;
                    }
                    break;

                case GLFW_KEY_F1:
                    showHelp = !showHelp;
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
                    instance().mInteractor_.setSpeed(1.f);
                    break;

                default:
                    break;
            }
        }
    }

    void CAWindow::sizeCallback(GLFWwindow *window, int width, int height)
    {
        // TODO checar se gViewport precisa ser atualiada aqui!!!
        instance().mRenderer_.resize(width, height);
        instance().mInteractor_.setScreenSize(width, height);
        instance().mAnimator_.setScreenSize(width, height);

        if (framework::replayProgress)
            framework::replayProgress->setPosition(width, height, 100);
        if (menuBar) menuBar->Resize((float)width, (float)height);
    }

    void CAWindow::errorCallback(int error, const char* description)
    {
        std::cerr << "[GLFW Error " << error << "] " << description << std::endl;
    }
}