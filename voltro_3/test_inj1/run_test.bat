@echo off
chcp 65001 >nul
echo ==========================================
echo Inj1Injector - Full Test Setup
    ==========================================
echo.

:: Step 1: Check if DLL exists
if not exist test_dll.dll (
    echo [!] test_dll.dll not found!
    echo [!] Compiling DLL first...
    call compile_dll.bat
)

:: Step 2: Check if injector exe exists
if not exist test_injector.exe (
    echo [!] test_injector.exe not found!
    echo [!] Compiling injector first...
    call compile_injector.bat
)

echo ==========================================
echo [+] Starting Injection Test
    ==========================================
echo.

:: Check if Roblox is running
set ROBLOX_FOUND=0
tasklist /FI "IMAGENAME eq RobloxPlayerBeta.exe" /FO CSV /NH | find "/" >nul
if errorlevel 1 (
    tasklist /FI "IMAGENAME eq Roblox.exe" /FO CSV /NH | find "/" >nul
    if errorlevel 1 (
        echo [!] Roblox is NOT running!
        echo [!] Please start Roblox first, then run this script
        pause
        exit /b 1
    ) else (
        set ROBLOX_FOUND=1
    )
) else (
    set ROBLOX_FOUND=1
)

if %ROBLOX_FOUND%==1 (
    echo [+] Roblox process detected!
    echo.
    echo [+] Running injector with test_dll.dll...
    echo.
    
    :: Run the injector - it will prompt for DLL path, but we provide it
    :: For automated testing, we can use echo to pipe the path
    echo test_dll.dll | test_injector.exe
    
    echo.
    echo ==========================================
    echo [+] Test completed!
    echo ==========================================
) else (
    echo [!] Could not detect Roblox process
)

echo.
pause
