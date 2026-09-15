================================================================================
                    Inj1Injector Standalone Test
================================================================================

This folder contains everything you need to test the Inj1Injector without
building the full Qt application.

================================================================================
                           FILES IN THIS FOLDER
================================================================================

1. Inj1Injector_standalone.h   - Header file (namespace-free)
2. Inj1Injector_standalone.cpp - Implementation (namespace-free)
3. test_main.cpp               - Console test application
4. test_dll.cpp                - Simple test DLL (shows message box)
5. test_lua_dll.cpp            - Lua execution test DLL (for Roblox)
6. compile_dll.bat             - Batch file to compile the test DLL
7. compile_injector.bat        - Batch file to compile the injector
8. run_test.bat                - Automated test script

================================================================================
                           QUICK START
================================================================================

OPTION 1 - Fully Automatic (RECOMMENDED):
----------------------------------------
1. Open "Developer Command Prompt for VS 2022" (x64 Native Tools)
   - Search for it in Start Menu
   - Make sure it's the x64 version

2. Navigate to this folder:
   cd C:\Users\Administrator\Desktop\voltro\voltro_3\test_inj1

3. Compile everything:
   compile_dll.bat
   compile_injector.bat

4. Start Roblox (RobloxPlayerBeta.exe)

5. Run the test:
   test_injector.exe
   When prompted, enter: test_dll.dll

6. If injection works, you should see a message box saying "DLL Injected Successfully!"


OPTION 2 - Using run_test.bat:
------------------------------
1. Open "Developer Command Prompt for VS 2022" (x64)
2. cd C:\Users\Administrator\Desktop\voltro\voltro_3\test_inj1
3. Run: run_test.bat
   This will automatically compile if needed and run the test


OPTION 3 - Manual Compilation:
------------------------------
1. Open "Developer Command Prompt for VS 2022" (x64)
2. cd C:\Users\Administrator\Desktop\voltro\voltro_3\test_inj1

3. Compile the DLL:
   cl.exe /LD /LDd test_dll.cpp /link user32.lib kernel32.lib

4. Compile the injector:
   cl.exe test_main.cpp Inj1Injector_standalone.cpp /EHsc /std:c++17 /Fe:test_injector.exe

5. Run:
   test_injector.exe


================================================================================
                           TROUBLESHOOTING
================================================================================

PROBLEM: cl.exe not found
SOLUTION: Use "Developer Command Prompt for VS 2022" or add MSVC to PATH

PROBLEM: Roblox not found
SOLUTION: Make sure Roblox (RobloxPlayerBeta.exe) is running before testing

PROBLEM: Injection failed
SOLUTION: 
  - Run as Administrator
  - Make sure Roblox is not protected by anti-cheat
  - Try with a different DLL
  - Check if the DLL path is correct

PROBLEM: Compilation errors
SOLUTION: Make sure you're using x64 Native Tools Command Prompt


================================================================================
                           WHAT TO EXPECT
================================================================================

If injection is SUCCESSFUL:
- You will see a message box: "DLL Injected Successfully!"
- A log file will be created: C:\Users\Administrator\Desktop\voltro_injection_test.log
- The test_injector.exe will show: [+] SUCCESS! DLL injected into Roblox!

If injection FAILS:
- The injector will show: [!] FAILED to inject DLL!
- Check the error messages for clues


================================================================================
                           ADVANCED TESTING
================================================================================

To test with the Lua DLL (if you have Roblox Lua API access):
1. Compile test_lua_dll.cpp instead:
   cl.exe /LD /LDd test_lua_dll.cpp /link user32.lib kernel32.lib

2. Run: test_injector.exe
3. Enter: test_lua_dll.dll

This will attempt to execute Lua code in Roblox and show a notification.


================================================================================
                           NOTES
================================================================================

- The injector uses NtSetIoCompletion for stealth injection
- It uses manual mapping with section stomping
- The test DLL is simple and just shows a message box
- For a real executor, you'd need proper Lua state access
- Always test in a safe environment

================================================================================
