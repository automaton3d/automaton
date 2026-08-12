#pragma once

#include <vector>
#include <atomic>

namespace sinc_overlay {

// Double-buffered radial profile of the emergent polarisation u(r)
// and geometric product (sB && active) per shell.
void update(unsigned selectedW);

// Read-front accessors for the HUD overlay.
const std::vector<float>& profile();
const std::vector<float>& andMask();
unsigned currentRadius();
bool ready();

} // namespace sinc_overlay
