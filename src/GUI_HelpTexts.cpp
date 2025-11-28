/*
 * GUI_HelpTexts.cpp (modernized, legacy structure preserved)
 *
 * Defines static arrays of help text strings used by GUI overlays.
 */

#include "GUI.h"

namespace framework {

// UI help instructions (11 items)
const char* ui_help[11] = {
    "Mouse: Rotate camera",
    "Scroll: Zoom in/out",
    "WASD: Pan camera",
    "Space: Pause/Resume",
    "R: Toggle recording",
    "T: Toggle tomography",
    "H: Toggle help",
    "ESC: Exit",
    "1-9: Select layer",
    "F1: Toggle fullscreen",
    "Tab: Next scenario"
};

// Recording help instructions (4 items)
const char* record_help[4] = {
    "R: Start/Stop recording",
    "P: Save recording",
    "L: Load recording",
    "K: Clear recording"
};

} // namespace framework