@echo off
setlocal EnableExtensions
pushd "%~dp0"
title THAKUR EXECUTOR Build System (Native Win32 + WebView2 + Monaco)
echo ===================================================================
echo  Compiling Thakur Standalone Native Host (thakur.exe)
echo  Architecture: Pure Win32 + Microsoft WebView2 + Monaco Editor
echo ===================================================================
echo.

tasklist /FI "IMAGENAME eq thakur.exe" /NH | find /I "thakur.exe" >nul
if not errorlevel 1 (
    echo [ERROR] thakur.exe is running. Close it, then run this build again.
    popd
    exit /b 1
)

set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%" == "" (
    echo [ERROR] Visual Studio C++ Environment not found.
    pause
    exit /b 1
)

echo [ENV] Setting up Visual Studio x64 Build Tools...
call "%VCVARS%" >nul 2>&1

for /d %%D in ("packages\Microsoft.Web.WebView2.*") do set "WV2_PKG=%%D"

if "%WV2_PKG%" == "" (
    echo [ERROR] WebView2 SDK package directory not found in packages.
    pause
    exit /b 1
)

set "WV2_INC=%WV2_PKG%\build\native\include"
set "WV2_LIB=%WV2_PKG%\build\native\x64\WebView2Loader.dll.lib"
set "WV2_DLL=%WV2_PKG%\build\native\x64\WebView2Loader.dll"

echo [BUILD] Compiling Windows Resources (V Logo Icon ^& Manifest)...
if exist "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" (
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe" /nologo /fo thakur.res thakur.rc
) else (
    rc.exe /nologo /fo thakur.res thakur.rc >nul 2>&1
)
if errorlevel 1 goto :build_error

echo [BUILD] Compiling thakur.exe...

cl /nologo /O2 /EHsc /W3 /std:c++17 /I"%WV2_INC%" /I"thakur_ui\src" /Fe:thakur.exe ^
    thakur_ui\src\main.cpp ^
    thakur_ui\src\script_hub.cpp ^
    thakur_ui\src\ipc\ipc_client.cpp ^
    thakur_ui\src\injector\ThakurInjector.cpp ^
    thakur.res ^
    "%WV2_LIB%" ^
    user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib wininet.lib dwmapi.lib /link /SUBSYSTEM:WINDOWS

if errorlevel 1 goto :build_error

echo.
echo [STAGING] Copying WebView2Loader.dll...
copy /y "%WV2_DLL%" "WebView2Loader.dll" >nul 2>&1
if errorlevel 1 goto :build_error
del /f /q main.obj script_hub.obj ipc_client.obj >nul 2>&1
echo ===================================================================
echo  [SUCCESS] thakur.exe built successfully!
echo ===================================================================
echo Ready to execute: thakur.exe
popd
endlocal
exit /b 0

:build_error
echo.
echo [ERROR] Build failed. Please inspect compilation output above.
popd
endlocal
exit /b 1
