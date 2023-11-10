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

set SOURCE_FILES=%SOURCE_DIR%\*.cpp
set HEADER_FILES=%INCLUDE_DIR%\*.h %INCLUDE_DIR%\*.hpp
set LIBRARY_FILES=%LIB_DIR%\*.lib

cl /EHsc /std:c++20 src\main.cpp /I%INCLUDE_DIR% /link /SUBSYSTEM:CONSOLE /NODEFAULTLIB:MSVCRT /LIBPATH:%LIB_DIR% glfw3.lib libglew32.lib glew32.lib opengl32.lib freetype.lib

endlocal
