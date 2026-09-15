@echo off
chcp 65001 >nul
echo ==========================================================================
echo                    Rebuilding Voltro3 with WebView2
========================================================================
echo.

:: Step 0: Set up environment for MSVC 2022
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [!] ERROR: Could not set up MSVC environment
    pause
    exit /b 1
)

echo [+] MSVC 2022 environment loaded

:: Step 1: Clean old build
if exist build (
    echo [+] Cleaning old build directory...
    rmdir /s /q build 2>nul
)

:: Step 2: Check for WebView2 files
if not exist "%~dp0WebView2Loader.dll" (
    echo [+] Copying WebView2Loader.dll from root...
    copy "%~dp0..\WebView2Loader.dll" "%~dp0WebView2Loader.dll" >nul 2>&1
    if errorlevel 1 (
        echo [!] WARNING: Could not copy WebView2Loader.dll
    )
)

:: Step 3: Configure CMake
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [!] ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo [+] CMake configuration successful

:: Step 4: Build
cmake --build build --config Release
if errorlevel 1 (
    echo [!] ERROR: Build failed
    echo.
    echo Checking for Qt...
    where qmake >nul 2>&1
    if errorlevel 1 (
        echo [!] Qt not found in PATH
        echo [+] Trying with Qt from C:\Qt\6.4.3\msvc2019_64...
        set PATH=C:\Qt\6.4.3\msvc2019_64\bin;%PATH%
        cmake --build build --config Release
    )
    if errorlevel 1 (
        pause
        exit /b 1
    )
)

echo [+] Build successful

:: Step 5: Copy WebView2Loader.dll to bin directory
if exist "%~dp0build\bin\voltro3.exe" (
    echo [+] Copying WebView2Loader.dll to build/bin/
    copy /y "%~dp0WebView2Loader.dll" "%~dp0build\bin\WebView2Loader.dll" >nul 2>&1
    
    echo.
    echo ==========================================================================
    echo [+] SUCCESS! voltro3.exe built with WebView2 support
    echo ==========================================================================
    echo.
    echo [+] Output: %~dp0build\bin\voltro3.exe
    echo [+] WebView2 DLL: %~dp0build\bin\WebView2Loader.dll
    echo.
    echo [+] To run: cd build\bin && start voltro3.exe
    echo.
) else (
    echo [!] ERROR: voltro3.exe not found in build/bin/
)

pause
