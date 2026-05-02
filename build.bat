@echo off
setlocal enabledelayedexpansion

REM === PATHS ===
set ROOT=%~dp0
set SRC=%ROOT%src
set BUILD=%ROOT%build
set LIBDIR=%ROOT%lib

REM === VCPKG ===
set VCPKG_ROOT=E:\vcpkg
set VCPKG_INC=%VCPKG_ROOT%\installed\x64-windows\include
set VCPKG_LIB=%VCPKG_ROOT%\installed\x64-windows\lib
set VCPKG_BIN=%VCPKG_ROOT%\installed\x64-windows\bin

REM === VISUAL STUDIO ===
set VS_PATH=E:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat
call "%VS_PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"

REM === INCLUDES (vcpkg primeiro) ===
set INCLUDES=/I"%VCPKG_INC%" /I"%SRC%\include" /I"%SRC%\include\zlib" /I"%SRC%"

REM === SOURCE FILES ===
set FILES="%ROOT%principal.cpp" "%ROOT%glad\glad.c" "%SRC%\tinyfiledialogs.c"

for /R "%SRC%" %%f in (*.cpp) do (
    set FILES=!FILES! "%%f"
)

pushd "%BUILD%"

echo Compilando...

cl /std:c++20 /O2 /EHsc /MD %INCLUDES% !FILES! ^
    /link ^
    /LIBPATH:"%LIBDIR%" ^
    /LIBPATH:"%VCPKG_LIB%" ^
    opengl32.lib ^
    glfw3dll.lib ^
    freetype.lib ^
    brotlidec.lib ^
    brotlicommon.lib ^
    bz2.lib ^
    zlib.lib ^
    user32.lib ^
    gdi32.lib ^
    shell32.lib ^
    kernel32.lib ^
    ole32.lib ^
    comdlg32.lib ^
    /OUT:automaton.exe

if %errorlevel% neq 0 (
    echo.
    echo ERRO NA COMPILACAO
    popd
    pause
    exit /b 1
)

echo.
echo Copiando DLLs necessarias...
copy "%VCPKG_BIN%\glfw3.dll"     . >nul 2>nul || echo AVISO: glfw3.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\freetype.dll"  . >nul 2>nul || echo AVISO: freetype.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\zlib1.dll"     . >nul 2>nul || echo AVISO: zlib1.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\bz2.dll"       . >nul 2>nul || echo AVISO: bz2.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\brotlidec.dll" . >nul 2>nul || echo AVISO: brotlidec.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\brotlicommon.dll" . >nul 2>nul || echo AVISO: brotlicommon.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\brotlienc.dll" . >nul 2>nul || echo AVISO: brotlienc.dll nao encontrada no vcpkg
copy "%VCPKG_BIN%\libpng16.dll"  . >nul 2>nul || echo AVISO: libpng16.dll nao encontrada no vcpkg

echo.
echo BUILD CONCLUIDO COM SUCESSO - automaton.exe gerado
popd
endlocal
pause
