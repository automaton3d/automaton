@echo off
setlocal EnableDelayedExpansion

:: build_msvc.bat - compila automaton.exe com nmake/MSVC
:: Rode dentro do "Developer Command Prompt for VS" na raiz do repo.
::
:: Uso:
::   build_msvc.bat            (CPU-only)
::   build_msvc.bat cuda       (CUDA acelerado)

set "USE_CUDA=0"
if /I "%~1"=="cuda" set "USE_CUDA=1"

if "!USE_CUDA!"=="1" (
    echo [INFO] Modo CUDA ativado.

    if not defined CUDA_PATH (
        echo [INFO] Procurando CUDA Toolkit...
        set "CUDA_FOUND="
        for /d %%D in ("C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\*") do (
            if not defined CUDA_FOUND (
                if exist "%%D\bin\nvcc.exe" (
                    set "CUDA_FOUND=%%D"
                )
            )
        )
        if not defined CUDA_FOUND (
            if exist "C:\PROGRA~1\NVIDIA~2\CUDA\v13.2\bin\nvcc.exe" (
                set "CUDA_FOUND=C:\PROGRA~1\NVIDIA~2\CUDA\v13.2"
            )
        )
        if not defined CUDA_FOUND (
            echo [ERRO] CUDA nao encontrado. Defina CUDA_PATH e tente novamente.
            echo Exemplo: set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.2
            exit /b 1
        )
        set "CUDA_PATH=!CUDA_FOUND!"
    )

    if not exist "!CUDA_PATH!\bin\nvcc.exe" (
        echo [ERRO] nvcc.exe nao encontrado em !CUDA_PATH!\bin
        exit /b 1
    )

    for /f "delims=" %%I in ("!CUDA_PATH!") do set "CUDA_SHORT=%%~sI"
    set "PATH=!CUDA_SHORT!\bin;!PATH!"
    echo CUDA_PATH=!CUDA_SHORT!
)

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

echo [INFO] Limpando objetos antigos para evitar conflitos entre CPU/CUDA...
nmake VCPKG_ROOT=%VCPKG_SHORT% clean >nul 2>&1

if "!USE_CUDA!"=="1" (
    echo [INFO] Iniciando build CUDA com nmake VCPKG_ROOT=%VCPKG_SHORT% USE_CUDA=1 CUDA_PATH=!CUDA_SHORT! ...
    nmake VCPKG_ROOT=%VCPKG_SHORT% USE_CUDA=1 CUDA_PATH=!CUDA_SHORT!
) else (
    echo [INFO] Iniciando build com nmake VCPKG_ROOT=%VCPKG_SHORT% ...
    nmake VCPKG_ROOT=%VCPKG_SHORT%
)
if errorlevel 1 (
    echo [ERRO] Build falhou.
    exit /b 1
)

echo [INFO] Build concluido.

cd /d "%~dp0build"
automaton.exe

exit /b 0

:: ============================================================
:: Procura recursivamente por glfw3dll.lib ou glfw3.lib dentro
:: do diretorio passado. Acha o triplet (diretorio acima de lib)
:: que tambem contem freetype.lib e zlib.lib.
:: ============================================================
:search_libs
set "SEARCH_DIR=%~1"
if not exist "%SEARCH_DIR%" exit /b 0

for /f "delims=" %%I in ('dir /S /B "%SEARCH_DIR%\glfw3dll.lib" "%SEARCH_DIR%\glfw3.lib" 2^>nul') do (
    :: %%~dpI e' o diretorio 'lib\' do arquivo
    set "LIB_DIR=%%~dpI"
    set "LIB_DIR=!LIB_DIR:~0,-1!"
    :: parent e' o diretorio do triplet (x64-windows)
    for %%J in ("!LIB_DIR!") do set "TRIPLET=%%~dpJ"
    set "TRIPLET=!TRIPLET:~0,-1!"
    :: verifica as outras libs no mesmo lib
    if exist "!LIB_DIR!\freetype.lib" (
        if exist "!LIB_DIR!\zlib.lib" (
            for %%K in ("!TRIPLET!") do set VCPKG_INSTALL=%%~fK
            exit /b 0
        )
    )
)
exit /b 0
