@echo off
where cl
if %errorlevel% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

mkdir ..\build
cls

del /Q ..\res

ply ..\resnb\cube.ply ..\res\cube.bply
ply ..\resnb\tree.ply ..\res\tree.bply
ply ..\resnb\trunk.ply ..\res\trunk.bply

bmp ..\resnb\texture.bmp ..\res\texture.bbmp
bmp ..\resnb\bark.bmp ..\res\bark.bbmp

pushd ..\build
fxc -nologo -T vs_5_0 ..\resnb\defaultv.hlsl -Fo ..\res\defaultv.cso
fxc -nologo -T ps_5_0 ..\resnb\defaultp.hlsl -Fo ..\res\defaultp.cso

cl -nologo -Wall -wd5045 -wd4820 -wd4100 -wd4201 -DSLINGAME_DEBUG -MD -Z7 -Fmwin32_slingame.map ..\src\win32_slingame.c /link kernel32.lib user32.lib d3d11.lib winmm.lib /OPT:REF /subsystem:WINDOWS
popd
