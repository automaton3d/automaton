#pragma once

#include <cstdint>
#include <cmath>

// Define a macro para __host__ __device__ que só é ativada pelo NVCC
#ifndef __CUDACC__
#define HOST_DEVICE
#else
#define HOST_DEVICE __host__ __device__
#endif

// ===================================================================
// CA CONSTANTS & MACROS (From simulation.h)
// ===================================================================

// Simulation symbols (Directions)
#define NORTH       0
#define EAST        1
#define SOUTH       2
#define WEST        3
#define UP          4
#define DOWN        5
#define FORWARD     6
#define BACKWARD    7

// Macros (Used in CPU files like utils.cpp)
#define ZERO(v)     (!(v[0] | v[1] | v[2]))

// Charge masks
#define C0_MASK     0x01
#define C1_MASK     0x02
#define C2_MASK     0x04
#define Q_MASK      0x08
#define W0_MASK     0x10
#define W1_MASK     0x20
#define COLOR_MASK  (C0_MASK | C1_MASK | C2_MASK)
#define WEAK_MASK   (W0_MASK | W1_MASK)
#define CHARGE_MASK (W0_MASK | W1_MASK | C0_MASK | C1_MASK | C2_MASK | Q_MASK)

// ===================================================================
// DATA STRUCTURES (Used by Host and Device)
// ===================================================================

// Platform-independent color type (RGBA for visualization)
struct Color {
    uint8_t r, g, b, a;

    // Substitua __host__ __device__ por HOST_DEVICE
    HOST_DEVICE Color() : r(0), g(0), b(0), a(255) {}
    HOST_DEVICE Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
      : r(red), g(green), b(blue), a(alpha) {}
};

// The core Cell structure (Assumed structure based on usage)

// Adicione isto antes da struct
    #ifdef __CUDACC__
        #define ALIGN(x) __align__(x)
    #else
        #define ALIGN(x) alignas(x)
    #endif

struct ALIGN(16) Cell {
    // Spatial coordinates (x, y, z, w where x[3] is w)
    unsigned x[4];

    // State variables
    uint8_t ch;     // Charge state (uses CHARGE_MASK flags)
    uint8_t a;      // Affinity

    // FSM/Wavefront variables
    uint8_t d;      // Wavefront distance/depth
    uint8_t t;      // Wavefront source
    uint8_t f;      // Wavefront field
    uint8_t k;      // Local step/tick counter

    // Flag variables (bools are unsafe in CUDA structs, use uint8_t)
    uint8_t pB:1;   // Primary flag?
    uint8_t sB:1;   // Secondary flag?
    uint8_t phiB:1; // Harmonic/Weak flag?
    uint8_t cB:1;   // Consumption flag?
    uint8_t hB:1;   // Propagation status flag?
    uint8_t bB:1;   // Propagation status flag?
    uint8_t kB:1;   // Collapse flag?

    // Relocation target coordinates (c[0], c[1], c[2] are x, y, z target)
    unsigned c[3];

    // Helper: Access the color bits (used in utils.cpp::isColorNeutral)
    // Helper: Access the color bits
    HOST_DEVICE unsigned char COLOR() const { return ch & COLOR_MASK; }

    // Default constructor
    HOST_DEVICE Cell() : ch(0), a(0), d(0), t(0), f(0), k(0), pB(0), sB(0), phiB(0), cB(0), hB(0), bB(0), kB(0) {
        x[0]=x[1]=x[2]=x[3]=0; c[0]=c[1]=c[2]=0;
    }
};
