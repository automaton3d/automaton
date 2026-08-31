@echo off
rem Temporary build for the headless attractor/closure/retina runner.
cd /d e:\automaton
if not exist obj mkdir obj
if not exist build mkdir build

cl /nologo /std:c++20 /O2 /EHsc /MD /D "NOMINMAX" ^
  /I"src\include" /I"src\include\zlib" /I"src" ^
  /I"E:\vcpkg\installed\x64-windows/include" ^
  /I"E:\vcpkg\installed\x64-windows/include/freetype2" ^
  tests\attractor_main.cpp ^
  src\model\simulation.cpp ^
  src\model\initSim.cpp ^
  src\model\interaction.cpp ^
  src\model\utils.cpp ^
  src\model\polarization.cpp ^
  src\model\charges.cpp ^
  src\model\attractor.cpp ^
  /Fo:obj\ /Fe:build\attractor_f1.exe /link /INCREMENTAL:NO

echo BUILD_EXIT=%errorlevel%