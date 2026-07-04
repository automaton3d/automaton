#include <iostream>
#include <vector>
#include "simulation.h"
#include "core_sphere.h"

using namespace automaton;
using namespace std;

int main() {
    cout << "=== Teste de Integracao Sphere Kernel (C++) ===" << endl;

    // Inicialização mínima das variáveis globais necessárias
    EL = 64;
    RMAX = 30;
    W_USED = 1; // Simplificação para o teste
    
    // Aloca os vetores globais
    try {
        lattice_curr.resize(EL * EL * EL * W_USED);
        lattice_draft.resize(EL * EL * EL * W_USED);
        lattice_mirror.resize(EL * EL * EL * W_USED);
    } catch (...) {
        cerr << "Erro ao alocar memoria." << endl;
        return 1;
    }

    cout << "Grid " << EL << "^3 alocado com sucesso." << endl;

    // Configura um centro de teste
    unsigned int center[3] = { EL/2, EL/2, EL/2 };

    // 1. Teste de Escrita na Esfera
    cout << "\n1. Criando padrao na esfera..." << endl;
    int count = 0;
    for (int x = 0; x < (int)EL; ++x) {
        for (int y = 0; y < (int)EL; ++y) {
            for (int z = 0; z < (int)EL; ++z) {
                Cell* cell = get_sphere_cell(lattice_curr, x, y, z, center);
                if (cell) {
                    cell->ch = 1; // Marca como ativa
                    cell->t = 1;
                    count++;
                }
            }
        }
    }
    cout << "   Celulas ativas criadas: " << count << endl;

    // 2. Teste de Mapeamento Antipodal
    cout << "\n2. Testando mapeamento antipodal..." << endl;
    int tx = center[0] + RMAX + 2; // Ponto fora da esfera
    int ty = center[1];
    int tz = center[2];
    
    Cell* outside = get_sphere_cell(lattice_curr, tx, ty, tz, center);
    if (outside) {
        cout << "   Acesso fora da esfera foi mapeado para (" 
             << (outside - &lattice_curr[0]) % EL << ", ..., ...)" << endl;
        // Nota: O cálculo exato do índice requer desfazer a matemática do getCell
    } else {
        cout << "   Acesso fora da esfera retornado como nulo (fora do alcance antipodal)." << endl;
    }

    // 3. Simulação de Passo
    cout << "\n3. Executando passo de Convolution..." << endl;
    step_sphere_convolution();
    
    cout << "4. Executando Commit..." << endl;
    step_sphere_commit();

    cout << "\n=== Teste Concluido ===" << endl;

    return 0;
}