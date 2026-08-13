#pragma once

#include <vector>
#include <atomic>

namespace sinc_overlay {

// Double-buffered radial sinc(r) displacement profile and geometric
// product (wave velocity v > 0 && active) per shell, computed from
// the integer wave CA on layer w = 0.
void update(unsigned selectedW);

// Read-front accessors for the HUD overlay.
const std::vector<float>& profile();
const std::vector<float>& andMask();
unsigned currentRadius();
bool ready();

} // namespace sinc_overlay
