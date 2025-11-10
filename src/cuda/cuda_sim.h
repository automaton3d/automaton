// cuda_sim.h  — put next to simulation.h
#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Simplified POD version of Cell for device transfer.
// Keep field names similar to CPU 'Cell' for easy mapping.
struct CellDevice {
    uint8_t  ch;       // charge bits
    uint8_t  pB;       // boolean -> 0/1
    uint8_t  sB;
    uint32_t a;
    uint32_t x0, x1, x2, x3; // positions
    uint32_t d;
    uint8_t  phiB;
    uint32_t t;
    uint32_t f;
    uint32_t c0, c1, c2;
    uint32_t k;
    uint8_t  s2B;
    uint8_t  kB;
    uint8_t  bB;
    uint8_t  hB;
    uint8_t  cB;
    // padding to keep 64-bit alignment (optional)
    uint8_t  _pad[3];
};

typedef unsigned int uint;

bool initCudaSimulation(unsigned EL, unsigned W_USED);
void cudaSimulationStep(); // compute one simulation step (device)
void cudaUpdateVoxelsLayer(unsigned layer); // writes into mapped host voxels[]
void cudaCleanup();

// Expose pointer to host-mapped voxel buffer (color format = 32-bit COLORREF / 0x00BBGGRR)
uint32_t* getMappedVoxels();
size_t getVoxelsSize(); // number of voxels (EL^3)
// Accessor for the current device lattice pointer
CellDevice* getDeviceCurrent();

#ifdef __cplusplus
}
#endif
