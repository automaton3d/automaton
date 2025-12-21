#pragma once
#include <vector>
#include "simulation.h" // Certifique-se que este arquivo define a struct Cell

namespace automaton {
    // Declaração dentro do namespace para coincidir com a busca do bridge.obj
    void getLayerData(unsigned selectedW, std::vector<Cell>& host_layer_data);
}

#ifdef USE_CUDA
// Outras funções de controle da GPU
void initGPU(unsigned EL, unsigned W_USED); 
void setSimulationParameters(unsigned EL, unsigned W_USED, int scenario);
void initGPULatticeState();  // ← DESCOMENTADA!
void runSimulationSteps(int numSteps);
void cleanupGPU();

#endif