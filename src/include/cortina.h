// cortina.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <glad/glad.h>

// Forward declarations from globals.h
class TextRenderer;

class Cortina {
public:
    // Construtor
    Cortina(float x, float y, float width, float height,
             const std::vector<std::string>& options,
             int initialSelection = 0,
             std::function<void(int)> callback = nullptr);

    // Renderiza o Cortina (deve ser chamado no loop de renderização)
    void render(TextRenderer* renderer);

    // Manipulação de entrada (deve ser chamado no callback de clique do mouse)
    bool handleMouseClick(double xpos, double ypos);
    
    // Manipulação da roda do mouse (scroll)
    void handleMouseScroll(double xpos, double ypos, double yoffset);

    // Obtém o índice da opção selecionada
    int getSelectedIndex() const;
    void setSelectedIndex(int idx);
    
    // Obtém o texto da opção selecionada
    std::string getSelectedText() const; 

    // Métodos de controle de estado
    void open() { isOpen = true; }
    void close() { isOpen = false; }
    void toggle() { isOpen = !isOpen; }
    bool isExpanded() const { return isOpen; }

    void scroll(int direction);
    void updateHover(int mx, int my);

private:
    // Posição e dimensões (coordenadas de tela)
    float x, y, width, height;
    
    // Lista de opções
    std::vector<std::string> options;
    
    // Estado do Cortina
    bool isOpen = false;
    int selectedIndex;

    int scrollOffset = 0;
    const int maxVisibleItems = 6;
        

    // Variáveis de estado do mouse para hover (bottom-up)
    int mouseX = 0; 
    int mouseY = 0;
    
    // Callback a ser executado na seleção
    std::function<void(int)> selectionCallback;

    // Constantes de aparência
    const glm::vec3 background_color = glm::vec3(0.1f, 0.1f, 0.1f);
    const glm::vec3 border_color     = glm::vec3(0.5f, 0.5f, 0.5f);
    const glm::vec3 hover_color      = glm::vec3(0.3f, 0.3f, 0.3f);
    const glm::vec3 text_color       = glm::vec3(1.0f, 1.0f, 1.0f);
    const float padding_x            = 20.0f;
    const float padding_y            = 15.0f;
    const float border_thickness     = 1.0f;
    const float text_scale           = 0.35f;

    void drawRect(float rx, float ry, float rwidth, float rheight, const glm::vec3& color);
    
    bool isMouseOver(double mx, double my, float rx, float ry, float rwidth, float rheight);
    
public:
    void updateMousePosition(double mx, double my) { mouseX = mx; mouseY = my; }
};