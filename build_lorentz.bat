@echo off
rem Build for the headless Michelson-Morley runner (lorentz_mm.exe).
cd /d e:\automaton
if not exist obj mkdir obj
if not exist build mkdir build

cl /nologo /std:c++20 /O2 /EHsc /MD /D "NOMINMAX" ^
  /I"src\include" /I"src\include\zlib" /I"src" ^
  /I"E:\vcpkg\installed\x64-windows/include" ^
  tests\lorentz_mm.cpp ^
  src\model\simulation.cpp ^
  src\model\initSim.cpp ^
  src\model\interaction.cpp ^
  src\model\utils.cpp ^
  src\model\polarization.cpp ^
  src\model\charges.cpp ^
  src\model\wavefront.cpp ^
  /Fo:obj\ /Fe:build\lorentz_mm.exe /link /INCREMENTAL:NO

echo BUILD_EXIT=%errorlevel%
