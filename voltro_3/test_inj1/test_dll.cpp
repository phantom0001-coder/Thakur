




#include <windows.h>
#include <stdio.h>


typedef int (*luaL_dostring_t)(void* L, const char* str);



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            
            MessageBoxA(NULL, "DLL Injected Successfully!", "Inj1Injector Test", MB_OK | MB_ICONINFORMATION);
            
            
            OutputDebugStringA("[Inj1Injector] DLL loaded into Roblox process!\n");
            
            
            
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


__declspec(dllexport) void TestFunction()
{
    MessageBoxA(NULL, "TestFunction called!", "Inj1Injector", MB_OK);
}

__declspec(dllexport) const char* GetDLLName()
{
    return "Inj1Injector_Test_DLL";
}
