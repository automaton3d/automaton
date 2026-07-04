# ================================================
# Makefile for Automaton - CPU ou CUDA
# Use: nmake                     (CPU-only)
#      nmake USE_CUDA=1          (CUDA acelerado)
# ================================================

TARGET = automaton.exe
BUILD_DIR = build
OBJ_DIR = obj

!IFDEF USE_CUDA
ENABLE_CUDA = 1
!ELSE
ENABLE_CUDA = 0
!ENDIF

# ================================================
# Objetos comuns
# ================================================

OBJ_COMMON = \
	$(OBJ_DIR)\glad.obj \
	$(OBJ_DIR)\tinyfiledialogs.obj \
	$(OBJ_DIR)\button.obj \
	$(OBJ_DIR)\callback.obj \
	$(OBJ_DIR)\camera.obj \
	$(OBJ_DIR)\core.obj \
	$(OBJ_DIR)\cortina.obj \
	$(OBJ_DIR)\draw_utils.obj \
	$(OBJ_DIR)\dropdown.obj \
	$(OBJ_DIR)\globals.obj \
	$(OBJ_DIR)\GUI.obj \
	$(OBJ_DIR)\GUI_2D.obj \
	$(OBJ_DIR)\GUI_3D.obj \
	$(OBJ_DIR)\GUI_Init.obj \
	$(OBJ_DIR)\GUI_Panels.obj \
	$(OBJ_DIR)\help.obj \
	$(OBJ_DIR)\hud.obj \
	$(OBJ_DIR)\input.obj \
	$(OBJ_DIR)\logo.obj \
	$(OBJ_DIR)\main.obj \
	$(OBJ_DIR)\menubar.obj \
	$(OBJ_DIR)\progress.obj \
	$(OBJ_DIR)\projection.obj \
	$(OBJ_DIR)\projection_manager.obj \
	$(OBJ_DIR)\radio.obj \
	$(OBJ_DIR)\recorder.obj \
	$(OBJ_DIR)\replay.obj \
	$(OBJ_DIR)\replay_progress.obj \
	$(OBJ_DIR)\scene.obj \
	$(OBJ_DIR)\shader.obj \
	$(OBJ_DIR)\sound.obj \
	$(OBJ_DIR)\splash.obj \
	$(OBJ_DIR)\stats.obj \
	$(OBJ_DIR)\stb_impl.obj \
	$(OBJ_DIR)\text_renderer.obj \
	$(OBJ_DIR)\tickbox.obj \
	$(OBJ_DIR)\tomography.obj \
	$(OBJ_DIR)\convolutes.obj \
	$(OBJ_DIR)\initSim.obj \
	$(OBJ_DIR)\interaction.obj \
	$(OBJ_DIR)\simulation.obj \
	$(OBJ_DIR)\utils.obj \
	$(OBJ_DIR)\config.obj \
	$(OBJ_DIR)\render_pipeline.obj \
	$(OBJ_DIR)\ac_project\bridge_simple.obj \
	$(OBJ_DIR)\ac_project\core_sphere.obj \
	$(OBJ_DIR)\Renderer2D.obj \
	$(OBJ_DIR)\geometry.obj

!IF $(ENABLE_CUDA)

OBJ = $(OBJ_COMMON) \
	  $(OBJ_DIR)\bridge.obj \
	  $(OBJ_DIR)\bridge_cuda.obj \
	  $(OBJ_DIR)\cuda_constants.obj \
	  $(OBJ_DIR)\cuda_automaton.obj

EXTRA_CPPFLAGS = /D "USE_CUDA" /D "CUDA_BRIDGE_CU"
EXTRA_NVCCFLAGS = -D "USE_CUDA"

!ELSE

# Modo CPU: Usa apenas bridge_simple e core_sphere
OBJ = $(OBJ_COMMON)

EXTRA_CPPFLAGS =
EXTRA_NVCCFLAGS =

!ENDIF

# ================================================
# Compiladores
# ================================================

CC = cl
NVCC = nvcc

VCPKG_ROOT = E:/vcpkg/installed/x64-windows
CUDA_PATH = C:\PROGRA~1\NVIDIA~2\CUDA\v13.2
CUDA_LIB = "$(CUDA_PATH)/lib/x64"

INCLUDES_MSVC = \
	/I"src\include" \
	/I"src\include\zlib" \
	/I"src\include\ac_project" \
	/I"src" \
	/I"$(VCPKG_ROOT)/include"

INCLUDES_NVCC = \
	-I"src\include" \
	-I"src\include\zlib" \
	-I"src\include\ac_project" \
	-I"src" \
	-I"$(VCPKG_ROOT)/include" \
	-I"src\include\zlib\cuda" \
	-I"src\cuda"

CFLAGS = /nologo /std:c++20 /O2 /EHsc /MD $(INCLUDES_MSVC) $(EXTRA_CPPFLAGS)

NVCC_FLAGS = -c -std=c++20 -O2 $(INCLUDES_NVCC) $(EXTRA_NVCCFLAGS) --compiler-options /MD

!IF $(ENABLE_CUDA)

LDFLAGS = /link \
	/LIBPATH:"lib" \
	/LIBPATH:"$(VCPKG_ROOT)/lib" \
	/LIBPATH:$(CUDA_LIB) \
	opengl32.lib \
	glfw3dll.lib \
	freetype.lib \
	brotlidec.lib \
	brotlicommon.lib \
	bz2.lib \
	zlib.lib \
	user32.lib \
	gdi32.lib \
	shell32.lib \
	kernel32.lib \
	ole32.lib \
	comdlg32.lib \
	cudart.lib

!ELSE

LDFLAGS = /link \
	/LIBPATH:"lib" \
	/LIBPATH:"$(VCPKG_ROOT)/lib" \
	opengl32.lib \
	glfw3dll.lib \
	freetype.lib \
	brotlidec.lib \
	brotlicommon.lib \
	bz2.lib \
	zlib.lib \
	user32.lib \
	gdi32.lib \
	shell32.lib \
	kernel32.lib \
	ole32.lib \
	comdlg32.lib

!ENDIF

# ================================================
# Targets principais
# ================================================

all: dirs $(BUILD_DIR)\$(TARGET) dlls copy_config

dirs:
	if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
	if not exist "$(OBJ_DIR)\ac_project" mkdir "$(OBJ_DIR)\ac_project"

$(BUILD_DIR)\$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) /Fe:$(BUILD_DIR)\$(TARGET) /out:$(BUILD_DIR)\$(TARGET)

# ================================================
# CUDA
# ================================================

!IF $(ENABLE_CUDA)

$(OBJ_DIR)\bridge_cuda.obj: src\cuda\bridge_cuda.cu src\cuda\cuda_sim_optimized.h
	$(NVCC) $(NVCC_FLAGS) -o $@ $**

$(OBJ_DIR)\cuda_automaton.obj: src\cuda\cuda_automaton.cu src\cuda\cuda_sim_optimized.h
	$(NVCC) $(NVCC_FLAGS) -o $@ $**

$(OBJ_DIR)\cuda_constants.obj: src\cuda\cuda_constants.cu
	$(NVCC) $(NVCC_FLAGS) -o $@ $**

!ENDIF

# ================================================
# Regras explícitas (Com dependências de cabeçalho)
# ================================================

# --- GUI & Core ---
$(OBJ_DIR)\button.obj: src\button.cpp src\include\button.h
	$(CC) $(CFLAGS) /c src\button.cpp /Fo$(OBJ_DIR)\button.obj

$(OBJ_DIR)\config.obj: src\config.cpp src\include\config.h
	$(CC) $(CFLAGS) /c src\config.cpp /Fo$(OBJ_DIR)\config.obj

$(OBJ_DIR)\callback.obj: src\callback.cpp src\include\callbacks.h
	$(CC) $(CFLAGS) /c src\callback.cpp /Fo$(OBJ_DIR)\callback.obj

$(OBJ_DIR)\camera.obj: src\camera.cpp src\include\camera.h
	$(CC) $(CFLAGS) /c src\camera.cpp /Fo$(OBJ_DIR)\camera.obj

$(OBJ_DIR)\core.obj: src\core.cpp src\include\core.h
	$(CC) $(CFLAGS) /c src\core.cpp /Fo$(OBJ_DIR)\core.obj

$(OBJ_DIR)\cortina.obj: src\cortina.cpp src\include\cortina.h
	$(CC) $(CFLAGS) /c src\cortina.cpp /Fo$(OBJ_DIR)\cortina.obj

$(OBJ_DIR)\draw_utils.obj: src\draw_utils.cpp src\include\draw_utils.h
	$(CC) $(CFLAGS) /c src\draw_utils.cpp /Fo$(OBJ_DIR)\draw_utils.obj

$(OBJ_DIR)\dropdown.obj: src\dropdown.cpp src\include\dropdown.h
	$(CC) $(CFLAGS) /c src\dropdown.cpp /Fo$(OBJ_DIR)\dropdown.obj

$(OBJ_DIR)\globals.obj: src\globals.cpp src\include\globals.h
	$(CC) $(CFLAGS) /c src\globals.cpp /Fo$(OBJ_DIR)\globals.obj

$(OBJ_DIR)\GUI.obj: src\GUI.cpp src\include\GUI.h
	$(CC) $(CFLAGS) /c src\GUI.cpp /Fo$(OBJ_DIR)\GUI.obj

$(OBJ_DIR)\GUI_2D.obj: src\GUI_2D.cpp src\include\GUI.h
	$(CC) $(CFLAGS) /c src\GUI_2D.cpp /Fo$(OBJ_DIR)\GUI_2D.obj

$(OBJ_DIR)\GUI_3D.obj: src\GUI_3D.cpp src\include\GUI.h src\include\voxel.h
	$(CC) $(CFLAGS) /c src\GUI_3D.cpp /Fo$(OBJ_DIR)\GUI_3D.obj

$(OBJ_DIR)\GUI_Init.obj: src\GUI_Init.cpp src\include\GUI.h
	$(CC) $(CFLAGS) /c src\GUI_Init.cpp /Fo$(OBJ_DIR)\GUI_Init.obj

$(OBJ_DIR)\GUI_Panels.obj: src\GUI_Panels.cpp src\include\GUI.h
	$(CC) $(CFLAGS) /c src\GUI_Panels.cpp /Fo$(OBJ_DIR)\GUI_Panels.obj

$(OBJ_DIR)\help.obj: src\help.cpp src\include\help.h
	$(CC) $(CFLAGS) /c src\help.cpp /Fo$(OBJ_DIR)\help.obj

$(OBJ_DIR)\hud.obj: src\hud.cpp src\include\hud.h
	$(CC) $(CFLAGS) /c src\hud.cpp /Fo$(OBJ_DIR)\hud.obj

$(OBJ_DIR)\input.obj: src\input.cpp src\include\input.h
	$(CC) $(CFLAGS) /c src\input.cpp /Fo$(OBJ_DIR)\input.obj

$(OBJ_DIR)\logo.obj: src\logo.cpp src\include\logo.h
	$(CC) $(CFLAGS) /c src\logo.cpp /Fo$(OBJ_DIR)\logo.obj

$(OBJ_DIR)\main.obj: src\main.cpp src\include\GUI.h src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\main.cpp /Fo$(OBJ_DIR)\main.obj

$(OBJ_DIR)\menubar.obj: src\menubar.cpp src\include\menubar.h
	$(CC) $(CFLAGS) /c src\menubar.cpp /Fo$(OBJ_DIR)\menubar.obj

$(OBJ_DIR)\progress.obj: src\progress.cpp src\include\progress.h
	$(CC) $(CFLAGS) /c src\progress.cpp /Fo$(OBJ_DIR)\progress.obj

$(OBJ_DIR)\projection.obj: src\projection.cpp src\include\projection.h
	$(CC) $(CFLAGS) /c src\projection.cpp /Fo$(OBJ_DIR)\projection.obj

$(OBJ_DIR)\projection_manager.obj: src\projection_manager.cpp src\include\projection_manager.h
	$(CC) $(CFLAGS) /c src\projection_manager.cpp /Fo$(OBJ_DIR)\projection_manager.obj

$(OBJ_DIR)\radio.obj: src\radio.cpp src\include\radio.h
	$(CC) $(CFLAGS) /c src\radio.cpp /Fo$(OBJ_DIR)\radio.obj

$(OBJ_DIR)\recorder.obj: src\recorder.cpp src\include\recorder.h
	$(CC) $(CFLAGS) /c src\recorder.cpp /Fo$(OBJ_DIR)\recorder.obj

$(OBJ_DIR)\replay.obj: src\replay.cpp src\include\replay.h
	$(CC) $(CFLAGS) /c src\replay.cpp /Fo$(OBJ_DIR)\replay.obj

$(OBJ_DIR)\replay_progress.obj: src\replay_progress.cpp src\include\replay_progress.h
	$(CC) $(CFLAGS) /c src\replay_progress.cpp /Fo$(OBJ_DIR)\replay_progress.obj

$(OBJ_DIR)\scene.obj: src\scene.cpp src\include\scene.h
	$(CC) $(CFLAGS) /c src\scene.cpp /Fo$(OBJ_DIR)\scene.obj

$(OBJ_DIR)\shader.obj: src\shader.cpp src\include\shader.h
	$(CC) $(CFLAGS) /c src\shader.cpp /Fo$(OBJ_DIR)\shader.obj

$(OBJ_DIR)\sound.obj: src\sound.cpp
	$(CC) $(CFLAGS) /c src\sound.cpp /Fo$(OBJ_DIR)\sound.obj

$(OBJ_DIR)\splash.obj: src\splash.cpp src\include\splash.h
	$(CC) $(CFLAGS) /c src\splash.cpp /Fo$(OBJ_DIR)\splash.obj

$(OBJ_DIR)\stats.obj: src\stats.cpp src\include\stats.h
	$(CC) $(CFLAGS) /c src\stats.cpp /Fo$(OBJ_DIR)\stats.obj

$(OBJ_DIR)\render_pipeline.obj: src\render_pipeline.cpp src\include\render_pipeline.h
	$(CC) $(CFLAGS) /c src\render_pipeline.cpp /Fo$(OBJ_DIR)\render_pipeline.obj

$(OBJ_DIR)\stb_impl.obj: src\stb_impl.cpp
	$(CC) $(CFLAGS) /c src\stb_impl.cpp /Fo$(OBJ_DIR)\stb_impl.obj

$(OBJ_DIR)\text_renderer.obj: src\text_renderer.cpp src\include\text_renderer.h
	$(CC) $(CFLAGS) /c src\text_renderer.cpp /Fo$(OBJ_DIR)\text_renderer.obj

$(OBJ_DIR)\Renderer2D.obj: src\Renderer2D.cpp src\include\Renderer2D.h
	$(CC) $(CFLAGS) /c src\Renderer2D.cpp /Fo$(OBJ_DIR)\Renderer2D.obj

$(OBJ_DIR)\tickbox.obj: src\tickbox.cpp src\include\tickbox.h
	$(CC) $(CFLAGS) /c src\tickbox.cpp /Fo$(OBJ_DIR)\tickbox.obj

$(OBJ_DIR)\tomography.obj: src\tomography.cpp src\include\tomography.h
	$(CC) $(CFLAGS) /c src\tomography.cpp /Fo$(OBJ_DIR)\tomography.obj

# --- Model ---
$(OBJ_DIR)\convolutes.obj: src\model\convolutes.cpp src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\model\convolutes.cpp /Fo$(OBJ_DIR)\convolutes.obj

$(OBJ_DIR)\initSim.obj: src\model\initSim.cpp src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\model\initSim.cpp /Fo$(OBJ_DIR)\initSim.obj

$(OBJ_DIR)\interaction.obj: src\model\interaction.cpp src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\model\interaction.cpp /Fo$(OBJ_DIR)\interaction.obj

$(OBJ_DIR)\simulation.obj: src\model\simulation.cpp src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\model\simulation.cpp /Fo$(OBJ_DIR)\simulation.obj

$(OBJ_DIR)\utils.obj: src\model\utils.cpp src\include\model\simulation.h
	$(CC) $(CFLAGS) /c src\model\utils.cpp /Fo$(OBJ_DIR)\utils.obj

$(OBJ_DIR)\bridge.obj: src\model\bridge.cpp src\include\model\simulation.h src\include\voxel.h
	$(CC) $(CFLAGS) /c src\model\bridge.cpp /Fo$(OBJ_DIR)\bridge.obj

$(OBJ_DIR)\geometry.obj: src\model\geometry.cpp src\include\model\geometry.h
	$(CC) $(CFLAGS) /c src\model\geometry.cpp /Fo$(OBJ_DIR)\geometry.obj

# --- Libraries ---
$(OBJ_DIR)\glad.obj: glad\glad.c
	$(CC) $(CFLAGS) /c glad\glad.c /Fo$(OBJ_DIR)\glad.obj
$(OBJ_DIR)\tinyfiledialogs.obj: src\tinyfiledialogs.c src\include\tinyfiledialogs.h
	$(CC) $(CFLAGS) /c src\tinyfiledialogs.c /Fo$(OBJ_DIR)\tinyfiledialogs.obj

# --- AC Project (Novo Núcleo Esférico) ---
$(OBJ_DIR)\ac_project\bridge_simple.obj: src\ac_project\bridge_simple.cpp src\include\model\simulation.h src\include\ac_project\core_sphere.h
	$(CC) $(CFLAGS) /c src\ac_project\bridge_simple.cpp /Fo$(OBJ_DIR)\ac_project\bridge_simple.obj

$(OBJ_DIR)\ac_project\core_sphere.obj: src\ac_project\core_sphere.cpp src\include\model\simulation.h src\include\ac_project\core_sphere.h
	$(CC) $(CFLAGS) /c src\ac_project\core_sphere.cpp /Fo$(OBJ_DIR)\ac_project\core_sphere.obj

# ================================================
# DLLs
# ================================================

dlls:
	copy "$(VCPKG_ROOT)\bin\glfw3.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\freetype.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\zlib1.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\bz2.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\brotlidec.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\brotlicommon.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\brotlienc.dll" $(BUILD_DIR)
	copy "$(VCPKG_ROOT)\bin\libpng16.dll" $(BUILD_DIR)

# ================================================
# Limpeza
# ================================================

clean:
	@echo Limpando objetos...
	-@del /Q /F $(OBJ_DIR)\*.obj 2>nul
	-@del /Q /F $(OBJ_DIR)\ac_project\*.obj 2>nul

	@echo Limpando binarios...
	-@del /Q /F $(BUILD_DIR)\*.exe 2>nul
	-@del /Q /F $(BUILD_DIR)\*.dll 2>nul
	-@del /Q /F $(BUILD_DIR)\*.pdb 2>nul

# ================================================
# Configuração
# ================================================

copy_config:
	if exist automaton.cfg copy automaton.cfg "$(BUILD_DIR)\automaton.cfg"

# ================================================
# Execução
# ================================================

run:
	cd "$(BUILD_DIR)" && $(TARGET)

rebuild:
	nmake clean
	nmake

# ================================================
# Targets simbólicos
# ================================================

.SYMBOLIC: clean run rebuild all dirs dlls copy_config