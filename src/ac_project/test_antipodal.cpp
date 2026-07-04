#include "simulation.h"
#include "core_sphere.h"
#include <iostream>
#include <iomanip>
#include <cstring>

extern void InitBuffers();

using namespace automaton;
using namespace std;

// Função auxiliar para imprimir coordenadas e dados
void print_cell_info(const char* label, int x, int y, int z, const Cell& c) {
    cout << label << ": (" << setw(3) << x << ", " << setw(3) << y << ", " << setw(3) << z << ") "
         << "| ch=" << hex << (int)c.ch << dec 
         << " t=" << c.t 
         << " d=" << c.d 
         << endl;
}

int main() {
    cout << "=== Teste de Validação Antipodal ===" << endl;
    InitBuffers(); 
    // 1. Configuração Inicial
    if (lattice_curr.empty() || lattice_draft.empty()) {
        cerr << "Erro: Buffers não inicializados. Verifique globals.cpp." << endl;
        return 1;
    }

    // Limpar tudo para garantir estado conhecido
    for (auto& c : lattice_curr) c = Cell();
    for (auto& c : lattice_draft) c = Cell();

    cout << "Grid " << EL << "^3 alocado." << endl;
    cout << "Raio da esfera (RMAX): " << RMAX << endl;

    // 2. Plantação da Célula Teste
    int cx = CENTER; 
    int cy = CENTER;
    int cz = CENTER;
    
    // Ponto de origem: Na borda da esfera
    int src_x = cx + RMAX + 2; 
    int src_y = cy;
    int src_z = cz;
    
    if (src_x >= EL) src_x = EL - 1; // Segurança para não estourar o array

    unsigned char test_charge = 0xAB;
    unsigned int test_time = 999;
    unsigned int test_dist = 888;

    size_t linear_idx = ((size_t)src_x * EL + src_y) * EL + src_z;
    
    // Plantar a célula
    lattice_curr[linear_idx].ch = test_charge;
    lattice_curr[linear_idx].t = test_time;
    lattice_curr[linear_idx].d = test_dist;
    lattice_curr[linear_idx].kB = true; // Marcar como ativa

    print_cell_info("Origem Plantada", src_x, src_y, src_z, lattice_curr[linear_idx]);

    // 3. Executar Passo de Convolução Esférica
    sphere_convolution_step(); 

    // 4. Verificar o Destino Antipodal
    int dst_x = 2 * cx - src_x;
    int dst_y = 2 * cy - src_y;
    int dst_z = 2 * cz - src_z;

    // Ajuste para toro se necessário
    if (dst_x < 0) dst_x += EL;
    if (dst_x >= EL) dst_x -= EL;

    size_t dst_idx = ((size_t)dst_x * EL + dst_y) * EL + dst_z;
    const Cell& result_cell = lattice_draft[dst_idx];

    print_cell_info("Destino Esperado (Antípoda)", dst_x, dst_y, dst_z, result_cell);

    // 5. Validação
    bool success = (result_cell.ch == test_charge && 
                    result_cell.t == test_time && 
                    result_cell.d == test_dist);

    cout << "----------------------------------------" << endl;
    if (success) {
        cout << "SUCESSO: Dados teleportados corretamente via antípoda!" << endl;
    } else {
        cout << "FALHA: Dados não encontrados ou corrompidos no antípoda." << endl;
        cout << "Esperado: ch=" << hex << (int)test_charge << dec << " t=" << test_time << endl;
        cout << "Obtido:   ch=" << hex << (int)result_cell.ch << dec << " t=" << result_cell.t << endl;
        
        // Debug: Verificar se ficou no próprio lugar (falha na aplicação da regra)
        const Cell& self_result = lattice_draft[linear_idx];
        if (self_result.ch == test_charge) {
            cout << "Nota: Os dados permaneceram na origem. A regra antipodal pode não ter sido acionada." << endl;
        }
    }
    cout << "=== Fim do Teste ===" << endl;

    return success ? 0 : 1;
}