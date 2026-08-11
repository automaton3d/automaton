@echo off
setlocal EnableDelayedExpansion

:: build_msvc.bat - compila automaton.exe com nmake/MSVC
:: Rode dentro do "Developer Command Prompt for VS" na raiz do repo.
:: Se VCPKG_ROOT nao estiver definido, tenta E:\vcpkg (raiz) ou
:: E:\vcpkg\installed\x64-windows (triplet ja instalado).

if not defined VCPKG_ROOT (
    if exist "E:\vcpkg\vcpkg.exe" (
        set VCPKG_ROOT=E:\vcpkg
    ) else if exist "E:\vcpkg\installed\x64-windows\lib\glfw3dll.lib" (
        set VCPKG_ROOT=E:\vcpkg\installed\x64-windows
    )
)

if not defined VCPKG_ROOT (
    echo [ERRO] VCPKG_ROOT nao definido e nao encontrei vcpkg em E:\vcpkg
    exit /b 1
)

:: Se VCPKG_ROOT aponta para a raiz do vcpkg, usa o triplet x64-windows
if exist "%VCPKG_ROOT%\installed\x64-windows\lib" (
    set VCPKG_INSTALL=%VCPKG_ROOT%\installed\x64-windows
) else (
    set VCPKG_INSTALL=%VCPKG_ROOT%
)

set "VCPKG_LIB=%VCPKG_INSTALL%\lib"
set "VCPKG_BIN=%VCPKG_INSTALL%\bin"

echo VCPKG_ROOT=%VCPKG_ROOT%
echo VCPKG_INSTALL=%VCPKG_INSTALL%

if not exist "%VCPKG_LIB%\glfw3dll.lib" (
    if not exist "%VCPKG_LIB%\glfw3.lib" (
        echo [ERRO] glfw3dll.lib / glfw3.lib nao encontrado em %VCPKG_LIB%
        echo Instale as dependencias com vcpkg:
        echo   vcpkg install glfw3 freetype zlib libpng bzip2 brotli --triplet x64-windows
        exit /b 1
    )
)

if not exist "%VCPKG_LIB%\freetype.lib" (
    echo [ERRO] freetype.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)

if not exist "%VCPKG_LIB%\zlib.lib" (
    echo [ERRO] zlib.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)

:: Exporta o caminho do triplet para o nmake/Makefile
set VCPKG_ROOT=%VCPKG_INSTALL%

echo [INFO] Iniciando build com nmake...
nmake
if errorlevel 1 (
    echo [ERRO] Build falhou.
    exit /b 1
)

echo [INFO] Build concluido.
exit /b 0
