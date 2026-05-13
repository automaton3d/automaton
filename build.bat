@echo off
cls
title Automaton Build System

setlocal

set MODE=%1

rem =====================================
rem ROUTER
rem =====================================

if "%MODE%"=="" goto build
if /I "%MODE%"=="?" goto help
if /I "%MODE%"=="help" goto help
if /I "%MODE%"=="clean" goto clean
if /I "%MODE%"=="cuda" goto cuda
if /I "%MODE%"=="cudarun" goto cudarun
if /I "%MODE%"=="run" goto run
if /I "%MODE%"=="rebuild" goto rebuild

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
echo   build.bat              ^> build (CPU)
echo   build.bat clean        ^> clean project
echo   build.bat cuda         ^> build with CUDA
echo   build.bat cudarun      ^> build + run with CUDA
echo   build.bat run          ^> build if needed + run
echo   build.bat rebuild      ^> clean + build + run
echo   build.bat ?            ^> show this help
echo   build.bat help         ^> show this help
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
goto end

rem =====================================
rem CUDA BUILD + RUN
rem =====================================

:cudarun
echo === Building + Running (CUDA ENABLED) ===
nmake USE_CUDA=1
if errorlevel 1 goto end
goto execute

rem =====================================
rem REBUILD
rem =====================================

:rebuild
echo === Rebuild (CPU) ===

nmake clean
if errorlevel 1 goto end

nmake
if errorlevel 1 goto end

goto execute

rem =====================================
rem RUN
rem =====================================

:run

if not exist build\automaton.exe (
	echo Executable not found. Building first...
	nmake
	if errorlevel 1 goto end
)

goto execute

rem =====================================
rem EXECUTE
rem =====================================

:execute
echo === Running ===

cd /d build

if not exist automaton.exe (
	echo ERROR: automaton.exe not found.
	cd ..
	goto end
)

automaton.exe

cd ..

goto end

rem =====================================
rem END
rem =====================================

:end
endlocal