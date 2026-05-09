@echo off
cls
setlocal

set MODE=%1

rem =====================================
rem ROUTER
rem =====================================

if "%MODE%"=="" goto build
if "%MODE%"=="?" goto help
if "%MODE%"=="help" goto help
if "%MODE%"=="clean" goto clean
if "%MODE%"=="cuda" goto cuda
if "%MODE%"=="run" goto run
if "%MODE%"=="rebuild" goto rebuild

goto build

rem =====================================
rem HELP
rem =====================================
:help
echo =====================================
echo Automaton Build Script (build.bat)
echo =====================================
echo.
echo Usage:
echo   build.bat           ^> build (CPU)
echo   build.bat clean     ^> clean project
echo   build.bat cuda      ^> build with CUDA
echo   build.bat run       ^> build + run
echo   build.bat rebuild   ^> clean + build + run
echo   build.bat ?         ^> show this help
echo   build.bat help      ^> show this help
echo.
goto end

rem =====================================
rem BUILD (CPU)
rem =====================================
:build
echo === Building (CPU) ===
nmake
goto end

rem =====================================
rem CLEAN
rem =====================================
:clean
echo === Cleaning ===
nmake clean
goto end

rem =====================================
rem CUDA BUILD
rem =====================================
:cuda
echo === Building (CUDA ENABLED) ===
nmake USE_CUDA=1
if errorlevel 1 goto end
goto run

rem =====================================
rem REBUILD
rem =====================================
:rebuild
echo === Rebuild (CPU) ===
nmake clean
nmake
if errorlevel 1 goto end
goto run

rem =====================================
rem RUN
rem =====================================
:run
echo === Running ===
cd /d build
automaton.exe
cd ..
goto end

rem =====================================
rem END
rem =====================================
:end
endlocal