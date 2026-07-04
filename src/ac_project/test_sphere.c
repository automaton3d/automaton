#include <stdio.h>
#include "core_sphere.h"

int main() {
    printf("=== Teste de Topologia Esférica Antipodal ===\n\n");

    SphereSystem sys;
    init_sphere_system(&sys);

    // 1. Criar um padrão inicial na esfera Current
    printf("1. Inicializando padrão na esfera...\n");
    int count = 0;
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                if (is_inside_sphere(x, y, z, sys.center, sys.radius)) {
                    // Ativar apenas um octante para teste
                    if (x >= sys.center && y >= sys.center && z >= sys.center) {
                        sys.current->data[x][y][z].active = 1;
                        sys.current->data[x][y][z].affinity = 100;
                        count++;
                    }
                }
            }
        }
    }
    printf("   Celulas ativas criadas: %d\n\n", count);

    // 2. Testar Transformação Antipodal
    printf("2. Testando mapeamento antipodal...\n");
    int test_x = sys.center + 5;
    int test_y = sys.center + 5;
    int test_z = sys.center + 5;
    
    printf("   Original: (%d, %d, %d)\n", test_x, test_y, test_z);
    
    int ax = test_x, ay = test_y, az = test_z;
    apply_antipodal(&ax, &ay, &az, sys.center);
    printf("   Antipodal: (%d, %d, %d)\n", ax, ay, az);
    
    // Verificar simetria
    if (sys.current->data[test_x][test_y][test_z].active == 1) {
        printf("   Fonte ativa. Copiando valor para o destino antipodal no Draft...\n");
        Cell* src = get_cell_safe(sys.current, test_x, test_y, test_z);
        Cell* dst = get_cell_safe(sys.draft, ax, ay, az);
        
        if (dst) {
            dst->active = src->active;
            dst->affinity = src->affinity;
            printf("   Copia realizada com sucesso no Draft.\n");
        }
    }
    printf("\n");

    // 3. Simular Passo de Convolution (Simplificado)
    printf("3. Simulando passo de Convolution (Current -> Draft)...\n");
    // Aqui entraria a lógica complexa de interação
    // Por enquanto, apenas verificamos se o acesso seguro funciona nas bordas
    
    Cell* border = get_cell_safe(sys.current, -1, sys.center, sys.center);
    if (border == NULL) {
        printf("   Acesso fora dos limites (-1) corretamente bloqueado (NULL).\n");
    }

    border = get_cell_safe(sys.current, GRID_SIZE, sys.center, sys.center);
    if (border == NULL) {
        printf("   Acesso fora dos limites (GRID_SIZE) corretamente bloqueado (NULL).\n");
    }
    printf("\n");

    // 4. Commit (Draft -> Current)
    printf("4. Realizando Commit (Swap)...\n");
    swap_buffers(&sys);
    
    // Verificar se o dado antipodal agora está na 'current' (que era draft)
    Cell* check = get_cell_safe(sys.current, ax, ay, az);
    if (check && check->active == 1) {
        printf("   Sucesso! Dado antipodal preservado apos swap.\n");
    } else {
        printf("   Erro: Dado nao encontrado apos swap.\n");
    }

    printf("\n=== Todos os testes concluidos. ===\n");

    // Limpeza
    free(sys.current);
    free(sys.draft);
    free(sys.mirror);

    return 0;
}