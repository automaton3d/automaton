// Cortina.cpp
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "cortina.h"
#include "globals.h" 
#include "text_renderer.h"
#include <iostream>

// Assume que estas variáveis globais estão definidas em globals.cpp e declaradas em globals.h
extern GLFWwindow* window;
extern GLuint colorProgram2D;
extern GLint colorMvpLoc2D;
extern GLint colorColorLoc2D;
extern int gViewport[4]; // Viewport dimensions/position

// O MVP para 2D (Ortográfico) pode ser calculado globalmente ou aqui.
// Vamos calcular aqui para simplificar, usando as dimensões da janela.
glm::mat4 ortho_projection;

// ============================================================================
// Construtor
// ============================================================================

Cortina::Cortina(float x, float y, float width, float height,
                   const std::vector<std::string>& options,
                   int initialSelection,
                   std::function<void(int)> callback)
    : x(x), y(y), width(width), height(height),
      options(options), selectedIndex(initialSelection),
      selectionCallback(callback), isOpen(false)
{
    if (options.empty()) {
        this->options.push_back("No Options");
        selectedIndex = 0;
    }
    if (selectedIndex < 0 || selectedIndex >= options.size()) {
        selectedIndex = 0;
    }
}

void Cortina::setSelectedIndex(int idx)
{
    // Verifica se o índice está dentro dos limites da lista de opções
    if (idx >= 0 && idx < (int)options.size())
    {
        selectedIndex = idx;
        
        // NOVO: Ao definir um novo índice, resetamos o scroll para garantir 
        // que o item selecionado fique visível no topo da lista se ela for reaberta.
        // Se você tiver scroll implementado:
        // scrollOffset = 0; 

        // Se o componente estiver aberto e o novo índice estiver fora da
        // visão atual, ajuste o scroll.
        if (isOpen) {
            if (idx < scrollOffset) {
                scrollOffset = idx;
            } else if (idx >= scrollOffset + maxVisibleItems) {
                scrollOffset = idx - maxVisibleItems + 1;
            }
        }
        
    } else {
        // Opcional: imprimir um aviso se o índice estiver fora do limite
        // std::cerr << "Aviso: Tentativa de definir índice fora do limite na Cortina." << std::endl;
    }
}

void Cortina::drawRect(float rx, float ry, float rwidth, float rheight, const glm::vec3& color)
{
    // Define a projeção ortográfica baseada nas dimensões da janela
    // Assumimos que o sistema de coordenadas é (0,0) no canto superior esquerdo.
    // O eixo Y de FreeType/tela é invertido em relação ao GL.
    // O viewport é {x, y, width, height}
    ortho_projection = glm::ortho(0.0f, (float)gViewport[2], (float)gViewport[3], 0.0f);

    glUseProgram(colorProgram2D);
    glUniformMatrix4fv(colorMvpLoc2D, 1, GL_FALSE, glm::value_ptr(ortho_projection));
    glUniform3f(colorColorLoc2D, color.r, color.g, color.b);

    // Vértices do retângulo (em coordenadas de tela)
    GLfloat vertices[] = {
        // x     y
        rx,          ry + rheight, // Bottom-left
        rx,          ry,           // Top-left
        rx + rwidth, ry,           // Top-right
        
        rx,          ry + rheight, // Bottom-left
        rx + rwidth, ry,           // Top-right
        rx + rwidth, ry + rheight  // Bottom-right
    };

    // Usar um VAO/VBO temporário ou global para 2D (assumimos que o framework tem um)
    // Para simplificar, usamos um buffer temporário (não ideal, mas funcional)
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);

    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Posição: layout location 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Limpeza
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &rectVBO);
    glDeleteVertexArrays(1, &rectVAO);
    glUseProgram(0);
}

// ============================================================================
// isMouseOver Helper
// ============================================================================

bool Cortina::isMouseOver(double mx, double my, float rx, float ry, float rwidth, float rheight)
{
    // As coordenadas do dropdown (rx, ry) estão em bottom-up
    // Mas são renderizadas em top-down pela projeção
    // Então precisamos converter my (que vem em top-down) para bottom-up
    // OU converter rx, ry para top-down
    
    // Convertendo rx, ry de bottom-up para top-down:
    float ry_topdown = gViewport[3] - ry - rheight;
    
    return mx >= rx && mx < (rx + rwidth) &&
           my >= ry_topdown && my < (ry_topdown + rheight);
}

// cortina.cpp: Implementação de Cortina::render

void Cortina::render(TextRenderer* renderer)
{
    if (!renderer) return;

    float ascender = renderer->getAscenderPx();
    float descender = renderer->getDescenderPx();
    float centerline_offset = 0.5f * (ascender - descender) * text_scale;
    const float VISUAL_TWEAK_PX = 1.0f * text_scale; // Pequeno ajuste de 1px (comum)


    // 1. Desenha a borda e o fundo da caixa principal
    drawRect(x - border_thickness, y - border_thickness,
             width + 2 * border_thickness, height + 2 * border_thickness,
             border_color);
    drawRect(x, y, width, height, background_color);

    // ===========================================================================
    // Renderiza o texto do item principal
    // ===========================================================================

    float main_rect_center_y_bu = y + height / 2.0f; // y é o canto INFERIOR (bottom-up)
    float main_text_baseline_y_bu = main_rect_center_y_bu - centerline_offset + VISUAL_TWEAK_PX;
    float main_text_y_td = (float)gViewport[3] - main_text_baseline_y_bu;

    renderer->RenderText(options[selectedIndex], x + padding_x, main_text_y_td-padding_y, text_scale, text_color); 

    // ===========================================================================
    // 2. Desenha o Indicador de Abertura (Seta)
    // ===========================================================================
    float arrow_width = 10.0f;
    float arrow_height = 8.0f;
    float arrow_x = x + width - arrow_width - 5.0f; // 5px de margem
    float arrow_y = y + height / 2.0f - arrow_height / 2.0f; // Centraliza verticalmente

    if (isOpen) {
        // Seta para CIMA
        // Traço vertical central
        drawRect(arrow_x + arrow_width/2.0f - 1.0f, arrow_y + arrow_height * 0.25f, 2.0f, arrow_height * 0.5f, text_color);
        // Traço horizontal superior
        drawRect(arrow_x, arrow_y + arrow_height * 0.75f, arrow_width, 2.0f, text_color); 
    } else {
        // Seta para BAIXO
        // Traço vertical central
        drawRect(arrow_x + arrow_width/2.0f - 1.0f, arrow_y + arrow_height * 0.25f, 2.0f, arrow_height * 0.5f, text_color);
        // Traço horizontal inferior
        drawRect(arrow_x, arrow_y + arrow_height * 0.25f, arrow_width, 2.0f, text_color); 
    }

    // ===========================================================================
    // 3. Desenha a Lista de Opções (Com Scroll)
    // ===========================================================================
    if (isOpen)
    {
        float item_height = height;

        // Lógica de Scroll: Define o intervalo visível
        int start_index = scrollOffset;
        int end_index = std::min((int)options.size(), scrollOffset + maxVisibleItems);
        
        // Loop apenas sobre os itens visíveis
        for (int i = start_index; i < end_index; ++i)
        {
            int pos = i - start_index;
            float item_y = y + height + pos * item_height;

            glm::vec3 item_bg_color =
                (i == selectedIndex) ? hover_color : background_color;

            if (isMouseOver(mouseX, mouseY, x, item_y, width, item_height)) {
                item_bg_color = hover_color;
            }

            drawRect(x - border_thickness, item_y - border_thickness,
                     width + 2 * border_thickness,
                     item_height + 2 * border_thickness,
                    border_color);

            drawRect(x, item_y, width, item_height, item_bg_color);

            float item_rect_center_y_bu = item_y + item_height / 2.0f;
            float item_text_baseline_y_bu =
                item_rect_center_y_bu - centerline_offset + VISUAL_TWEAK_PX;
            float item_text_y_td =
                (float)gViewport[3] - item_text_baseline_y_bu;

            renderer->RenderText(
                options[i],
                x + padding_x,
                item_text_y_td - padding_y,
                text_scale,
                text_color
            );
        }
        
    // Opcional: Desenhar uma barra de rolagem (Scrollbar) se a lista for maior que maxVisibleItems
    // ...
    }
}

// Dentro de Cortina::handleMouseClick(double xpos, double ypos)
bool Cortina::handleMouseClick(double xpos, double ypos)
{
    bool handled = false;

    // 1. Clique na caixa principal (Toggle)
    if (isMouseOver(xpos, ypos, x, y, width, height)) {
        isOpen = !isOpen;
        handled = true;
    } 
    // 2. Se estiver aberto, verifica o clique na lista
    else if (isOpen) {
        float item_height = height;

        // NOVO: Define o intervalo visível (igual ao do render)
        int start_index = scrollOffset;
        int end_index = std::min((int)options.size(), scrollOffset + maxVisibleItems);
        
        for (int i = start_index; i < end_index; ++i) { 
            int pos = i - start_index; // Posição relativa na tela (0, 1, 2...)
            float item_y = y + height + pos * item_height; // Coordenada Y bottom-up do item
            
            if (isMouseOver(xpos, ypos, x, item_y, width, item_height)) {
                selectedIndex = i; 
                isOpen = false;
                handled = true;
   
                // Executa o callback se definido
                if (selectionCallback) {
                    selectionCallback(selectedIndex);
                }
                break;
            }
        }
        
        // 3. Se o clique foi fora da lista E fora da caixa principal, feche-o.
        if (!handled) {
            isOpen = false; 
        }
    }
    
    return handled;
}

// Implementação de Cortina::handleMouseScroll
void Cortina::handleMouseScroll(double xpos, double ypos, double yoffset)
{
    // A rolagem só é relevante se o Cortina estiver aberto
    if (!isOpen) {
        return;
    }

    // Calcula a área da lista suspensa visível
    int visible_count = std::min((int)options.size(), maxVisibleItems);
    float list_top = y + height;
    float list_height = height * visible_count;
    
    // Verifica se o mouse está sobre a área da lista suspensa
    // isMouseOver(mx, my, rect_x, rect_y, rect_w, rect_h)
    // O retângulo da lista começa em (x, list_top) e tem altura list_height
    if (isMouseOver(xpos, ypos, x, list_top, width, list_height)) {
        
        // yoffset > 0 significa rolagem para cima (rolar lista para cima)
        if (yoffset > 0) {
            // Rola para cima (diminui o offset)
            if (scrollOffset > 0) {
                scrollOffset--;
            }
        } 
        // yoffset < 0 significa rolagem para baixo (rolar lista para baixo)
        else if (yoffset < 0) {
            // Calcula o máximo de rolagem permitido
            int max_scroll = (int)options.size() - maxVisibleItems;
            if (max_scroll > 0 && scrollOffset < max_scroll) {
                scrollOffset++;
            }
        }
    }
    // Nota: O método scroll(int direction) já deve estar funcionando (e chama esta lógica)
    // Se você estiver chamando o método scroll, ele também deve funcionar.
}

void Cortina::scroll(int direction)
{
    if (!isOpen) return;

    // Rolagem para cima (direção > 0)
    if (direction > 0) { 
        if (scrollOffset > 0) {
            scrollOffset--;
        }
    } 
    // Rolagem para baixo (direção < 0)
    else if (direction < 0) { 
        // Calcula o máximo de rolagem permitido
        int max_scroll = (int)options.size() - maxVisibleItems;
        if (max_scroll > 0 && scrollOffset < max_scroll) {
            scrollOffset++;
        }
    }
}

void Cortina::updateHover(int mx, int my)
{
    // Simplesmente armazena as coordenadas do mouse (bottom-up),
    // que serão usadas pela função isMouseOver dentro do render.
    mouseX = mx;
    mouseY = my;
}

std::string Cortina::getSelectedText() const
{
    if (selectedIndex >= 0 && selectedIndex < options.size()) {
        return options[selectedIndex];
    }
    return "";
}

