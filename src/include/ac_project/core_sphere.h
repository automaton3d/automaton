#ifndef CORE_SPHERE_H
#define CORE_SPHERE_H

#include "simulation.h"
#include <vector>

namespace automaton {

    // Acessa célula com segurança e mapeamento antipodal se necessário
    // Retorna nullptr se estiver fora do domínio válido (fora da esfera e não mapeável)
    Cell* get_sphere_cell(std::vector<Cell>& lattice, int x, int y, int z);

    // Passos do Autômato (Sem multiplicação, sem tabelas, sem laços internos por célula)
    void sphere_convolution_step();
    void sphere_diffusion_step();
    void sphere_relocation_step();
    
    // Inicialização mínima (apenas centro ativo)
    void sphere_init_minimal();
}

#endif