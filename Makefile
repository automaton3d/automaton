# ================================================
# Automaton Makefile - Final Version
# ================================================

TARGET = automaton.exe
BUILD_DIR = build

# Object files
OBJ = glad.obj tinyfiledialogs.obj \
      button.obj callback.obj camera.obj core.obj cortina.obj draw_utils.obj \
      dropdown.obj globals.obj GUI.obj GUI_2D.obj GUI_3D.obj GUI_Init.obj \
      GUI_Panels.obj help.obj hud.obj input.obj logo.obj main.obj menubar.obj \
      progress.obj projection.obj projection_manager.obj radio.obj recorder.obj \
      replay.obj replay_progress.obj scene.obj shader.obj sound.obj splash.obj \
      stats.obj stb_impl.obj text_renderer.obj tickbox.obj tomography.obj \
      bridge.obj convolutes.obj initSim.obj interaction.obj simulation.obj \
      utils.obj debug.obj

CC = cl
VCPKG_ROOT = E:/vcpkg/installed/x64-windows

CFLAGS = /nologo /std:c++20 /O2 /EHsc /MD \
         /I"src\include" /I"src\include\zlib" /I"src" \
         /I"$(VCPKG_ROOT)/include"

LDFLAGS = /link \
          /LIBPATH:"lib" \
          /LIBPATH:"$(VCPKG_ROOT)/lib" \
          opengl32.lib glfw3dll.lib freetype.lib \
          brotlidec.lib brotlicommon.lib bz2.lib zlib.lib \
          user32.lib gdi32.lib shell32.lib kernel32.lib ole32.lib comdlg32.lib

# ================================================
all: $(BUILD_DIR)\$(TARGET) dlls

# Robust linking command
$(BUILD_DIR)\$(TARGET): $(OBJ)
    if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    $(CC) $(OBJ) $(LDFLAGS) /Fe:$(BUILD_DIR)\$(TARGET) /out:$(BUILD_DIR)\$(TARGET)

# ================================================
# Compilation Rules
# ================================================

button.obj: src\button.cpp
    $(CC) $(CFLAGS) /c src\button.cpp /Fo$@

callback.obj: src\callback.cpp
    $(CC) $(CFLAGS) /c src\callback.cpp /Fo$@

camera.obj: src\camera.cpp
    $(CC) $(CFLAGS) /c src\camera.cpp /Fo$@

core.obj: src\core.cpp
    $(CC) $(CFLAGS) /c src\core.cpp /Fo$@

cortina.obj: src\cortina.cpp
    $(CC) $(CFLAGS) /c src\cortina.cpp /Fo$@

draw_utils.obj: src\draw_utils.cpp
    $(CC) $(CFLAGS) /c src\draw_utils.cpp /Fo$@

dropdown.obj: src\dropdown.cpp
    $(CC) $(CFLAGS) /c src\dropdown.cpp /Fo$@

globals.obj: src\globals.cpp
    $(CC) $(CFLAGS) /c src\globals.cpp /Fo$@

GUI.obj: src\GUI.cpp
    $(CC) $(CFLAGS) /c src\GUI.cpp /Fo$@

GUI_2D.obj: src\GUI_2D.cpp
    $(CC) $(CFLAGS) /c src\GUI_2D.cpp /Fo$@

GUI_3D.obj: src\GUI_3D.cpp
    $(CC) $(CFLAGS) /c src\GUI_3D.cpp /Fo$@

GUI_Init.obj: src\GUI_Init.cpp
    $(CC) $(CFLAGS) /c src\GUI_Init.cpp /Fo$@

GUI_Panels.obj: src\GUI_Panels.cpp
    $(CC) $(CFLAGS) /c src\GUI_Panels.cpp /Fo$@

help.obj: src\help.cpp
    $(CC) $(CFLAGS) /c src\help.cpp /Fo$@

hud.obj: src\hud.cpp
    $(CC) $(CFLAGS) /c src\hud.cpp /Fo$@

input.obj: src\input.cpp
    $(CC) $(CFLAGS) /c src\input.cpp /Fo$@

logo.obj: src\logo.cpp
    $(CC) $(CFLAGS) /c src\logo.cpp /Fo$@

main.obj: src\main.cpp
    $(CC) $(CFLAGS) /c src\main.cpp /Fo$@

menubar.obj: src\menubar.cpp
    $(CC) $(CFLAGS) /c src\menubar.cpp /Fo$@

progress.obj: src\progress.cpp
    $(CC) $(CFLAGS) /c src\progress.cpp /Fo$@

projection.obj: src\projection.cpp
    $(CC) $(CFLAGS) /c src\projection.cpp /Fo$@

projection_manager.obj: src\projection_manager.cpp
    $(CC) $(CFLAGS) /c src\projection_manager.cpp /Fo$@

radio.obj: src\radio.cpp
    $(CC) $(CFLAGS) /c src\radio.cpp /Fo$@

recorder.obj: src\recorder.cpp
    $(CC) $(CFLAGS) /c src\recorder.cpp /Fo$@

replay.obj: src\replay.cpp
    $(CC) $(CFLAGS) /c src\replay.cpp /Fo$@

replay_progress.obj: src\replay_progress.cpp
    $(CC) $(CFLAGS) /c src\replay_progress.cpp /Fo$@

scene.obj: src\scene.cpp
    $(CC) $(CFLAGS) /c src\scene.cpp /Fo$@

shader.obj: src\shader.cpp
    $(CC) $(CFLAGS) /c src\shader.cpp /Fo$@

sound.obj: src\sound.cpp
    $(CC) $(CFLAGS) /c src\sound.cpp /Fo$@

splash.obj: src\splash.cpp
    $(CC) $(CFLAGS) /c src\splash.cpp /Fo$@

stats.obj: src\stats.cpp
    $(CC) $(CFLAGS) /c src\stats.cpp /Fo$@

stb_impl.obj: src\stb_impl.cpp
    $(CC) $(CFLAGS) /c src\stb_impl.cpp /Fo$@

text_renderer.obj: src\text_renderer.cpp
    $(CC) $(CFLAGS) /c src\text_renderer.cpp /Fo$@

tickbox.obj: src\tickbox.cpp
    $(CC) $(CFLAGS) /c src\tickbox.cpp /Fo$@

tomography.obj: src\tomography.cpp
    $(CC) $(CFLAGS) /c src\tomography.cpp /Fo$@

bridge.obj: src\model\bridge.cpp
    $(CC) $(CFLAGS) /c src\model\bridge.cpp /Fo$@

convolutes.obj: src\model\convolutes.cpp
    $(CC) $(CFLAGS) /c src\model\convolutes.cpp /Fo$@

initSim.obj: src\model\initSim.cpp
    $(CC) $(CFLAGS) /c src\model\initSim.cpp /Fo$@

interaction.obj: src\model\interaction.cpp
    $(CC) $(CFLAGS) /c src\model\interaction.cpp /Fo$@

simulation.obj: src\model\simulation.cpp
    $(CC) $(CFLAGS) /c src\model\simulation.cpp /Fo$@

utils.obj: src\model\utils.cpp
    $(CC) $(CFLAGS) /c src\model\utils.cpp /Fo$@

debug.obj: src\model\debug.cpp
    $(CC) $(CFLAGS) /c src\model\debug.cpp /Fo$@

glad.obj: glad\glad.c
    $(CC) $(CFLAGS) /c glad\glad.c /Fo$@

tinyfiledialogs.obj: src\tinyfiledialogs.c
    $(CC) $(CFLAGS) /c src\tinyfiledialogs.c /Fo$@

# ================================================
dlls:
    if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    copy "$(VCPKG_ROOT)\bin\glfw3.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\freetype.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\zlib1.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\bz2.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\brotlidec.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\brotlicommon.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\brotlienc.dll" $(BUILD_DIR) >nul 2>nul
    copy "$(VCPKG_ROOT)\bin\libpng16.dll" $(BUILD_DIR) >nul 2>nul

clean:
    -del /Q *.obj 2>nul
    if exist $(BUILD_DIR) (
        del /Q $(BUILD_DIR)\*.exe 2>nul
        del /Q $(BUILD_DIR)\*.dll 2>nul
        del /Q $(BUILD_DIR)\*.pdb 2>nul
        @echo Preservando pastas: fonts, shaders, etc.
    )
    
run: all
    cd $(BUILD_DIR) && $(TARGET)

rebuild: clean all

.PHONY: all clean dlls run rebuild