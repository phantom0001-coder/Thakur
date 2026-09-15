@echo off
chcp 65001 >nul
echo ==========================================
echo Compiling Test DLL for Inj1Injector...
echo ==========================================
echo.

:: Check if cl.exe is available
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo [!] cl.exe not found in PATH
    echo [!] Please run this from Visual Studio Developer Command Prompt
    echo [!] Or set the PATH to include MSVC compiler
    pause
    exit /b 1
)

:: Compile the test DLL
cl.exe /LD /LDd test_dll.cpp /link user32.lib kernel32.lib

if errorlevel 1 (
    echo [!] Compilation failed!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo [+] SUCCESS! test_dll.dll compiled
echo ==========================================
echo.

:: Check if DLL was created
if exist test_dll.dll (
    echo [+] DLL file: test_dll.dll
    echo [+] Size: %~z0 bytes
    for %%F in (test_dll.dll) do set DLL_SIZE=%%~zF
    echo [+] Ready for injection testing!
) else (
    echo [!] ERROR: test_dll.dll was not created
)

echo.
pause
