/*
 * test_full_cycle.cpp
 * Teste de Ciclo Completo: Convolution -> Diffusion -> Relocation -> Commit
 */

#include "simulation.h"
#include "core_sphere.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace automaton;
using namespace std;

// Declaração da função de inicialização global
extern void InitBuffers();

void print_step_info(int step, int active_count, int collapsed_count, int moved_count) {
    cout << "Passo " << step << ": Ativas=" << active_count 
         << " | Colapsadas(kB)=" << collapsed_count 
         << " | Movidas(c!=0)=" << moved_count << endl;
}

int count_active(const vector<Cell>& lattice) {
    int count = 0;
    for (const auto& c : lattice) {
        if (c.t > 0 || c.ch != 0) count++;
    }
    return count;
}

int count_collapsed(const vector<Cell>& lattice) {
    int count = 0;
    for (const auto& c : lattice) {
        if (c.kB) count++;
    }
    return count;
}

int count_moving(const vector<Cell>& lattice) {
    int count = 0;
    for (const auto& c : lattice) {
        if (c.c[0] != 0 || c.c[1] != 0 || c.c[2] != 0) count++;
    }
    return count;
}

void print_slice_z(const vector<Cell>& lattice, int z_level, const char* label) {
    cout << "\n--- Slice Z=" << z_level << " (" << label << ") ---" << endl;
    int range = 5;
    int cx = CENTER;
    int cy = CENTER;
    
    // Imprime cabeçalho X
    cout << "   ";
    for (int x = cx - range; x <= cx + range; ++x) {
        cout << setw(3) << (x % 10);
    }
    cout << endl;

    for (int y = cy - range; y <= cy + range; ++y) {
        cout << setw(2) << (y % 10) << ":";
        for (int x = cx - range; x <= cx + range; ++x) {
            size_t idx = ((size_t)x * EL + y) * EL + z_level;
            const Cell& c = lattice[idx];
            
            char symbol = '.';
            if (c.kB) symbol = 'X'; // Colapsada
            else if (c.c[0] > 0 || c.c[1] > 0 || c.c[2] > 0) symbol = '>'; // Movendo
            else if (c.t > 0 && c.t == c.d) symbol = 'O'; // Frente de onda
            else if (c.t > 0) symbol = 'o'; // Interior
            else if (c.ch != 0) symbol = '#'; // Carga
            
            cout << setw(3) << symbol;
        }
        cout << endl;
    }
}

int main() {
    cout << "=== Teste de Ciclo Completo Esferico ===" << endl;

    // 1. Inicialização
    InitBuffers();
    
    cout << "Grid " << EL << "^3 alocado (W_USED=" << W_USED << ")." << endl;
    cout << "RMAX: " << RMAX << " | Centro: " << CENTER << endl;

    // 2. Configuração do Estado Inicial
    // Limpar buffers
    for (auto& c : lattice_curr) c = Cell();
    for (auto& c : lattice_draft) c = Cell();
    for (auto& c : lattice_mirror) c = Cell();

    int cx = CENTER;
    int cy = CENTER;
    int cz = CENTER;

    // Criar duas frentes de onda adjacentes para forçar interação na convolução
    // Célula A
    size_t idxA = ((size_t)cx * EL + cy) * EL + cz;
    lattice_curr[idxA].t = 5;
    lattice_curr[idxA].d = 4; // t != d (na superfície)
    lattice_curr[idxA].ch = 0x01;
    
    // Célula B (vizinha)
    size_t idxB = ((size_t)(cx+1) * EL + cy) * EL + cz;
    lattice_curr[idxB].t = 5;
    lattice_curr[idxB].d = 4;
    lattice_curr[idxB].ch = 0x02;

    // Adicionar uma partícula com vetor de movimento para testar Relocation
    size_t idxMove = ((size_t)(cx-5) * EL + cy) * EL + cz;
    lattice_curr[idxMove].t = 10;
    lattice_curr[idxMove].d = 10;
    lattice_curr[idxMove].ch = 0xAA;
    lattice_curr[idxMove].c[0] = 2; // Move +2 em X
    lattice_curr[idxMove].c[1] = 0;
    lattice_curr[idxMove].c[2] = 0;

    cout << "\nEstado Inicial Configurado:" << endl;
    cout << "  - 2 Frentes de onda em (" << cx << "," << cy << "," << cz << ") e vizinho" << endl;
    cout << "  - 1 Partícula móvel em (" << cx-5 << "," << cy << "," << cz << ") com c=(2,0,0)" << endl;

    print_slice_z(lattice_curr, cz, "Inicial (Curr)");

    // 3. Loop de Evolução
    int total_steps = 5;
    
    for (int step = 1; step <= total_steps; ++step) {
        cout << "\n>>> INICIO PASSO " << step << " <<<" << endl;

        // Fase 1: Convolução (Detecta interações na superfície)
        sphere_convolution_step();
        
        // Fase 2: Diffusion (Propaga kB, f, c)
        sphere_diffusion_step();

        // Fase 3: Relocation (Move partículas com c != 0)
        sphere_relocation_step();

        // Estatísticas do Draft antes do Commit
        int active = count_active(lattice_draft);
        int collapsed = count_collapsed(lattice_draft);
        int moving = count_moving(lattice_draft);
        
        print_step_info(step, active, collapsed, moving);

        // Visualização parcial no passo 1 e no final
        if (step == 1 || step == total_steps) {
            print_slice_z(lattice_draft, cz, "Draft (Pos-Fases)");
        }

        // Fase 4: Commit (Swap dos ponteiros ou cópia)
        // Como estamos usando vetores globais separados, faremos a cópia manual:
        // Draft -> Curr, Curr -> Mirror (ou lógica equivalente)
        // Para este teste, simplesmente copiamos Draft de volta para Curr para o próximo passo
        
        // Nota: Em produção, isso seria um swap de ponteiros se usássemos ponteiros,
        // mas com std::vector fixos, precisamos copiar ou gerenciar índices de buffer.
        // Aqui faremos uma cópia simplificada para demonstração.
        
        // Na implementação real com 3 buffers (Curr, Draft, Mirror):
        // O próximo passo lê de Curr. Então Curr deve receber o estado processado.
        // Vamos fazer: Curr = Draft.
        
        // Otimização: Em vez de copiar tudo, poderíamos usar índices de buffer circular.
        // Mas para este teste, copiaremos para garantir consistência.
        lattice_curr = lattice_draft; 
        
        // Limpa Draft para o próximo passo (boas práticas, embora a sobrescrita deva cuidar disso)
        // fill(lattice_draft.begin(), lattice_draft.end(), Cell()); 
        // Na verdade, as funções de processo devem escrever em todas as células relevantes ou limpar antes.
        // Assumindo que as funções escrevem apenas onde há atividade, limpamos o resto?
        // Para segurança neste teste, vamos zerar o draft antes da próxima iteração implicitamente
        // ao sobrescrever apenas as ativas? Não, melhor zerar.
        // Mas para performance, a lógica correta é: Draft começa zerado ou as funções fazem clear?
        // Vamos assumir que as funções de step escrevem o novo estado. 
        // Se houver células que deixam de existir, elas precisam ser zeradas no Draft.
        // Para simplificar este teste, faremos um clear explícito no início do loop ou após o commit.
        // Vamos zerar o Draft agora para o próximo passo começar limpo.
        for(auto& c : lattice_draft) {
             // Preservar informações persistentes se necessário, mas aqui resetamos tudo
             // Exceto talvez contadores globais se houvesse.
             // Reset seguro:
             c = Cell(); 
        }
        
        // Atualiza Mirror (opcional para este teste, mas parte do ciclo)
        lattice_mirror = lattice_curr;
    }

    cout << "\n=== Fim do Teste de Ciclo Completo ===" << endl;
    cout << "Verifique se as frentes de onda interagiram (kB ativado) e se a partícula se moveu." << endl;

    return 0;
}