#include "simulation.h"
#include "core_sphere.h"
#include <iostream>
#include <iomanip>

using namespace automaton;
using namespace std;

// Declaração da função de init
extern void InitBuffers();

int main() {
    cout << "=== Teste de Convolucao Esferica (convolute0) ===" << endl;

    // 1. Inicialização
    InitBuffers();
    if (lattice_curr.empty()) {
        cerr << "Erro crítico: Buffers não alocados." << endl;
        return 1;
    }
    cout << "Grid 64^3 alocado (W_USED=" << W_USED << ")." << endl;

    // 2. Configuração do Cenário
    // Criar duas frentes de onda artificiais próximas para forçar interseção
    int cx = CENTER;
    int cy = CENTER;
    int cz = CENTER;

    // Frente 1: Centro exato
    // Definimos t=5, d=4 (superfície ativa)
    size_t idx_center = ((size_t)cx * EL + cy) * EL + cz;
    lattice_curr[idx_center].t = 5;
    lattice_curr[idx_center].d = 4; 
    lattice_curr[idx_center].ch = 1; // Carga teste

    // Frente 2: Vizinho imediato (ex: x+1)
    // Também ativo
    size_t idx_neighbor = ((size_t)(cx+1) * EL + cy) * EL + cz;
    lattice_curr[idx_neighbor].t = 5;
    lattice_curr[idx_neighbor].d = 4;
    lattice_curr[idx_neighbor].ch = 2; // Carga diferente

    cout << "Cenário: Duas células ativas adjacentes no centro." << endl;
    cout << "  Centro (" << cx << "," << cy << "," << cz << "): t=" << lattice_curr[idx_center].t 
         << ", d=" << lattice_curr[idx_center].d << endl;
    cout << "  Vizinho (" << cx+1 << "," << cy << "," << cz << "): t=" << lattice_curr[idx_neighbor].t 
         << ", d=" << lattice_curr[idx_neighbor].d << endl;

    // 3. Executar Convolução
    cout << "\nExecutando sphere_convolution_step..." << endl;
    sphere_convolution_step();

    // 4. Verificar Resultados no Draft
    cout << "\nVerificando Draft:" << endl;
    
    Cell* pDraftCenter = get_sphere_cell(lattice_draft, cx, cy, cz);
    Cell* pDraftNeighbor = get_sphere_cell(lattice_draft, cx+1, cy, cz);

    bool center_detected = (pDraftCenter && pDraftCenter->kB);
    bool neighbor_detected = (pDraftNeighbor && pDraftNeighbor->kB);

    cout << "  Centro Draft kB: " << (center_detected ? "SIM (Interagiu!)" : "NÃO") << endl;
    cout << "  Vizinho Draft kB: " << (neighbor_detected ? "SIM (Interagiu!)" : "NÃO") << endl;

    if (center_detected || neighbor_detected) {
        cout << "\nSUCESSO: A convolução detectou a interseção das frentes de onda." << endl;
        return 0;
    } else {
        cout << "\nFALHA: Nenhuma interação detectada. Verifique a lógica de pareamento." << endl;
        return 1;
    }
}