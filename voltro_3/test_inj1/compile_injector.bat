@echo off
chcp 65001 >nul
echo ==========================================
echo Compiling Inj1Injector Standalone Test...
echo ==========================================
echo.

:: Check if cl.exe is available
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo [!] cl.exe not found in PATH
    echo [!] Please run this from Visual Studio Developer Command Prompt (x64)
    echo [!] Or set the PATH to include MSVC compiler
    pause
    exit /b 1
)

:: Compile the standalone injector test
cl.exe test_main.cpp Inj1Injector_standalone.cpp /EHsc /std:c++17 /Fe:test_injector.exe

if errorlevel 1 (
    echo [!] Compilation failed!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo [+] SUCCESS! test_injector.exe compiled
echo ==========================================
echo.

if exist test_injector.exe (
    echo [+] Executable: test_injector.exe
    for %%F in (test_injector.exe) do set EXE_SIZE=%%~zF
    echo [+] Size: %EXE_SIZE% bytes
) else (
    echo [!] ERROR: test_injector.exe was not created
)

echo.
echo [+] To test:
if exist test_dll.dll (
    echo [+]   1. Make sure Roblox is running
    echo [+]   2. Run: test_injector.exe
    echo [+]   3. Enter path: test_dll.dll
) else (
    echo [+]   1. First compile the DLL: compile_dll.bat
    echo [+]   2. Make sure Roblox is running
    echo [+]   3. Run: test_injector.exe
    echo [+]   4. Enter path: test_dll.dll
)

echo.
pause
