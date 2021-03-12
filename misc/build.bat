@echo off
where cl
if %errorlevel% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

mkdir ..\build
cls

del /Q ..\res

rem ply ..\resnb\cube.ply ..\res\cube.bply
ply ..\resnb\tree.ply ..\res\tree.bply
rem ply ..\resnb\trunk.ply ..\res\trunk.bply
ply ..\resnb\bow.ply ..\res\bow.bply
ply ..\resnb\arrow.ply ..\res\arrow.bply

rem bmp ..\resnb\texture.bmp ..\res\texture.bbmp
bmp ..\resnb\bark.bmp ..\res\bark.bbmp

pushd ..\build

copy ..\resnb\icon.ico ..\build\icon.ico
copy ..\misc\slingame.rc ..\build\slingame.rc

rc -nologo slingame.rc

fxc -nologo -T vs_5_0 ..\resnb\defaultv.hlsl -Fo ..\res\defaultv.cso
fxc -nologo -T ps_5_0 ..\resnb\defaultp.hlsl -Fo ..\res\defaultp.cso

set "compilerflags=-nologo -Wall -wd5045 -wd4820 -wd4100 -wd4201"
set "linkerflags=slingame.res kernel32.lib user32.lib d3d11.lib winmm.lib /OPT:REF /subsystem:WINDOWS"

if "%1" equ "debug" (
   cl %compilerflags% -DSLINGAME_DEBUG -Z7 -Fmwin32_slingame.map ..\src\win32_slingame.c /link %linkerflags%
)

if "%1" equ "release" (
   cl %compilerflags% ..\src\win32_slingame.c /link %linkerflags%
)
popd
