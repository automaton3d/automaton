@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ===============================
echo   Inicializando MSVC 64-bit...
echo ===============================

call "E:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo ERRO: Não foi possível carregar vcvars64.bat
    pause
    exit /b 1
)

echo.
echo ===============================
echo   Recolhendo arquivos .cpp...
echo ===============================

set SRC=

for /r src %%f in (*.cpp) do (
    set "SRC=!SRC! %%f"
)

echo ===============================
echo   Compilando...
echo ===============================

cl.exe ^
 /std:c++17 ^
 /I C:\GL\GLUT\include ^
 /I src\include ^
 /I src\include\zlib ^
 /I src\include\model ^
 /I src\glad ^
 /I C:\GL\GLM ^
 /I C:\GL\GLFWx64\include ^
 !SRC! ^
 /Fe:bin\programa.exe ^
 /link ^
 C:\GL\GLUT\lib\freeglut.lib ^
 opengl32.lib ^
 gdi32.lib ^
 winmm.lib ^

if %errorlevel% neq 0 (
    echo.
    echo ===============================
    echo   COMPILAÇÃO FALHOU
    echo ===============================
    pause
    exit /b 1
)

echo.
echo ===============================
echo   COMPILAÇÃO CONCLUÍDA
echo ===============================
pause
