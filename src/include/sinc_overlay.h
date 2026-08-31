#pragma once

#include <vector>
#include <atomic>

namespace sinc_overlay {

// Update the self-contained radial sinc(r) / r*sin(r) overlay CA.
void update(unsigned selectedW);

// Read-front accessors for the HUD overlay.
const std::vector<float>& profile();
const std::vector<float>& triggerRate();
const std::vector<float>& peakHistory();
const std::vector<float>& andMask();
unsigned currentRadius();
unsigned graphSize();
bool ready();

} // namespace sinc_overlay
