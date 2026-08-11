@echo off
setlocal EnableDelayedExpansion

:: build_msvc.bat - compila automaton.exe com nmake/MSVC
:: Rode dentro do "Developer Command Prompt for VS" na raiz do repo.

if not defined VCPKG_ROOT (
    if exist "E:\vcpkg\vcpkg.exe" (
        set VCPKG_ROOT=E:\vcpkg
    ) else if exist "E:\vcpkg\installed\x64-windows\lib" (
        set VCPKG_ROOT=E:\vcpkg
    )
)

if not defined VCPKG_ROOT (
    echo [ERRO] VCPKG_ROOT nao definido e nao encontrei vcpkg em E:\vcpkg
    exit /b 1
)

:: Tenta achar as libs primeiro no triplet padrao
if exist "%VCPKG_ROOT%\installed\x64-windows\lib\glfw3dll.lib" (
    set VCPKG_INSTALL=%VCPKG_ROOT%\installed\x64-windows
    goto found
)
if exist "%VCPKG_ROOT%\lib\glfw3dll.lib" (
    set VCPKG_INSTALL=%VCPKG_ROOT%
    goto found
)

:: Se nao achou, procura recursivamente por glfw3dll.lib ou glfw3.lib
set VCPKG_INSTALL=
call :search_libs "%VCPKG_ROOT%"
if not defined VCPKG_INSTALL call :search_libs "E:\vcpkg"
if not defined VCPKG_INSTALL (
    echo [ERRO] Nao encontrei glfw3dll.lib/glfw3.lib, freetype.lib e zlib.lib
    echo em VCPKG_ROOT=%VCPKG_ROOT% nem em E:\vcpkg
    echo Instale as dependencias com vcpkg:
    echo   vcpkg install glfw3 freetype zlib libpng bzip2 brotli --triplet x64-windows
    exit /b 1
)

:found
for /f "delims=" %%I in ("%VCPKG_INSTALL%") do set VCPKG_SHORT=%%~sI
set "VCPKG_LIB=%VCPKG_SHORT%\lib"
set "VCPKG_BIN=%VCPKG_SHORT%\bin"

echo VCPKG_ROOT=%VCPKG_ROOT%
echo VCPKG_INSTALL=%VCPKG_INSTALL%
echo VCPKG_SHORT=%VCPKG_SHORT%

if not exist "%VCPKG_LIB%\freetype.lib" (
    echo [ERRO] freetype.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)
if not exist "%VCPKG_LIB%\zlib.lib" (
    echo [ERRO] zlib.lib nao encontrado em %VCPKG_LIB%
    exit /b 1
)

echo [INFO] Iniciando build com nmake VCPKG_ROOT=%VCPKG_SHORT% ...
nmake VCPKG_ROOT=%VCPKG_SHORT%
if errorlevel 1 (
    echo [ERRO] Build falhou.
    exit /b 1
)

echo [INFO] Build concluido.
exit /b 0

:: ============================================================
:: Procura recursivamente por glfw3dll.lib ou glfw3.lib dentro do
:: diretorio passado e seta VCPKG_INSTALL para o triplet que as
:: contem (diretorio acima de 'lib').
:: ============================================================
:search_libs
set "SEARCH_DIR=%~1"
if not exist "%SEARCH_DIR%" exit /b 0

for /f "delims=" %%I in ('dir /S /B "%SEARCH_DIR%\glfw3dll.lib" "%SEARCH_DIR%\glfw3.lib" 2^>nul') do (
    set "CAND=%%~dpI"
    :: Remove a barra final do diretorio 'lib'
    set "CAND=!CAND:~0,-1!"
    for %%J in ("!CAND!\freetype.lib") do (
        if exist "%%~fJ" (
            for %%K in ("!CAND!\zlib.lib") do (
                if exist "%%~fK" (
                    for %%L in ("!CAND!") do set VCPKG_INSTALL=%%~fL
                    exit /b 0
                )
            )
        )
    )
)
exit /b 0
