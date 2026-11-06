@echo off
rem CMake-free build script for the Enigma project.
rem Requires MSVC (cl.exe) on PATH, e.g. from a VS Developer Command Prompt.
rem Targets:
rem   build\enigma_tests.exe       - core algorithm tests (pure C++20)
rem   build\enigma_math_tests.exe  - math module tests (pure C++20)
rem   build\enigma_sim.exe         - 3D simulator (Win32 + wgl + OpenGL 3.3)
setlocal
cd /d "%~dp0"

if not exist build mkdir build

rem ---- Core tests (pure C++20: no windows.h, no GL) ----
cl /nologo /std:c++20 /EHsc /O2 /W4 ^
   /Icore /Itests ^
   /Fo:build\ ^
   core\enigma.cpp tests\test_core.cpp ^
   /Fe:build\enigma_tests.exe
if errorlevel 1 exit /b 1

echo Build OK: build\enigma_tests.exe
build\enigma_tests.exe
if errorlevel 1 exit /b 1

rem ---- Math tests (pure C++20: no windows.h, no GL) ----
cl /nologo /std:c++20 /EHsc /O2 /W4 ^
   /Imath /Itests ^
   /Fo:build\ ^
   tests\test_math.cpp ^
   /Fe:build\enigma_math_tests.exe
if errorlevel 1 exit /b 1

echo Build OK: build\enigma_math_tests.exe
build\enigma_math_tests.exe
if errorlevel 1 exit /b 1

rem ---- 3D simulator application (Win32 + wgl + OpenGL 3.3 Core) ----
cl /nologo /std:c++20 /EHsc /O2 /W4 ^
   /Icore /Iplatform /Igl /Iapp /Imath /Iscene ^
   /Fo:build\ ^
   core\enigma.cpp platform\platform_window_win32.cpp gl\gl_loader_wgl.cpp ^
   scene\mesh.cpp scene\camera.cpp scene\renderer.cpp app\main.cpp ^
   /Fe:build\enigma_sim.exe ^
   /link user32.lib gdi32.lib opengl32.lib
if errorlevel 1 exit /b 1

echo Build OK: build\enigma_sim.exe
exit /b 0
