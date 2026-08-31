@echo off
rem Build for the dispersion test (dispersion.exe).
cd /d e:\automaton
if not exist build mkdir build
cl /nologo /std:c++20 /O2 /EHsc /MD /D "NOMINMAX" tests\dispersion.cpp /Fe:build\dispersion.exe /link /INCREMENTAL:NO
echo BUILD_EXIT=%errorlevel%
