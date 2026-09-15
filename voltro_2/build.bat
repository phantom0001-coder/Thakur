@echo off
title VOLTRO 2 (Solara RBLX Edition) Build System
echo ===================================================================
echo  Compiling Voltro 2 - Standalone Executable ^& Solara Runtime
echo  Targets: voltro2_runtime.dll + voltro2.exe
echo ===================================================================
echo.
REM Terminate running voltro2.exe instance so linker doesn't lock
taskkill /F /IM voltro2.exe >nul 2>&1
timeout /t 1 /nobreak >nul 2>&1

REM 1. Locate Visual Studio C++ Compiler
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%" == "" (
    echo [ERROR] Visual Studio C++ Environment not found.
    exit /b 1
)

echo [ENV] Setting up Visual Studio x64 Build Tools...
call "%VCVARS%" >nul 2>&1

if not exist "%~dp0build" mkdir "%~dp0build"

echo [BUILD] 1/3 Compiling voltro2_runtime.dll (Xeno Backend)...
cl /nologo /O2 /EHsc /W3 /std:c++20 /I"%~dp0include" /LD /Fe:"%~dp0build\voltro2_runtime.dll" ^
    "%~dp0src\voltro2_runtime.cpp" ^
    user32.lib psapi.lib advapi32.lib

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] voltro2_runtime.dll build failed.
    exit /b 1
)

echo [BUILD] 2/3 Compiling voltro2.exe (Native Xeno Win32 UI)...
cl /nologo /O2 /EHsc /W3 /std:c++20 ^
    /I"%~dp0include" ^
    /I"%~dp0..\voltro_ui\src" ^
    /Fe:"%~dp0build\voltro2.exe" ^
    "%~dp0src\main_app.cpp" ^
    "%~dp0..\voltro_ui\src\script_hub.cpp" ^
    "%~dp0..\voltro.res" ^
    user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib wininet.lib dwmapi.lib ^
    /link /SUBSYSTEM:WINDOWS

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] voltro2.exe build failed.
    exit /b 1
)

echo [BUILD] 3/3 Compiling test_runtime.exe...
cl /nologo /O2 /EHsc /W3 /std:c++20 "%~dp0tests\test_runtime.cpp" /Fe:"%~dp0build\test_runtime.exe" user32.lib advapi32.lib psapi.lib

del /f /q *.obj >nul 2>&1
del /f /q "%~dp0build\*.exp" >nul 2>&1
del /f /q "%~dp0build\*.lib" >nul 2>&1

echo.
echo ===================================================================
echo  [SUCCESS] All Voltro 2 components built successfully!
echo  Application : voltro_2\build\voltro2.exe (Native Win32, 0% WebView2)
echo  Runtime DLL : voltro_2\build\voltro2_runtime.dll
echo ===================================================================
echo.
