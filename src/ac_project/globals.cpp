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