@echo off

if not defined VC_ENV_SET (
	call "G:\Visual Studio 2022\VC\Auxiliary\Build\vcvars64.bat"
	set VC_ENV_SET=1
)

setlocal

set SOURCE_DIR=src
set INCLUDE_DIR=include
set LIB_DIR=lib

set SOURCE_FILES=%SOURCE_DIR%\*.cpp
set IMGUI_FILES=imgui\*.cpp

set NODEFAULTS=/NODEFAULTLIB:MSVCRT
set LINK_FLAGS=/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO
set LINKED_LIBRARIES=glfw3.lib libglew32.lib glew32.lib opengl32.lib freetype.lib kernel32.lib gdi32.lib shell32.lib Comdlg32.lib

REM NEEDED(?) => winspool.lib ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib

cl /O2 /GL /Fo.\release\obj\ /std:c++20 /EHsc /MT /MP %SOURCE_FILES% %IMGUI_FILES% /I%INCLUDE_DIR% /link %LINK_FLAGS% /OUT:release/Engine3D.exe %NODEFAULTS% /LIBPATH:%LIB_DIR% %LINKED_LIBRARIES%

endlocal
