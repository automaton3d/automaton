#ifdef NOVO
/*
 * cuda_integration.cpp
 */

#include "cuda/cuda_sim.h"
#include "model/simulation.h" // your CPU Cell definition
#include <vector>
#include <cuda_runtime.h>

void packAndUploadCPUtoDevice(const std::vector<automaton::Cell>& cpuLattice,
                              unsigned EL, unsigned W_USED)
{
    size_t total = (size_t)EL * EL * EL * W_USED;
    std::vector<CellDevice> tmp(total);

    for (size_t i = 0; i < total; ++i) {
        const automaton::Cell& c = cpuLattice[i];
        CellDevice &d = tmp[i];
        d.ch = c.ch;
        d.pB = c.pB ? 1 : 0;
        d.sB = c.sB ? 1 : 0;
        d.a = c.a;
        d.x0 = c.x[0]; d.x1 = c.x[1]; d.x2 = c.x[2]; d.x3 = c.x[3];
        d.d = c.d;
        d.phiB = c.phiB ? 1 : 0;
        d.t = c.t;
        d.f = c.f;
        d.c0 = c.c[0]; d.c1 = c.c[1]; d.c2 = c.c[2];
        d.k = c.k;
        d.s2B = c.s2B ? 1 : 0;
        d.kB = c.kB ? 1 : 0;
        d.bB = c.bB ? 1 : 0;
        d.hB = c.hB ? 1 : 0;
        d.cB = c.cB ? 1 : 0;
    }

    cudaMemcpy(getDeviceCurrent(), tmp.data(),
               total * sizeof(CellDevice),
               cudaMemcpyHostToDevice);
}
#endif
