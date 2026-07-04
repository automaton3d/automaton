#include "simulation.h"
#include "core_sphere.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace automaton;
using namespace std;

// Declaração da função de inicialização global
extern void InitBuffers();

int main() {
    cout << "=== Teste de Relocation Esferico (Diagnostico) ===" << endl;

    // 1. Inicialização
    InitBuffers();
    
    // Limpeza explícita
    for (auto& c : lattice_curr) c = Cell();
    for (auto& c : lattice_draft) c = Cell();

    cout << "RMAX: " << RMAX << " | Centro: " << CENTER << endl;
    cout << "Grid " << EL << "^3 alocado (W_USED=" << W_USED << ")." << endl;

    // 2. Configurar Cenário
    int cx = CENTER;
    int cy = CENTER;
    int cz = CENTER;
    
    // Célula de origem no centro
    size_t src_idx = ((size_t)cx * EL + cy) * EL + cz;
    lattice_curr[src_idx].ch = 0xCD;      // Valor marcador
    lattice_curr[src_idx].t = 10;
    lattice_curr[src_idx].c[0] = 5;       // Deslocamento X = +5
    lattice_curr[src_idx].c[1] = 0;
    lattice_curr[src_idx].c[2] = 0;

    cout << "Origem (Centro): (" << setw(3) << cx << ", " << setw(3) << cy << ", " << setw(3) << cz << ") ";
    cout << "| ch=" << hex << (int)lattice_curr[src_idx].ch << dec;
    cout << " | c=(" << lattice_curr[src_idx].c[0] << "," << lattice_curr[src_idx].c[1] << "," << lattice_curr[src_idx].c[2] << ")";
    cout << " | t=" << lattice_curr[src_idx].t << endl;

    // Destino esperado (sem antípoda ainda)
    int exp_x = cx + 5;
    int exp_y = cy;
    int exp_z = cz;
    cout << "Destino Esperado (Matemático): (" << setw(3) << exp_x << ", " << setw(3) << exp_y << ", " << setw(3) << exp_z << ")" << endl;

    // 3. Executar Relocation
    cout << "\nExecutando sphere_relocation_step..." << endl;
    sphere_relocation_step();

    // 4. Diagnóstico Completo
    cout << "\n--- Varredura de Diagnóstico ---" << endl;
    
    bool found = false;
    int count = 0;

    // Varre apenas a região relevante para não poluir o output
    int start_x = max(0, cx - 2);
    int end_x = min((int)EL - 1, cx + 10);
    
    for (int x = start_x; x <= end_x; x++) {
        for (int y = cy; y <= cy; y++) { // Apenas linha Y central
            for (int z = cz; z <= cz; z++) { // Apenas linha Z central
                size_t idx = ((size_t)x * EL + y) * EL + z;
                if (lattice_draft[idx].ch == 0xCD) {
                    cout << "ENCONTRADO em Draft: (" << setw(3) << x << ", " << setw(3) << y << ", " << setw(3) << z << ") ";
                    cout << "| ch=" << hex << (int)lattice_draft[idx].ch << dec;
                    cout << " | c=(" << lattice_draft[idx].c[0] << "," << lattice_draft[idx].c[1] << "," << lattice_draft[idx].c[2] << ")";
                    cout << " | kB=" << (lattice_draft[idx].kB ? "SIM" : "NAO") << endl;
                    found = true;
                    
                    if (x == exp_x && y == exp_y && z == exp_z) {
                        cout << ">> POSIÇÃO CORRETA! Sucesso." << endl;
                    } else {
                        cout << ">> POSIÇÃO INCORRETA! (Esperado: " << exp_x << ")" << endl;
                    }
                }
                count++;
            }
        }
    }

    // Verifica se ficou na origem (erro comum)
    if (lattice_draft[src_idx].ch == 0xCD) {
        cout << "ALERTA: Os dados permaneceram na origem no Draft!" << endl;
    } else {
        cout << "Origem no Draft está limpa (ch=0)." << endl;
    }

    if (!found) {
        cout << "FALHA CRÍTICA: Nenhum dado com ch=0xCD encontrado na região varrida do Draft." << endl;
        cout << "Verifique se sphere_relocation_step está escrevendo em lattice_draft." << endl;
    } else {
        cout << "\nTeste Concluído." << endl;
    }

    return 0;
}