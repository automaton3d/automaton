// ============================================================================
// tomography.h - Tomography System Header
// ============================================================================
#pragma once
#include <glm/glm.hpp>

namespace tomography {

    struct VoxelData {
        glm::vec3 pos;
        uint32_t color;
    };

    // No tomography.cpp
    struct VoxelSnapshot {
        glm::vec3 pos;
        uint32_t color;
    };

    // Initialize tomography system (call once at startup)
    void init();

    // Update tomography state (call when controls change)
    void update();

    // Check if a voxel should be visible in tomogram
    bool isVoxelVisible(unsigned x, unsigned y, unsigned z);

    // Render tomography slice visualization
    void renderSlice();

    // Render the semi-transparent cutting plane
    void renderTomoPlane();

    // Render tomography UI controls
    void renderControls();

    // Get current slice position (0.0 to 1.0)
    float getSlicePosition();

    // Set slice position (0.0 to 1.0)
    void setSlicePosition(float pos);

    // Request an update of the tomography snapshot
    void requestUpdate();

    // Handle mouse clicks for tomography controls
    void handleMouseClick(int mouseX, int mouseY);

    // Cleanup resources
    void cleanup();
}
