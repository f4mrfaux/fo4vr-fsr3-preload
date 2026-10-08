@echo off
rem Builds FSR3Preload.dll (x64, static CRT) and the test harness into .\build
rem Run from an "x64 Native Tools Command Prompt for VS 2022" (or any shell where cl.exe targets x64).
setlocal
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /W4 /O2 /MT /D_CRT_SECURE_NO_WARNINGS /LD src\FSR3Preload.c /Fo:build\ /Fe:build\FSR3Preload.dll /link /NOLOGO shell32.lib ole32.lib || exit /b 1
cl /nologo /W4 /O2 /MT test\harness.c /Fo:build\ /Fe:build\harness.exe /link /NOLOGO || exit /b 1
echo Built build\FSR3Preload.dll
