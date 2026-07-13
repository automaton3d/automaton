# It from Bit — A Concrete Attempt

Hi all,

I'm developing a toy model of the universe based on the cellular automaton paradigm. The underlying research ideas are described in this paper:

**https://doi.org/10.5281/zenodo.3818302**

This repository contains my ongoing effort to implement a computationally small version of the proposed universal automaton. Although the implementation is intentionally compact, the underlying rules are independent of the model's memory size. The project is written in C++, and the core automaton implementation is located in the **model** directory.

---

## 🚀 Build and Run

### Requirements

* Windows
* MSVC (Visual Studio or Build Tools)
* `nmake`
* OpenGL-capable GPU
* (Optional) CUDA Toolkit

### Build

Open a **Developer Command Prompt for Visual Studio** and run:

```text
nmake
```

### Run

```text
bin\automaton.exe
```

---

## 📁 Project Structure

```text
src/
  ├── model/        # Core automaton logic
  ├── cuda/         # CUDA backend (optional)
  ├── include/      # Headers
  └── *.cpp         # Rendering, GUI, control

bin/                # Executable and assets
build/              # Intermediate files (ignored)
```

---

## 🧠 Features

- 3D cellular automaton
- OpenGL visualization
- Custom GUI system
- Tomographic slicing (XY, YZ, ZX)
- Deterministic and stochastic initial states
- Optional CUDA acceleration

---

## 🔬 Research Goals

- Compute the Poincaré Cycle for sizes L = 8, L = 16 and L = 32 (the physical universe is estimated at L = 2²⁶⁹)
- Investigate charge quantization for L = 32
- Plot the entropy cycle

---

## 👤 About

I'm an independent researcher interested in fundamental physics, cellular automata, and emergent computation.

ResearchGate:

https://www.researchgate.net/

---

# ❤️ Support This Project

If you find this research interesting and would like to help its development, you can support it in one of the following ways.

## ☕ Buy Me a Coffee

You can make a secure international donation through Buy Me a Coffee:

**https://buymeacoffee.com/afurtado?new=1**

Every contribution helps fund computing resources, software, and the time required to continue this research.

---

## 🇧🇷 PIX (Brazil)

[![PIX QR Code](pix.jpg)](pix.jpg)

**PIX key:** *(scan the QR code above)*

Any contribution, no matter how small, is greatly appreciated and directly supports the continued development of this project.

---

Last update: July 13, 2026
