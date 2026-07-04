/*
 * bridge_simple.cpp
 * Ponte minimalista para visualização do núcleo esférico (ac_project)
 * Restaurada compatibilidade com controle de mouse e câmera legados.
 */

#include "GUI.h"          // Interface gráfica legada (contém defs de voxels, etc)
#include "model/simulation.h" // Estrutura Cell e variáveis globais
#include "tomography.h"   // Para suporte a cortes tomográficos (opcional)
#include <cstdint>
#include <cmath>
#include <iostream>

// Acessa variáveis globais do namespace automaton
namespace automaton {
    extern unsigned EL;
    extern unsigned CENTER;
    extern unsigned RMAX;
    extern std::vector<Cell> lattice_curr;
    extern unsigned int pulse_tick;
    
    // Variáveis de controle de camada (para compatibilidade com UI)
    extern std::vector<std::array<unsigned, 3>> lcenters;
}

// Variáveis de tomografia (globais em tomography.cpp geralmente)
// Se não estiverem acessíveis, o código usará o fallback "true"
//extern bool tomoEnable; 
//extern int tomo_x, tomo_y, tomo_z;

using namespace automaton;

// ============================================================
// Helper de Visibilidade (Compatível com Legacy)
// ============================================================

bool isVisibleInTomogramSimple(unsigned x, unsigned y, unsigned z)
{
    // Se a tomografia estiver desativada, mostra tudo
    if (!tomoEnable || !tomoEnable) // Verificação dupla de segurança
        return true;

    // Lógica simplificada de cortes (XY, YZ, ZX)
    // Nota: Ajuste conforme a implementação real em tomography.cpp se necessário
    if (z != tomo_z && /* cheque se corte Z está ativo */ false) return false;
    if (x != tomo_x && /* cheque se corte X está ativo */ false) return false;
    if (y != tomo_y && /* cheque se corte Y está ativo */ false) return false;

    return true;
}

// ============================================================
// Implementação da Visualização Simplificada
// ============================================================

void updateBufferSimple()
{
    size_t idx = 0;
    unsigned int pulse_r2 = pulse_from_time(pulse_tick);
    
    // Pré-cálculo do raio atual para o marcador
    unsigned int current_r = 0;
    if (pulse_r2 > 0) {
        current_r = (unsigned int)sqrt((double)pulse_r2);
    }
    unsigned int markerX = CENTER + current_r;

    // Varredura sobre todo o volume cúbico
    // Otimização: O compilador deve desenrolar isso, mas garantimos que não há branches pesados internos
    for (unsigned x = 0; x < EL; ++x) {
        for (unsigned y = 0; y < EL; ++y) {
            for (unsigned z = 0; z < EL; ++z) {
                
                uint32_t color = 0x00000000u; // Transparente por padrão

                // 1. Verifica Tomografia/Cortes (Importante para performance e UX)
                if (!isVisibleInTomogramSimple(x, y, z)) {
                    voxels[idx++] = 0x00000000u;
                    continue;
                }

                // Acesso seguro à célula (Camada 0 por padrão para visualização simples)
                // Ajuste o índice se estiver usando W_USED > 1 explicitamente
                size_t linearIdx = ((size_t)x * EL + y) * EL + z; 
                const Cell& cell = lattice_curr[linearIdx]; 

                // ---------------------------------------------------------
                // Lógica de Cores para Diagnóstico Esférico (Refinada)
                // ---------------------------------------------------------

                // 1. Marcador de Raio Atual (Ponto Vermelho no eixo X) - Apenas para debug
                if (x == markerX && y == CENTER && z == CENTER) {
                    color = 0xFF5050FFu; // Vermelho brilhante
                }
                // 2. Centro da Esfera (Verde)
                else if (x == CENTER && y == CENTER && z == CENTER) {
                    color = 0x50FF50FFu; // Verde
                }
                // 3. Interação/Colapso (kB ativo) - Magenta (Alta Prioridade Visual)
                else if (cell.kB) {
                    color = 0xFF00FFFFu; // Magenta
                }
                // 4. Partícula em Movimento (Ciano) - Destaque para relocação
                else if (cell.c[0] != 0 || cell.c[1] != 0 || cell.c[2] != 0) {
                    color = 0x00FFFFFFu; // Ciano
                }
                // 5. Frente de Onda Pulsante (Amarelo) - Baseado em r2
                else if (cell.r2 != INF_R2 && cell.r2 == pulse_r2) {
                    color = 0xFFFF50FFu; // Amarelo
                }
                // 6. Interior da Esfera / Carga (Branco/Azulado)
                else if (cell.t > 0 || cell.ch != 0) {
                    // Intensidade baseada em t ou carga para dar profundidade
                    uint8_t intensity = (uint8_t)(std::min(255u, (unsigned)cell.t + cell.ch));
                    if (intensity < 50) intensity = 50; // Mínimo para ver algo
                    color = (intensity << 24) | (intensity << 16) | (intensity + 50 << 8) | 255; 
                    // Um tom azulado/branco suave
                }
                
                // 7. Borda da Esfera (Laranja Fraco) - Ajuda a visualizar o limite antipodal
                // Calcula distância de Manhattan simples (sem sqrt caro)
                int dx = (int)x - (int)CENTER;
                int dy = (int)y - (int)CENTER;
                int dz = (int)z - (int)CENTER;
                // Valor absoluto rápido
                int adx = dx < 0 ? -dx : dx;
                int ady = dy < 0 ? -dy : dy;
                int adz = dz < 0 ? -dz : dz;
                int dist_manhattan = adx + ady + adz;

                // Se estiver na casca externa e ainda não tiver cor
                if (color == 0x00000000u && cell.r2 != INF_R2) {
                     if (dist_manhattan >= RMAX - 1 && dist_manhattan <= RMAX + 1) {
                         color = 0x40FF8000u; // Laranja fraco (Alpha=64)
                     }
                }

                voxels[idx++] = color;
            }
        }
    }
}

// ============================================================
// Hook para o Loop Principal
// ============================================================

namespace automaton { //
    // Sobrescreve a função updateBuffer global usada pela GUI
    void updateBuffer() 
    {
        updateBufferSimple();
    }
}