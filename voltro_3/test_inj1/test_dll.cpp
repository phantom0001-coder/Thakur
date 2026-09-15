// ============================================================
//  test_dll.cpp  - Simple DLL for Roblox injection testing
//  Compile with: cl.exe /LD /LDd test_dll.cpp /link user32.lib
// ============================================================

#include <windows.h>
#include <stdio.h>

// Roblox Lua execution function (if available)
typedef int (*luaL_dostring_t)(void* L, const char* str);

// For testing - simple DLL that shows a message box and logs to console

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            // Show a message box to confirm injection
            MessageBoxA(NULL, "DLL Injected Successfully!", "Inj1Injector Test", MB_OK | MB_ICONINFORMATION);
            
            // Also print to console (if Roblox has one attached)
            OutputDebugStringA("[Inj1Injector] DLL loaded into Roblox process!\n");
            
            // Try to execute Lua code if we can find the Lua state
            // This is a simple test - in a real executor you'd hook into the game
            FILE* f = fopen("C:\\Users\\Administrator\\Desktop\\voltro_injection_test.log", "a");
            if (f) {
                fprintf(f, "[SUCCESS] DLL injected at %llu\n", GetTickCount64());
                fclose(f);
            }
        }
        break;
    case DLL_PROCESS_DETACH:
        OutputDebugStringA("[Inj1Injector] DLL unloading from Roblox process!\n");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

// Export a function for testing
__declspec(dllexport) void TestFunction()
{
    MessageBoxA(NULL, "TestFunction called!", "Inj1Injector", MB_OK);
}

__declspec(dllexport) const char* GetDLLName()
{
    return "Inj1Injector_Test_DLL";
}
