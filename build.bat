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
set BUILD_FLAGS=/DEBUG /INCREMENTAL
set LINKED_LIBRARIES=glfw3.lib freetype.lib OpenAL32.lib kernel32.lib shell32.lib opengl32.lib Comdlg32.lib gdi32.lib

REM NEEDED(?) => winspool.lib ole32.lib oleaut32.lib uuid.lib advapi32.lib user32.lib 

cl /Fo.\debug\obj\ /std:c++20 /EHsc /Zi /Od /MTd /MP %SOURCE_FILES% src/glad.c %IMGUI_FILES% /I%INCLUDE_DIR% /link %BUILD_FLAGS% /OUT:debug/Engine3D_debug.exe %NODEFAULTS% /LIBPATH:%LIB_DIR% %LINKED_LIBRARIES%

endlocal
