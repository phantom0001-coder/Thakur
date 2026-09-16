





#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>


typedef struct lua_State lua_State;

typedef int (*luaL_dostring_t)(lua_State* L, const char* str);


lua_State* g_L = nullptr;
luaL_dostring_t g_luaL_dostring = nullptr;


bool FindLuaState()
{
    
    
    
    HMODULE hRoblox = GetModuleHandleA(NULL); 
    if (!hRoblox)
        return false;
    
    
    
    
    
    return true;
}


void ExecuteLua(const char* code)
{
    if (g_L && g_luaL_dostring)
    {
        int result = g_luaL_dostring(g_L, code);
        if (result != 0)
        {
            OutputDebugStringA("[Inj1Injector] Lua execution failed!\n");
        }
        else
        {
            OutputDebugStringA("[Inj1Injector] Lua executed successfully!\n");
        }
    }
    else
    {
        
        FILE* f = fopen("C:\\Users\\Administrator\\Desktop\\roblox_test_script.lua", "w");
        if (f)
        {
            fprintf(f, "%s\n", code);
            fclose(f);
        }
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            OutputDebugStringA("[Inj1Injector] Lua DLL loaded!\n");
            
            
            FILE* f = fopen("C:\\Users\\Administrator\\Desktop\\voltro_lua_test.log", "a");
            if (f)
            {
                fprintf(f, "[SUCCESS] Lua DLL injected at %llu\n", GetTickCount64());
                fprintf(f, "[INFO] Attempting to execute Lua code...\n");
                fclose(f);
            }
            
            
            if (FindLuaState())
            {
                
                ExecuteLua("game:GetService(game, \"StarterGui\"):SetCore(\"SendNotification\", {Title = \"Inj1Injector\", Text = \"DLL Injection Successful!\", Duration = 5})");
            }
            
            
            MessageBoxA(NULL, "Lua DLL Injected!\nCheck Roblox for notification", "Inj1Injector Lua Test", MB_OK | MB_ICONINFORMATION);
        }
        break;
    case DLL_PROCESS_DETACH:
        OutputDebugStringA("[Inj1Injector] Lua DLL unloading!\n");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}


__declspec(dllexport) void TestLuaExecution()
{
    ExecuteLua("warn('Inj1Injector: Test function called!')");
}
