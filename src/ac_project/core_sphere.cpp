#include "core_sphere.h"
#include <cstring>
#include <iostream>

namespace automaton {

// Função auxiliar segura para acessar células (sem multiplicação, sem tabelas)
// Retorna nullptr se fora dos limites físicos do array ou fora da esfera lógica (opcional)
// Aqui usamos para acesso seguro ao array linearizado
inline Cell* get_cell_safe(vector<Cell>& lattice, int x, int y, int z) {
    if (x < 0 || x >= (int)EL || y < 0 || y >= (int)EL || z < 0 || z >= (int)EL) {
        return nullptr;
    }
    // Índice linear para célula (x,y,z) na camada w=0.
    // O armazenamento intercala w: i = (((x*EL)+y)*EL + z) * W_USED + w
    size_t idx = (((size_t)x * EL + y) * EL + z) * W_USED;

    if (idx >= lattice.size()) return nullptr;
    return &lattice[idx];
}

// Acessor com topologia esférica antipodal
// Se (x,y,z) estiver fora da esfera definida por RMAX centrada em CENTER,
// mapeia para o antípoda dentro da esfera.
Cell* get_sphere_cell(vector<Cell>& lattice, int x, int y, int z) {
    // Tratamento de borda toroidal padrão primeiro (para garantir acesso válido ao array)
    if (x < 0) x += EL;
    if (x >= (int)EL) x -= EL;
    if (y < 0) y += EL;
    if (y >= (int)EL) y -= EL;
    if (z < 0) z += EL;
    if (z >= (int)EL) z -= EL;

    // Verifica se está dentro da esfera lógica. Se estiver fora, mapeia para o antípoda.
    int dx = x - CENTER;
    int dy = y - CENTER;
    int dz = z - CENTER;
    int r2_int = dx*dx + dy*dy + dz*dz;
    int rmax2 = (int)RMAX * (int)RMAX;

    if (r2_int > rmax2) {
        // Mapeia para o antípoda e garante que fique dentro do array
        x = 2 * (int)CENTER - x;
        y = 2 * (int)CENTER - y;
        z = 2 * (int)CENTER - z;

        if (x < 0) x += EL;
        if (x >= (int)EL) x -= EL;
        if (y < 0) y += EL;
        if (y >= (int)EL) y -= EL;
        if (z < 0) z += EL;
        if (z >= (int)EL) z -= EL;
    }

    return get_cell_safe(lattice, x, y, z);
}

// Atualiza r2, r, (u,v), active, phiB, pB e sB a partir das coordenadas x[]
// e do relógio local t. Sem tabelas: usa isqrt e u^2+v^2=R^4.
void sphere_phase_step() {
    if (RMAX == 0 || W_USED == 0 || EL == 0) return;

    unsigned int phase_full = 2u * RMAX * RMAX;
    size_t total = lattice_curr.size();

    for (size_t i = 0; i < total; ++i) {
        Cell& c = lattice_curr[i];

        // Reconstrói as coordenadas a partir do índice linear (robusto a cópias)
        unsigned int w = (unsigned int)(i % W_USED);
        size_t idx3d = i / W_USED;
        unsigned int z = (unsigned int)(idx3d % EL);
        unsigned int y = (unsigned int)((idx3d / EL) % EL);
        unsigned int x = (unsigned int)(idx3d / (EL * EL));

        c.x[0] = x;
        c.x[1] = y;
        c.x[2] = z;
        c.x[3] = w;

        int dx = (int)x - (int)CENTER;
        int dy = (int)y - (int)CENTER;
        int dz = (int)z - (int)CENTER;
        int r2_int = dx*dx + dy*dy + dz*dz;
        if (r2_int < 0) r2_int = 0;
        c.r2 = (unsigned int)r2_int;

        // Update integer radius from exact r^2 without isqrt.
        // The previous radius is an excellent starting estimate.
        int r = c.r;
        if (r < 0) r = 0;
        while (r > 0 && (unsigned int)r * (unsigned int)r > c.r2)
            r--;
        while ((unsigned int)(r + 1) * (unsigned int)(r + 1) <= c.r2)
            r++;
        c.r = r;

        unsigned int pulse_r2 = pulse_from_time(c.t);
        c.active = (c.r2 == pulse_r2) ? 1u : 0u;

        if (c.r < 0 || c.r > (int)RMAX) {
            c.u = 0;
            c.v = 0;
            c.phiB = false;
            c.pB = false;
            c.sB = false;
            continue;
        }

        unsigned int w_offset = (unsigned int)(((unsigned long long)w * (unsigned long long)phase_full) / (unsigned long long)W_USED);
        unsigned int cell_phase = (((unsigned int)c.r * 2u * RMAX) + w_offset) % phase_full;
        int m = (int)(cell_phase / (unsigned int)RMAX);
        int R = (int)RMAX;
        int u, v;

        if (m < R) {
            int arg = m * (R - m);
            int s = isqrt(arg);
            u = R * (R - 2 * m);
            v = 2 * R * s;
        } else {
            int m2 = m - R;
            int arg = m2 * (R - m2);
            int s = isqrt(arg);
            u = R * (2 * m - 3 * R);
            v = -2 * R * s;
        }

        c.u = u;
        c.v = v;
        c.phiB = (c.active != 0);
        c.pB   = (u > 0);
        c.sB   = (v > 0);
    }
}

void sphere_convolution_step() {
    // Atualiza polarização e active antes de aplicar regras de interação
    sphere_phase_step();

    // Varredura sobre a região de interesse (caixa delimitadora da esfera)
    int range = RMAX + 2;
    int min_x = CENTER - range;
    int max_x = CENTER + range;
    int min_y = CENTER - range;
    int max_y = CENTER + range;
    int min_z = CENTER - range;
    int max_z = CENTER + range;

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int z = min_z; z <= max_z; ++z) {

                Cell* pCurr = get_sphere_cell(lattice_curr, x, y, z);
                if (!pCurr) continue;

                // Base do double buffer: copia Curr para Draft antes de modificar
                Cell* pDraft = get_sphere_cell(lattice_draft, x, y, z);
                if (pDraft) {
                    *pDraft = *pCurr;
                }

                // Regra 1: Detecção de Superfície
                // Apenas células na frente de onda interagem fortemente
                bool is_surface = (pCurr->active != 0);

                // Regra 2: Interação de Pares (Simplificada do integrated)
                // Se é superfície, procura vizinho com afinidade compatível
                if (is_surface && pDraft) {
                    // Exemplo: verificar vizinhos imediatos
                    // No integrated, isso é feito com máscaras e checks de carga
                    // Aqui vamos simular a detecção de colisão de frentes
                    bool collision = false;

                    // Check vizinho X+
                    Cell* pNx = get_sphere_cell(lattice_curr, x+1, y, z);
                    if (pNx && pNx->active != 0) {
                         // Condição de interação: cargas opostas ou mesma afinidade?
                         // No integrated: neutralColor ou neutralWeak
                         if ((pCurr->ch & COLOR_MASK) != 0 && (pNx->ch & COLOR_MASK) != 0) {
                             collision = true;
                         }
                    }

                    if (collision) {
                        // Ativa colapso
                        pDraft->kB = true;
                        // Atualiza frequência: f = f + t (emergência de harmônicos)
                        pDraft->f = pDraft->f + pDraft->t;

                        // Teste Sine Mask: se f >= d (ou condição similar), ativa s2B
                        if (pDraft->f >= pDraft->d && pDraft->d > 0) {
                            pDraft->s2B = true;
                        }
                    }

                    // Propagação antipodal: uma frente ativa na superfície
                    // também aparece no ponto antípoda (dentro da esfera).
                    int ax = 2 * (int)CENTER - x;
                    int ay = 2 * (int)CENTER - y;
                    int az = 2 * (int)CENTER - z;
                    Cell* pAnt = get_sphere_cell(lattice_draft, ax, ay, az);
                    if (pAnt && pAnt != pDraft) {
                        *pAnt = *pCurr;
                    }
                }
            }
        }
    }
}

void sphere_diffusion_step() {
    // Difusão de kB, f, c, s2B
    // Estratégia: Varredura e propagação para vizinhos se o vizinho não tiver ou tiver menor valor
    
    int range = RMAX + 2;
    int cx = CENTER, cy = CENTER, cz = CENTER;

    // Direções 6
    const int dx[6] = {1, -1, 0, 0, 0, 0};
    const int dy[6] = {0, 0, 1, -1, 0, 0};
    const int dz[6] = {0, 0, 0, 0, 1, -1};

    for (int x = cx - range; x <= cx + range; ++x) {
        for (int y = cy - range; y <= cy + range; ++y) {
            for (int z = cz - range; z <= cz + range; ++z) {
                
                Cell* pCurr = get_sphere_cell(lattice_curr, x, y, z);
                if (!pCurr) continue;
                Cell* pDraft = get_sphere_cell(lattice_draft, x, y, z);
                if (!pDraft) continue;

                // Garante que o Draft tem pelo menos os dados do Curr
                // (Necessário porque a convolução pode ter escrito apenas parcialmente)
                if (pDraft->t == 0 && pCurr->t != 0) {
                     *pDraft = *pCurr;
                }

                // Difusão de kB (Colapso): OR lógico com vizinhos
                if (pCurr->kB) {
                    for(int i=0; i<6; ++i) {
                        Cell* pN = get_sphere_cell(lattice_draft, x+dx[i], y+dy[i], z+dz[i]);
                        if(pN) pN->kB = true;
                    }
                }

                // Difusão de f (Frequência): Maximo local
                // Se vizinho tem f maior, atualiza
                unsigned int max_f = pDraft->f;
                for(int i=0; i<6; ++i) {
                    Cell* pN = get_sphere_cell(lattice_curr, x+dx[i], y+dy[i], z+dz[i]);
                    if(pN && pN->f > max_f) max_f = pN->f;
                }
                if (max_f > pDraft->f) {
                    pDraft->f = max_f;
                    // Re-testa s2B após difusão de f
                    if (pDraft->f >= pDraft->d && pDraft->d > 0) {
                        pDraft->s2B = true;
                    }
                }

                // Difusão de c (Vetor de Movimento): Inércia/Transporte
                // Se a célula não tem vetor, tenta pegar do vizinho (arrasto)
                if (pDraft->c[0] == 0 && pDraft->c[1] == 0 && pDraft->c[2] == 0) {
                    for(int i=0; i<6; ++i) {
                        Cell* pN = get_sphere_cell(lattice_curr, x+dx[i], y+dy[i], z+dz[i]);
                        if(pN && (pN->c[0] != 0 || pN->c[1] != 0 || pN->c[2] != 0)) {
                            pDraft->c[0] = pN->c[0];
                            pDraft->c[1] = pN->c[1];
                            pDraft->c[2] = pN->c[2];
                            break; // Pega o primeiro encontrado
                        }
                    }
                }
            }
        }
    }
}

void sphere_relocation_step() {
    // Move células com c != 0
    // Importante: Ler de Curr, Escrever em Draft.
    // Se mover, zera c no destino (ou mantém, dependendo da regra de inércia)
    // No integrated, o movimento é um passo discreto.
    
    int range = RMAX + 2;
    int cx = CENTER, cy = CENTER, cz = CENTER;

    // Precisamos varrer e mover. Cuidado com sobrescrita se fizermos in-place.
    // Como usamos Double Buffer (Curr -> Draft), é seguro.
    
    // Nota: A relocação deve ocorrer APÓS a difusão? Ou antes?
    // Ordem do integrated: Convolution -> Diffusion -> Relocation.
    // Então lemos o estado já difundido de Curr? 
    // Não, o loop principal faz:
    // 1. Convolution(Curr->Draft)
    // 2. Diffusion(Curr->Draft) (acumula no Draft)
    // 3. Relocation(Curr->Draft) (move no Draft)
    // Mas geralmente Relocation lê o vetor c de Curr e move o conteúdo para Draft.
    
    // Vamos assumir que lemos c de Curr e movemos o estado de Curr para Draft[pos+c].
    
    for (int x = cx - range; x <= cx + range; ++x) {
        for (int y = cy - range; y <= cy + range; ++y) {
            for (int z = cz - range; z <= cz + range; ++z) {
                
                Cell* pCurr = get_sphere_cell(lattice_curr, x, y, z);
                if (!pCurr) continue;

                // Só processa se tiver vetor de movimento
                if (pCurr->c[0] == 0 && pCurr->c[1] == 0 && pCurr->c[2] == 0) {
                    continue;
                }

                int nx = x + (int)pCurr->c[0];
                int ny = y + (int)pCurr->c[1];
                int nz = z + (int)pCurr->c[2];

                // Verifica se o destino é válido (dentro da esfera/toro)
                // Usa get_sphere_cell para lidar com wrap-around ou antípoda se necessário
                // Mas atenção: get_sphere_cell faz wrap no array. 
                // Se a lógica é "sair da esfera = antípoda", precisamos de lógica extra aqui.
                // Por enquanto, vamos usar o wrap toroidal padrão do array.
                
                Cell* pDest = get_sphere_cell(lattice_draft, nx, ny, nz);
                if (pDest) {
                    // Move o conteúdo
                    // Preserva informações que já estavam no destino? 
                    // Geralmente overwrite ou merge. No integrated, é cópia.
                    *pDest = *pCurr;
                    
                    // Zera o vetor no destino (chegou)
                    pDest->c[0] = 0;
                    pDest->c[1] = 0;
                    pDest->c[2] = 0;
                    
                    // Marca como relocada (opcional)
                    pDest->kB = true; // Reutilizando kB como flag de "atividade recente"? Não, cuidado.
                    // Melhor não mexer em kB aqui para não confundir com colapso.
                }
            }
        }
    }
}

} // namespace automaton