@echo off
SETLOCAL

set LIBS_FOLDER=%CD%\libs\Win32

set GLEW_INC=%LIBS_FOLDER%\glew\include
set GLEW_LIB=%LIBS_FOLDER%\glew\lib\Release\x64
set GLEW_BIN=%LIBS_FOLDER%\glew\bin\Release\x64

set GLFW_INC=%LIBS_FOLDER%\glfw3\include
set GLFW_LIB=%LIBS_FOLDER%\glfw3\lib\x64
set GLFW_BIN=%GLFW_LIB%

set GLM_INC=%LIBS_FOLDER%\glm

set CommonCompilerFlags=-Zi -Od -EHsc -nologo -FC -I%GLEW_INC% -I%GLFW_INC% -I%GLM_INC% -std:c++14
set CommonLinkerFlags=-debug -libpath:%GLEW_LIB% -libpath:%GLFW_LIB% glew32.lib glfw3dll.lib opengl32.lib

if not exist bin (
    mkdir bin
)
pushd bin
if not exist glew32.dll (
    robocopy %GLEW_BIN% . *.dll
)
if not exist glfw3.dll (
    robocopy %GLFW_BIN% . *.dll
)
cl %CommonCompilerFlags% ..\src\main.cpp -link -subsystem:console %CommonLinkerFlags% -out:opengl_foobar.exe
popd