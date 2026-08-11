#include "simulation.h"
#include <iostream>
#include <vector>
#include <algorithm> // para fill

using namespace std;

// Definição das variáveis globais dentro do namespace automaton
namespace automaton {
    // Variáveis básicas (ajuste os valores conforme necessário para testes)
    unsigned EL = 64;
    unsigned W_USED = 3; 
    unsigned RMAX = 30;      // Raio da esfera
    unsigned CENTER = 32;    // Centro da grade (EL/2)
    
    // Buffers principais
    vector<Cell> lattice_curr;
    vector<Cell> lattice_draft;
    vector<Cell> lattice_mirror;

    // Implementação da função InitBuffers dentro do namespace
    void InitBuffers() {
        // Limpa buffers existentes
        lattice_curr.clear();
        lattice_draft.clear();
        lattice_mirror.clear();

        size_t totalSize = (size_t)EL * EL * EL * W_USED;
        cout << "Alocando " << totalSize << " celulas..." << endl;

        try {
            lattice_curr.resize(totalSize);
            lattice_draft.resize(totalSize);
            lattice_mirror.resize(totalSize);
            
            // Inicializa com células zeradas
            fill(lattice_curr.begin(), lattice_curr.end(), Cell());
            fill(lattice_draft.begin(), lattice_draft.end(), Cell());
            fill(lattice_mirror.begin(), lattice_mirror.end(), Cell());

            // Define as coordenadas x[] e r2/r iniciais a partir do índice linear
            for (size_t i = 0; i < totalSize; ++i) {
                unsigned int w = (unsigned int)(i % W_USED);
                size_t idx3d = i / W_USED;
                unsigned int z = (unsigned int)(idx3d % EL);
                unsigned int y = (unsigned int)((idx3d / EL) % EL);
                unsigned int x = (unsigned int)(idx3d / (EL * EL));

                int dx = (int)x - (int)CENTER;
                int dy = (int)y - (int)CENTER;
                int dz = (int)z - (int)CENTER;
                int r2_int = dx*dx + dy*dy + dz*dz;
                if (r2_int < 0) r2_int = 0;

                Cell* cells[3] = { &lattice_curr[i], &lattice_draft[i], &lattice_mirror[i] };
                for (int b = 0; b < 3; ++b) {
                    Cell& c = *cells[b];
                    c.x[0] = x;
                    c.x[1] = y;
                    c.x[2] = z;
                    c.x[3] = w;
                    c.r2 = (unsigned int)r2_int;
                    c.r  = isqrt(r2_int);
                }
            }

            cout << "Buffers alocados com sucesso." << endl;
            cout << "RMAX: " << RMAX << " | Centro: " << CENTER << endl;
        } catch (const bad_alloc& e) {
            cerr << "Erro de alocação: " << e.what() << endl;
            exit(1);
        }
    }
}

// Função ponte no escopo global para facilitar chamadas externas
void InitBuffers() {
    automaton::InitBuffers();
}