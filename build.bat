@echo off

if not defined VC_ENV_SET (
	call "G:\Visual Studio 2022\VC\Auxiliary\Build\vcvars64.bat"
	set VC_ENV_SET=1
)

setlocal

set SOURCE_DIR=src
set INCLUDE_DIR=include
set LIB_DIR=lib
set OUTPUT_DIR=debug_build
set OUTPUT_FILE=Engine3D.exe

set SOURCE_FILES=src\main.cpp %SOURCE_DIR%\*.cpp
set IMGUI_FILES=imgui\*.cpp

set NODEFAULTS=/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMT
set LINKED_LIBRARIES=glfw3.lib libglew32.lib glew32.lib opengl32.lib freetype.lib user32.lib kernel32.lib shell32.lib gdi32.lib

REM glfw3.lib;opengl32.lib;kernel32.lib;user32.lib;;winspool.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;comdlg32.lib;advapi32.lib;glfw3.lib

cl /Fo.\build\obj\ /Febuild\Engine3D.exe /std:c++20 /EHsc /MDd %SOURCE_FILES% %IMGUI_FILES% /I%INCLUDE_DIR% /link %NODEFAULTS% /LIBPATH:%LIB_DIR% %LINKED_LIBRARIES%

endlocal
