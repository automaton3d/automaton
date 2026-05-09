# ================================================
# Makefile for Automaton - CPU ou CUDA
# Use: nmake                     (CPU-only)
#      nmake USE_CUDA=1          (CUDA acelerado)
# ================================================

# ================================================
# Automaton Makefile - CPU or CUDA (with short path)
# ================================================

TARGET = automaton.exe
BUILD_DIR = build

!IFDEF USE_CUDA
ENABLE_CUDA = 1
!ELSE
ENABLE_CUDA = 0
!ENDIF

# Objetos comuns
OBJ_COMMON = glad.obj tinyfiledialogs.obj button.obj callback.obj camera.obj core.obj \
             cortina.obj draw_utils.obj dropdown.obj globals.obj GUI.obj GUI_2D.obj \
             GUI_3D.obj GUI_Init.obj GUI_Panels.obj help.obj hud.obj input.obj logo.obj \
             main.obj menubar.obj progress.obj projection.obj projection_manager.obj \
             radio.obj recorder.obj replay.obj replay_progress.obj scene.obj shader.obj \
             sound.obj splash.obj stats.obj stb_impl.obj text_renderer.obj tickbox.obj \
             tomography.obj convolutes.obj initSim.obj interaction.obj simulation.obj \
             utils.obj debug.obj config.obj render_pipeline.obj

!IF $(ENABLE_CUDA)
OBJ = $(OBJ_COMMON) bridge.obj bridge_cuda.obj cuda_automaton.obj
EXTRA_CPPFLAGS = /D "USE_CUDA" /D "CUDA_BRIDGE_CU"
EXTRA_NVCCFLAGS = -D "USE_CUDA"
!ELSE
OBJ = $(OBJ_COMMON) bridge.obj
EXTRA_CPPFLAGS =
EXTRA_NVCCFLAGS =
!ENDIF

CC = cl
NVCC = nvcc

VCPKG_ROOT = E:/vcpkg/installed/x64-windows
CUDA_PATH = C:\PROGRA~1\NVIDIA~2\CUDA\v13.2
CUDA_LIB = "$(CUDA_PATH)/lib/x64"

INCLUDES_MSVC = /I"src\include" /I"src\include\zlib" /I"src" /I"$(VCPKG_ROOT)/include"
INCLUDES_NVCC = -I"src\include" -I"src\include\zlib" -I"src" -I"$(VCPKG_ROOT)/include" -I"src\include\zlib\cuda" -I"src\cuda"

CFLAGS = /nologo /std:c++20 /O2 /EHsc /MD $(INCLUDES_MSVC) $(EXTRA_CPPFLAGS)

NVCC_FLAGS = -c -std=c++20 -O2 $(INCLUDES_NVCC) $(EXTRA_NVCCFLAGS) --compiler-options /MD

!IF $(ENABLE_CUDA)
LDFLAGS = /link /LIBPATH:"lib" /LIBPATH:"$(VCPKG_ROOT)/lib" /LIBPATH:$(CUDA_LIB) \
          opengl32.lib glfw3dll.lib freetype.lib brotlidec.lib brotlicommon.lib \
          bz2.lib zlib.lib user32.lib gdi32.lib shell32.lib kernel32.lib ole32.lib \
          comdlg32.lib cudart.lib
!ELSE
LDFLAGS = /link /LIBPATH:"lib" /LIBPATH:"$(VCPKG_ROOT)/lib" \
          opengl32.lib glfw3dll.lib freetype.lib brotlidec.lib brotlicommon.lib \
          bz2.lib zlib.lib user32.lib gdi32.lib shell32.lib kernel32.lib ole32.lib \
          comdlg32.lib
!ENDIF

all: $(BUILD_DIR)\$(TARGET) dlls copy_config

$(BUILD_DIR)\$(TARGET): $(OBJ)
    if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    $(CC) $(OBJ) $(LDFLAGS) /Fe:$(BUILD_DIR)\$(TARGET) /out:$(BUILD_DIR)\$(TARGET)

!IF $(ENABLE_CUDA)
bridge_cuda.obj: src\cuda\bridge_cuda.cu
    $(NVCC) $(NVCC_FLAGS) -o $@ $**

cuda_automaton.obj: src\cuda\cuda_automaton.cu
    $(NVCC) $(NVCC_FLAGS) -o $@ $**
!ENDIF

# Regras explícitas para objetos
button.obj: src\button.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

config.obj: src\config.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

callback.obj: src\callback.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

camera.obj: src\camera.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

core.obj: src\core.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

cortina.obj: src\cortina.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

draw_utils.obj: src\draw_utils.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

dropdown.obj: src\dropdown.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

globals.obj: src\globals.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

GUI.obj: src\GUI.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

GUI_2D.obj: src\GUI_2D.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

GUI_3D.obj: src\GUI_3D.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

GUI_Init.obj: src\GUI_Init.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

GUI_Panels.obj: src\GUI_Panels.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

help.obj: src\help.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

hud.obj: src\hud.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

input.obj: src\input.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

logo.obj: src\logo.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

main.obj: src\main.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

menubar.obj: src\menubar.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

progress.obj: src\progress.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

projection.obj: src\projection.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

projection_manager.obj: src\projection_manager.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

radio.obj: src\radio.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

recorder.obj: src\recorder.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

replay.obj: src\replay.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

replay_progress.obj: src\replay_progress.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

scene.obj: src\scene.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

shader.obj: src\shader.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

sound.obj: src\sound.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

splash.obj: src\splash.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

stats.obj: src\stats.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

render_pipeline.obj: src\render_pipeline.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

stb_impl.obj: src\stb_impl.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@
    
text_renderer.obj: src\text_renderer.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

tickbox.obj: src\tickbox.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

tomography.obj: src\tomography.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

convolutes.obj: src\model\convolutes.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

initSim.obj: src\model\initSim.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

interaction.obj: src\model\interaction.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

simulation.obj: src\model\simulation.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

utils.obj: src\model\utils.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

debug.obj: src\model\debug.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

glad.obj: glad\glad.c
    $(CC) $(CFLAGS) /c $** /Fo$@

tinyfiledialogs.obj: src\tinyfiledialogs.c
    $(CC) $(CFLAGS) /c $** /Fo$@

bridge.obj: src\model\bridge.cpp
    $(CC) $(CFLAGS) /c $** /Fo$@

dlls:
    if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\glfw3.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\freetype.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\zlib1.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\bz2.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\brotlidec.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\brotlicommon.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\brotlienc.dll" $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\libpng16.dll" $(BUILD_DIR)

clean:
    -del /Q *.obj 2>nul
    if exist $(BUILD_DIR) (
        del /Q $(BUILD_DIR)\*.exe 2>nul
        del /Q $(BUILD_DIR)\*.dll 2>nul
        del /Q $(BUILD_DIR)\*.pdb 2>nul
    )

copy_config:
    if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    if exist automaton.cfg copy automaton.cfg $(BUILD_DIR)\automaton.cfg
    
run: all
    cd $(BUILD_DIR) && $(TARGET)

rebuild: clean all

.PHONY: all clean dlls run rebuild
