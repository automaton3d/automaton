cmake_minimum_required(VERSION 3.18)

# Set vcpkg toolchain BEFORE project()
set(CMAKE_TOOLCHAIN_FILE "E:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(simulation LANGUAGES CXX CUDA)

# ==============================================================
# Configurações gerais
# ==============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

# Arquitetura da GPU (75 = Turing; mude para 86/89/native se necessário)
set(CMAKE_CUDA_ARCHITECTURES 75)

# Resolve conflito de runtime MSVCRT (evita LNK4098)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# ==============================================================
# Encontrar pacotes vcpkg
# ==============================================================
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)

# ==============================================================
# Fontes do projeto - LISTADAS EXPLICITAMENTE
# ==============================================================

# Arquivos C++ (.cpp)
file(GLOB CPP_SOURCES
    "src/*.cpp"
    "src/model/*.cpp"
)

# Arquivos C (.c)
set(C_SOURCES
    "src/glad/glad.c"
    "src/tinyfiledialogs.c"
)

# Arquivos CUDA (.cu) - EXPLICITAMENTE LISTADOS
file(GLOB CUDA_SOURCES
    "src/automaton/*.cu"
)

# Verificar se os arquivos CUDA foram encontrados
message(STATUS "=== Arquivos CUDA encontrados ===")
foreach(cuda_file ${CUDA_SOURCES})
    message(STATUS "  ${cuda_file}")
endforeach()

# Combinar todas as fontes
set(ALL_SOURCES ${CPP_SOURCES} ${C_SOURCES} ${CUDA_SOURCES})

# Arquivos .c puros devem ser compilados como C
set_source_files_properties(${C_SOURCES} PROPERTIES LANGUAGE C)

# CRITICAL: Marcar explicitamente arquivos .cu como CUDA
set_source_files_properties(${CUDA_SOURCES} PROPERTIES LANGUAGE CUDA)

message(STATUS "=== Total de arquivos fonte: ${ALL_SOURCES} ===")

# ==============================================================
# Executável
# ==============================================================
add_executable(simulation ${ALL_SOURCES})

# ==============================================================
# Propriedades CUDA importantes
# ==============================================================
set_target_properties(simulation PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# ==============================================================
# Definições de compilação
# ==============================================================
target_compile_definitions(simulation PRIVATE USE_CUDA)

# ==============================================================
# Opções de compilação CUDA
# ==============================================================
target_compile_options(simulation PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:
        --expt-relaxed-constexpr
        --extended-lambda
        -Xcompiler=/W3
    >
)

# ==============================================================
# Diretórios de include
# ==============================================================
target_include_directories(simulation PRIVATE
    "src/include"
    "src/include/model"
    "src/include/zlib"
    "C:/GL/GLM"
    "C:/GL/GLAD/include"
    "C:/GL/GLFWx64/include"
)

# ==============================================================
# Diretórios de bibliotecas (apenas para libs não-vcpkg)
# ==============================================================
target_link_directories(simulation PRIVATE
    "C:/GL/GLFWx64/lib-vc2022"
)

# ==============================================================
# Link das bibliotecas
# ==============================================================
target_link_libraries(simulation PRIVATE
    glfw3
    opengl32
    gdi32
    user32
    shell32
    comdlg32
    comctl32
    ole32
    winmm
    cudart
    Freetype::Freetype      # vcpkg target (includes headers automatically)
    ZLIB::ZLIB              # vcpkg target
)

# ==============================================================
# Copy DLLs to output directory (Windows only)
# ==============================================================
if(WIN32)
    add_custom_command(TARGET simulation POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "E:/vcpkg/installed/x64-windows/bin/freetype.dll"
            $<TARGET_FILE_DIR:simulation>
        COMMENT "Copying freetype.dll to output directory"
    )
    
    add_custom_command(TARGET simulation POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "E:/vcpkg/installed/x64-windows/bin/zlib1.dll"
            $<TARGET_FILE_DIR:simulation>
        COMMENT "Copying zlib1.dll to output directory"
    )
endif()