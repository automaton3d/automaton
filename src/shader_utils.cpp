#include "shader_utils.h"
#include <iostream>

// Vertex shader para texto
const char* textVertexShaderSrc = R"glsl(
    #version 460 core
    layout (location = 0) in vec4 vertex; // posição.xy, texcoord.xy

    out vec2 TexCoords;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)glsl";

// Fragment shader para texto
const char* textFragmentShaderSrc = R"glsl(
    #version 460 core
    in vec2 TexCoords;
    out vec4 FragColor;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main() {
        float alpha = texture(text, TexCoords).a;
        FragColor = vec4(textColor, alpha);
    }
)glsl";

namespace framework {

unsigned int compileTextShader() {
    GLint success;
    char infoLog[512];

    // Vertex shader
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &textVertexShaderSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, nullptr, infoLog);
        std::cerr << "Text vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Fragment shader
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &textFragmentShaderSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, nullptr, infoLog);
        std::cerr << "Text fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Link program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Text shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

unsigned int compileColorShader() {
    const char* vertexSrc = R"glsl(
        #version 460 core
        layout(location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )glsl";

    const char* fragmentSrc = R"glsl(
        #version 460 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0); // red
        }
    )glsl";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

unsigned int compileTextureShader() {
    const char* vertexSrc = R"glsl(
        #version 460 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;
        uniform mat4 uMVP;
        out vec2 vUV;
        void main() {
            vUV = aUV;
            gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
        }
    )glsl";

    const char* fragmentSrc = R"glsl(
        #version 460 core
        in vec2 vUV;
        uniform sampler2D uTexture;
        out vec4 FragColor;
        void main() {
            FragColor = texture(uTexture, vUV);
        }
    )glsl";

    GLint success;
    char infoLog[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrc, nullptr);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, nullptr, infoLog);
        std::cerr << "Texture vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, nullptr, infoLog);
        std::cerr << "Texture fragment shader compilation failed: " << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Texture shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

} // namespace framework
