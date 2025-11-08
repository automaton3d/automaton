# Toy Universe Splash Screen Instructions

## Overview
The splash screen is the entry point for the **Toy Universe** simulation project — a cellular automaton-based toy model exploring fundamental physics concepts, as described in the manuscript *"It from bit: a concrete attempt"* (available in the project documentation).

It allows users to select key parameters for the simulation: **lattice size (EL)** and **number of layers (W_DIM)**.  
The interface is designed for simplicity, with dropdowns for parameter selection and buttons to launch the simulation or view statistics.

---

## UI Elements

| Element | Label / Description | Default / Range |
|----------|--------------------|-----------------|
| **Title** | `"It from bit: a concrete attempt"` — displayed at the top | — |
| **Logo** | Visual logo below the title | — |
| **Size Dropdown** | `"Size:"` — choose odd lattice size values | Default: **11**<br>Range: **3–31** (odd) |
| **Layers Dropdown** | `"Layers:"` — choose even number of layers | Default: **4**<br>Range: **2–364** (even) |
| **Use Standard Checkbox** | `"Use standard (max)"` — sets layers to max (364) when checked | Checked: **W_DIM = 364**<br>Unchecked: user selectable |
| **Simulation Button** | Launches the main CA simulation | — |
| **Statistics Button** | Runs statistics mode (implementation-specific) | — |
| **Help Link** | Opens online documentation | [automaton/help.html at master · automaton3d/automaton](https://github.com/automaton3d/automaton/blob/master/help.html) |

---

## Usage Instructions

1. **Select Lattice Size:**  
   Use the `"Size"` dropdown to choose an odd value between `3` and `31`.  
   Larger values increase computational complexity and memory usage.

2. **Select Number of Layers:**  
   - If `"Use standard (max)"` is checked → **W_DIM = 364**.  
   - If unchecked → choose an even number between `2` and `364`.  
   The dropdown shows up to 8 items at a time; scrolling or window resizing may be required for larger ranges.

3. **Launch Simulation:**  
   - Click **"Simulation"** to run the cellular automaton model.  
   - Click **"Statistics"** for data analysis mode.

4. **View Help:**  
   Click the **"Help"** link to open documentation in your browser.

---

## Memory Constraints

Each simulation runs on a **4D lattice** (`EL × EL × EL × W_DIM`), with each cell occupying approximately **64 bytes** (based on the `Cell` class fields such as `ch, pB, sB, a, x[4], d, phiB, t, f, c[3], k, s2B, kB, bB, hB, cB`).

| Parameter | Formula | Approx. Memory Usage | Notes |
|------------|----------|----------------------|--------|
| **Default (EL=11, W_DIM=4)** | `11³ × 4 × 64 bytes` | **≈ 340 KB** | Very light, suitable for all systems |
| **Maximum (EL=31, W_DIM=364)** | `31³ × 364 × 64 bytes` | **≈ 700 MB** | May exceed limits on low-RAM systems |

**Memory Formula:**
\[
\text{Memory} \approx EL^3 \times W\_DIM \times 64\ \text{bytes}
\]

---

## Standard Value for `maxLayers` (W_DIM)

The **standard maximum** number of layers follows:
\[
W\_DIM = 3 \times EL^2
\]

| Lattice Size (EL) | Standard W_DIM (3 × EL²) | Rounded (even) |
|--------------------|---------------------------|----------------|
| 9 | 243 | 244 |
| 11 | 363 | **364** |
| 13 | 507 | 508 |
| 15 | 675 | 676 |

Selecting values above this may degrade performance or cause memory errors.  
For large simulations, ensure **≥ 4 GB of free RAM**.

**Tip:**  
Start small (`EL=11`, `W_DIM=4`) to validate your setup, then scale gradually.

---

## Additional Notes

- The project is a **deterministic cellular automaton** using **classical logic and arithmetic**.  
- Implements a **3-torus topology** and **binary-string state representation** per point.  
- Constants like `EL` and `W_DIM` are defined in `simulation.h`.  
- For theoretical and implementation details, see:
  - `manuscript.pdf` (in project root)
  - [Online Help](https://github.com/automaton3d/automaton/blob/master/help.html)

---

## Document Information

| Field | Value |
|--------|--------|
| **Created on** | 🕓 *11:04 AM -03, Tuesday, October 14, 2025* |
| **Project** | Toy Universe — “It from bit: a concrete attempt” |
| **Repository** | [automaton3d/automaton](https://github.com/automaton3d/automaton) |
| **Author** | Project maintainers of Toy Universe |

---

If you encounter issues, consult the **manuscript** for theoretical context or review the **source code** for implementation details.

