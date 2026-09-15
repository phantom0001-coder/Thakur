@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ==========================================================================
echo                    Inj1Injector - COMPLETE TEST
========================================================================
echo.
echo     This script will:
echo     1. Compile the test DLL
echo     2. Compile the injector executable
echo     3. Check if Roblox is running
echo     4. Inject and test automatically
    ==========================================================================
echo.

:: Check for Administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] WARNING: Not running as Administrator!
    echo [!] Injection may fail - please run as Administrator
    echo.
)

:: Check if we're in the right directory
if not exist "%~dp0test_main.cpp" (
    echo [!] ERROR: Not in the test_inj1 directory!
    echo [!] Please run from: C:\Users\Administrator\Desktop\voltro\voltro_3\test_inj1
    pause
    exit /b 1
)

:: Check if cl.exe is available
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo [!] ERROR: cl.exe (Visual Studio C++ Compiler) not found!
    echo.
    echo     SOLUTION 1: Open "Developer Command Prompt for VS 2022" (x64)
    echo                   Then navigate to this folder and run TEST_NOW.bat
    echo.
    echo     SOLUTION 2: Set PATH to include MSVC compiler:
    echo                   call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo.
    pause
    exit /b 1
)

echo [+] Step 1: Compiling test DLL...
echo ----------------------------------------------------------------------------

:: Compile the test DLL
cl.exe /LD /LDd test_dll.cpp /link user32.lib kernel32.lib >nul 2>&1
if errorlevel 1 (
    echo [!] FAILED to compile test_dll.cpp
    echo.
    echo     Trying with full include paths...
    set INCLUDE=%INCLUDE%;"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt"
    set INCLUDE=%INCLUDE%;"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\um"
    set INCLUDE=%INCLUDE%;"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\shared"
    set LIB=%LIB%;"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22000.0\ucrt\x64"
    set LIB=%LIB%;"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22000.0\um\x64"
    
    cl.exe /LD /LDd test_dll.cpp /link user32.lib kernel32.lib
    if errorlevel 1 (
        echo [!] STILL FAILED!
        echo.
        pause
        exit /b 1
    )
)

if not exist test_dll.dll (
    echo [!] ERROR: test_dll.dll was not created!
    pause
    exit /b 1
)

echo [+] SUCCESS: test_dll.dll compiled
for %%F in (test_dll.dll) do set /a DLL_SIZE=%%~zF
for %%F in (test_dll.dll) do set DLL_DATE=%%~tF
for %%F in (test_dll.dll) do set DLL_TIME=%%~zF
echo [+] Size: %DLL_SIZE% bytes
echo.

echo [+] Step 2: Compiling injector executable...
echo ----------------------------------------------------------------------------

:: Compile the injector
cl.exe test_main.cpp Inj1Injector_standalone.cpp /EHsc /std:c++17 /Fe:test_injector.exe >nul 2>&1
if errorlevel 1 (
    echo [!] FAILED to compile test_injector.exe
    echo.
    echo     Trying with full include paths...
    cl.exe test_main.cpp Inj1Injector_standalone.cpp /EHsc /std:c++17 /Fe:test_injector.exe /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\ucrt" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\shared"
    if errorlevel 1 (
        echo [!] STILL FAILED!
        echo.
        pause
        exit /b 1
    )
)

if not exist test_injector.exe (
    echo [!] ERROR: test_injector.exe was not created!
    pause
    exit /b 1
)

echo [+] SUCCESS: test_injector.exe compiled
for %%F in (test_injector.exe) do set /a EXE_SIZE=%%~zF
echo [+] Size: %EXE_SIZE% bytes
echo.

echo [+] Step 3: Checking for Roblox process...
echo ----------------------------------------------------------------------------

:: Check if Roblox is running
set ROBLOX_RUNNING=0
set ROBLOX_PID=0

for /f "tokens=2 delims=," %%A in ('tasklist /FI "IMAGENAME eq RobloxPlayerBeta.exe" /FO CSV /NH /NOHEADER ^| find "/"') do (
    set ROBLOX_RUNNING=1
    set ROBLOX_PID=%%A
)

if %ROBLOX_RUNNING%==0 (
    for /f "tokens=2 delims=," %%A in ('tasklist /FI "IMAGENAME eq Roblox.exe" /FO CSV /NH /NOHEADER ^| find "/"') do (
        set ROBLOX_RUNNING=1
        set ROBLOX_PID=%%A
    )
)

if %ROBLOX_RUNNING%==0 (
    echo [!] Roblox is NOT running!
    echo.
    echo     Starting Roblox automatically might not work.
    echo     Please manually start Roblox (RobloxPlayerBeta.exe), then run this script again.
    echo.
    echo     Or: Start Roblox, then press any key to continue...
    pause >nul
    
    :: Re-check after user presses a key
    for /f "tokens=2 delims=," %%A in ('tasklist /FI "IMAGENAME eq RobloxPlayerBeta.exe" /FO CSV /NH /NOHEADER ^| find "/"') do (
        set ROBLOX_RUNNING=1
        set ROBLOX_PID=%%A
    )
    
    if %ROBLOX_RUNNING%==0 (
        for /f "tokens=2 delims=," %%A in ('tasklist /FI "IMAGENAME eq Roblox.exe" /FO CSV /NH /NOHEADER ^| find "/"') do (
            set ROBLOX_RUNNING=1
            set ROBLOX_PID=%%A
        )
    )
)

if %ROBLOX_RUNNING%==1 (
    echo [+] Roblox is RUNNING!
    if not "%ROBLOX_PID%"=="0" (
        echo [+] Process ID: %ROBLOX_PID%
    )
) else (
    echo [!] Roblox is still NOT running!
    echo [!] Cannot continue without Roblox process
    echo.
    pause
    exit /b 1
)

echo.
echo [+] Step 4: Running injection test...
echo ----------------------------------------------------------------------------
echo.

:: Run the test with automatic DLL path
echo Injecting test_dll.dll into Roblox (PID: %ROBLOX_PID%)...
echo.

:: Create a temporary response file
echo test_dll.dll > temp_input.txt

:: Run the injector with the DLL path
test_injector.exe < temp_input.txt > test_output.txt 2>&1

:: Clean up
del temp_input.txt 2>nul

:: Check the output
type test_output.txt

:: Parse the output
set TEST_RESULTS=test_output.txt
set SUCCESS=0
set FAIL=0

for /f "delims=" %%A in ('type test_output.txt ^| find /i "SUCCESS"') do (
    set SUCCESS=1
)

for /f "delims=" %%A in ('type test_output.txt ^| find /i "FAILED"') do (
    set FAIL=1
)

:: Clean up output file
del test_output.txt 2>nul

echo.
echo ==========================================================================

if %SUCCESS%==1 (
    echo                        RESULT: INJECTION SUCCESSFUL!
) else if %FAIL%==1 (
    echo                        RESULT: INJECTION FAILED
) else (
    echo                        RESULT: UNKNOWN (check output)
)

    ==========================================================================
echo.

if %SUCCESS%==1 (
    echo [+] A message box should have appeared in Roblox!
    echo [+] Check for: "DLL Injected Successfully!"
    echo [+] Log file created: C:\Users\Administrator\Desktop\voltro_injection_test.log
) else (
    echo [+] Check the error messages above for troubleshooting
)

echo.
echo [+] Files created:
if exist test_dll.dll echo     - test_dll.dll (test DLL)
if exist test_injector.exe echo     - test_injector.exe (injector)
if exist test_dll.pdb echo     - test_dll.pdb (debug symbols)
if exist test_injector.pdb echo     - test_injector.pdb (debug symbols)
if exist test_dll.exp echo     - test_dll.exp (export library)
if exist test_dll.lib echo     - test_dll.lib (import library)
if exist test_dll.obj echo     - test_dll.obj (object file)
if exist test_main.obj echo     - test_main.obj (object file)
if exist Inj1Injector_standalone.obj echo     - Inj1Injector_standalone.obj (object file)

echo.
echo [+] You can now:
echo     1. Manually test with: test_injector.exe
echo     2. Compile Lua DLL with: cl.exe /LD test_lua_dll.cpp /link user32.lib
    ==========================================================================
echo.

endlocal
pause
