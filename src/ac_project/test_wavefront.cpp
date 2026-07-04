#include "simulation.h"
#include "core_sphere.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Declaração da função de inicialização (definida em globals.cpp)
extern void InitBuffers();

using namespace automaton;

struct Point {
    int x, y, z;
};

// Função auxiliar para imprimir status
void print_status(const char* label, int x, int y, int z, const Cell& c) {
    std::cout << label << ": (" << std::setw(3) << x << ", " 
              << std::setw(3) << y << ", " << std::setw(3) << z << ") "
              << "| r2=" << c.r2 << " t=" << c.t << std::endl;
}

int main() {
    std::cout << "=== Teste de Propagação de Frente de Onda ===" << std::endl;

    // 1. Inicializar Buffers
    InitBuffers(); 

    if (lattice_curr.empty() || lattice_draft.empty()) {
        std::cerr << "Erro: Buffers não inicializados corretamente." << std::endl;
        return 1;
    }

    std::cout << "Grid " << EL << "^3 alocado." << std::endl;
    std::cout << "Raio da esfera (RMAX): " << RMAX << std::endl;

    // Limpar buffers
    for (auto& c : lattice_curr) c = Cell();
    for (auto& c : lattice_draft) c = Cell();

    // 2. Inicializar Frente de Onda no Centro
    int cx = CENTER;
    int cy = CENTER;
    int cz = CENTER;

    // Ativar centro com r2 = 0 e t = 1
    size_t centerIdx = ((size_t)cx * EL + cy) * EL + cz;
    lattice_curr[centerIdx].r2 = 0;
    lattice_curr[centerIdx].t = 1;
    lattice_curr[centerIdx].kB = true; // Marca como ativa para propagação

    print_status("Centro Inicial", cx, cy, cz, lattice_curr[centerIdx]);

    // 3. Simular Passos de Propagação (BFS Simplificado)
    // Vamos simular manualmente alguns passos de expansão para verificar a lógica
    int steps = 5;
    
    for (int step = 1; step <= steps; ++step) {
        // Copiar estado atual para draft (resetando r2 para infinito exceto onde propagar)
        for (auto& c : lattice_draft) {
            c.r2 = 0xFFFFFFFFu; 
            c.t = 0;
        }

        // Varredura simples para propagação (apenas vizinhos imediatos para demonstração)
        // Em um cenário real, isso usaria a lógica completa de vizinhança
        for (int x = 0; x < EL; ++x) {
            for (int y = 0; y < EL; ++y) {
                for (int z = 0; z < EL; ++z) {
                    size_t idx = ((size_t)x * EL + y) * EL + z;
                    Cell& curr = lattice_curr[idx];

                    // Se esta célula tem uma frente ativa (r2 válido e t > 0)
                    if (curr.r2 != 0xFFFFFFFFu && curr.t == step) {
                        // Propagar para vizinhos (6 direções)
                        int dx[] = {1, -1, 0, 0, 0, 0};
                        int dy[] = {0, 0, 1, -1, 0, 0};
                        int dz[] = {0, 0, 0, 0, 1, -1};

                        for (int i = 0; i < 6; ++i) {
                            int nx = x + dx[i];
                            int ny = y + dy[i];
                            int nz = z + dz[i];

                            // Obter célula vizinha com suporte antipodal
                            Cell* pNeighbor = get_sphere_cell(lattice_curr, nx, ny, nz);
                            
                            if (pNeighbor != nullptr) {
                                // Calcular índice linear do vizinho no buffer Draft
                                // Nota: get_sphere_cell retorna o ponteiro para o Current.
                                // Precisamos calcular o índice correspondente no Draft.
                                // Como a topologia é consistente, as coordenadas (nx, ny, nz) mapeiam para o mesmo índice linear
                                // após o ajuste antipodal interno se necessário.
                                // Mas para escrita segura no Draft, precisamos das coordenadas "reais" do array.
                                // A função get_sphere_cell lida com o wrap, mas para escrever no Draft no índice correto,
                                // assumimos que o índice linear calculado por (x,y,z) ajustados é o correto.
                                
                                // Simplificação: Vamos assumir que se get_sphere_cell retornou não-nulo,
                                // as coordenadas nx, ny, nz (já ajustadas internamente ou não) são válidas para indexação direta
                                // se estiverem dentro de [0, EL). Se houve wrap antipodal, as coordenadas retornadas pela função 
                                // (se ela as normalizasse) seriam usadas. 
                                // Como nossa função atual retorna o ponteiro, vamos recalcular o índice seguro.
                                
                                int safe_x = nx;
                                int safe_y = ny;
                                int safe_z = nz;

                                // Aplicar lógica de fronteira manual para garantir índice válido no Draft
                                if (safe_x < 0 || safe_x >= EL || safe_y < 0 || safe_y >= EL || safe_z < 0 || safe_z >= EL) {
                                    // Se saiu do cubo, aplica antípoda manual para achar o índice no Draft
                                    safe_x = (safe_x < 0 || safe_x >= EL) ? (2 * cx - nx) : nx;
                                    safe_y = (safe_y < 0 || safe_y >= EL) ? (2 * cy - ny) : ny;
                                    safe_z = (safe_z < 0 || safe_z >= EL) ? (2 * cz - nz) : nz;
                                    
                                    // Wrap final no toro se ainda fora (caso antípoda também esteja fora, o que não deve ocorrer se RMAX < L/2)
                                    if (safe_x < 0) safe_x += EL; if (safe_x >= EL) safe_x -= EL;
                                    if (safe_y < 0) safe_y += EL; if (safe_y >= EL) safe_y -= EL;
                                    if (safe_z < 0) safe_z += EL; if (safe_z >= EL) safe_z -= EL;
                                }

                                size_t nIdx = ((size_t)safe_x * EL + safe_y) * EL + safe_z;
                                Cell& neighborDraft = lattice_draft[nIdx];

                                // Regra de propagação: atualiza se o novo r2 for menor
                                unsigned int new_r2 = curr.r2 + 1; // Simplificação: distância Manhattan ou BFS passo 1
                                // Em BFS real, r2 seria a distância quadrada, aqui usamos passos para simplicidade do teste
                                
                                if (new_r2 < neighborDraft.r2) {
                                    neighborDraft.r2 = new_r2;
                                    neighborDraft.t = step + 1;
                                    neighborDraft.d = new_r2; // Usando d como proxy de distância
                                }
                            }
                        }
                    }
                }
            }
        }

        // Commit: Draft -> Curr (apenas para as células propagadas)
        for (size_t i = 0; i < lattice_curr.size(); ++i) {
            if (lattice_draft[i].t > lattice_curr[i].t) {
                lattice_curr[i].r2 = lattice_draft[i].r2;
                lattice_curr[i].t = lattice_draft[i].t;
            }
        }
        
        // Imprimir status de pontos chave a cada passo
        if (step == 1 || step == steps) {
             std::cout << "--- Passo " << step << " ---" << std::endl;
             print_status("Centro", cx, cy, cz, lattice_curr[centerIdx]);
             
             // Verificar ponto na borda (ex: cx + step)
             int bx = cx + step;
             if (bx >= EL) bx = EL - 1;
             size_t bIdx = ((size_t)bx * EL + cy) * EL + cz;
             print_status("Borda (aprox)", bx, cy, cz, lattice_curr[bIdx]);
        }
    }

    // 4. Verificação Final: A onda atingiu o antípoda?
    // Se iniciamos em Centro e demos 30 passos (RMAX), deveria chegar na borda e saltar.
    // Vamos verificar um ponto específico que sofreria antípoda se a onda fosse grande o suficiente.
    // Para este teste curto (5 passos), verificamos apenas a propagação local.
    
    std::cout << "=== Fim do Teste de Propagação ===" << std::endl;
    std::cout << "Verifique se os valores de 't' aumentaram nos vizinhos do centro." << std::endl;

    return 0;
}