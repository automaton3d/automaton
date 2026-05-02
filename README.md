# It from bit — a concrete attempt

Hi all,

I'm developing a toy model of the universe using the cellular automaton paradigm. The detailed research ideas are contained in this <A HREF="https://doi.org/10.5281/zenodo.3818302">pdf</A> file.

Here I present my efforts to implement a tiny version (in computational terms only because the rules are independent of the memory size of the model) of the universal automaton described in the aforementioned document. A program is being developed in the C++ programming language. The most important files are in the *model* directory, which contains the automaton algorithm.

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

```
nmake
```

### Run

```
bin\automaton.exe
```

---

## 📁 Project Structure

```
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

* 3D cellular automaton
* OpenGL visualization
* Custom GUI system
* Tomographic slicing (XY, YZ, ZX)
* Deterministic and stochastic initial states
* Optional CUDA acceleration

---

## 🔬 Research Goals

* Compute the Poincaré Cycle for sizes L=8, L=16 and L=32 (the universe is L=2^269)
* Check charge quantization for L=32
* Plot the entropy cycle

---

## 👤 About

I'm an independent researcher. My link to the ResearchGate portal is:

https://www.researchgate.net/

---

## 💚 Support this project via PIX

[![PIX QR Code](pix.jpg)](pix.jpg)
**PIX key:** *(scan the QR code above)*

Your contribution — of any amount — directly helps me continue developing and testing the universe model at higher computational scales.

---

Last update: 06/11/2025
