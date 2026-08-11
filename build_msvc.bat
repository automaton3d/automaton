@echo off
setlocal EnableDelayedExpansion

:: build_msvc.bat - compila automaton.exe com nmake/MSVC
:: Rode dentro do "Developer Command Prompt for VS" na raiz do repo.

if not defined VCPKG_ROOT (
    set VCPKG_ROOT=E:\vcpkg\installed\x64-windows
)

set "VCPKG_LIB=%VCPKG_ROOT%\lib"

echo VCPKG_ROOT=%VCPKG_ROOT%

if not exist "%VCPKG_LIB%\glfw3dll.lib" (
    echo [ERRO] glfw3dll.lib nao encontrado em %VCPKG_LIB%
    echo Instale as dependencias com vcpkg:
    echo   vcpkg install glfw3 freetype zlib libpng bzip2 brotli --triplet x64-windows
    exit /b 1
)

if not exist "%VCPKG_LIB%\freetype.lib" (
    echo [ERRO] freetype.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)

if not exist "%VCPKG_LIB%\zlib.lib" (
    echo [ERRO] zlib.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)

echo [INFO] Iniciando build com nmake...
nmake
if errorlevel 1 (
    echo [ERRO] Build falhou.
    exit /b 1
)

echo [INFO] Build concluido.
exit /b 0
