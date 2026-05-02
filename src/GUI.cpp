/*
 * GUI.cpp
 *
 * Implements the GUIrenderer orchestration logic.
 */

#include "GUI.h"
#include "globals.h"
#include "hslider.h"
#include "vslider.h"
#include "tickbox.h"
#include "replay_progress.h"
#include "progress.h"
#include "logo.h"
#include "text_renderer.h"
#include "dropdown.h"
#include "projection.h"
#include "radio.h"
#include "layers.h"
#include "model/simulation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <atomic>
#include "menubar.h"
#include "help.h"

namespace framework
{
    bool showAboutDialog = false;

    extern HSlider hslider;
    extern ReplayProgressBar* replayProgress;
    extern int windowWidth;
    extern int windowHeight;

    std::atomic<bool> replayFrames{false};

    void resize(int width, int height)
    {
        if (height == 0) height = 1;
        float ratio = static_cast<float>(width) / static_cast<float>(height);

        // Update OpenGL viewport
        glViewport(0, 0, width, height);

        // Update global viewport array
        gViewport[0] = 0;
        gViewport[1] = 0;
        gViewport[2] = width;
        gViewport[3] = height;

        // Update framework-specific dimensions
        windowWidth  = width;
        windowHeight = height;

        // Update projection matrices
        mOrtho       = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
        mPerspective = glm::perspective(glm::radians(45.0f), ratio, 0.01f, 100.0f);

        // Resize widgets
        if (progress)
            progress->resize(width, height);
        if (replayProgress)
            replayProgress->setPosition(width, height, height - 100);
        if (menuBar)
            menuBar->Resize(static_cast<float>(width), static_cast<float>(height));

        // HSlider has no resize() → use recenter() instead
        hslider.recenter(width);

        updateProjection();
    }

    void renderClear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
    }

    void requestExit()
    {
        // framework::CAWindow::instance().pendingExit = true;
    }
}
