#include <windows.h>
#include <iostream>
#include <vector>
#include "../include/solara/injector.h"

typedef const char* (*GetSolaraVersionFn)();
typedef void        (*InitializeSolaraFn)();
typedef void        (*QueueScriptFn)(const char*);
typedef bool        (*IsSolaraHookedFn)();

int main() {
    // -----------------------------------------------------------------------
    // Load the Solara runtime DLL locally for version/queue access
    // -----------------------------------------------------------------------
    std::cout << "[TEST] Loading voltro2_runtime.dll..." << std::endl;
    char exePath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* slash = strrchr(exePath, '\\');
    if (slash) *slash = '\0';

    std::string dllPath = std::string(exePath) + "\\voltro2_runtime.dll";
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        dllPath = "voltro2_runtime.dll";
    }
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        dllPath = "build\\voltro2_runtime.dll";
    }

    HMODULE hMod = LoadLibraryA(dllPath.c_str());
    if (!hMod) {
        std::cerr << "[TEST FAILED] Could not load voltro2_runtime.dll (" << dllPath << "). Error: "
                  << GetLastError() << std::endl;
        return 1;
    }

    auto getVersion  = reinterpret_cast<GetSolaraVersionFn>(GetProcAddress(hMod, "GetSolaraVersion"));
    auto isAttached  = reinterpret_cast<bool(*)()>(GetProcAddress(hMod, "IsSolaraAttached"));

    if (!getVersion || !isAttached) {
        std::cerr << "[TEST FAILED] Missing export functions in voltro2_runtime.dll" << std::endl;
        FreeLibrary(hMod);
        return 1;
    }

    std::cout << "[TEST] Xeno Runtime Version: " << getVersion() << std::endl;
    std::cout << "[TEST] Attached Status     : " << (isAttached() ? "ACTIVE" : "STANDBY") << std::endl;

    // -----------------------------------------------------------------------
    // Process Scanner
    // -----------------------------------------------------------------------
    std::cout << "\n[TEST] Running Solara ProcessScanner for active Roblox targets..." << std::endl;
    auto targets = Solara::Injector::ProcessScanner::findRobloxTargets();
    std::cout << "[TEST] Detected Roblox processes: " << targets.size() << std::endl;
    for (const auto& t : targets) {
        std::wcout << L"  -> PID: " << t.pid << L" (" << t.processName
                   << L") 64-bit: " << (t.is64Bit ? L"YES" : L"NO") << std::endl;
    }

    if (!targets.empty()) {
        DWORD targetPid = targets[0].pid;

        // -------------------------------------------------------------------
        // [STRATEGY 0] Manual PE Map — Hyperion Bypass
        // -------------------------------------------------------------------
        std::cout << "\n=======================================================" << std::endl;
        std::cout << " [STRATEGY 0] Manual PE Map -> Roblox PID " << targetPid  << std::endl;
        std::cout << "=======================================================" << std::endl;

        // Resolve the DLL path
        char exeDir[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exeDir, MAX_PATH);
        if (char* sl = strrchr(exeDir, '\\')) *sl = '\0';

        std::string dllForMap = std::string(exeDir) + "\\voltro2_runtime.dll";
        if (GetFileAttributesA(dllForMap.c_str()) == INVALID_FILE_ATTRIBUTES)
            dllForMap = std::string(exeDir) + "\\build\\voltro2_runtime.dll";
        if (GetFileAttributesA(dllForMap.c_str()) == INVALID_FILE_ATTRIBUTES)
            dllForMap = "voltro_2\\build\\voltro2_runtime.dll";

        std::cout << "[MMAP] DLL source : " << dllForMap << std::endl;
        std::cout << "[MMAP] Starting manual PE map (no LoadLibraryA)..." << std::endl;

        std::string mmErr;
        bool mmOk = Solara::Injector::InjectDll(targetPid, dllForMap, mmErr);

        if (mmOk) {
            std::cout << "[INJECT] Injection SUCCEEDED!" << std::endl;
            if (!mmErr.empty())
                std::cout << "[INJECT] Note: " << mmErr << std::endl;
        } else {
            std::cout << "[INJECT] Injection failed: " << mmErr << std::endl;
        }

        // -------------------------------------------------------------------
        // Verify: PEB module list (manual map won't show here — that's correct)
        // -------------------------------------------------------------------
        bool isLoaded = Solara::Injector::IsModuleLoaded(targetPid, "voltro2_runtime.dll");
        std::cout << "[TEST] IsModuleLoaded (PEB): "
                  << (isLoaded ? "YES (LOADED in module list)"
                               : "NO  (expected for manual map — not in PEB Ldr)")
                  << std::endl;

        // -------------------------------------------------------------------
        // Verify: VoltroIPC pipe — DllMain starts it if DLL is live in Roblox
        // -------------------------------------------------------------------
        std::cout << "[TEST] Probing VoltroIPC pipe (DllMain signal)..." << std::endl;
        HANDLE hPipe = CreateFileW(L"\\\\.\\pipe\\VoltroIPC",
                                   GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hPipe != INVALID_HANDLE_VALUE) {
            std::cout << "[TEST] *** VoltroIPC CONNECTED — DLL is LIVE inside Roblox! ***" << std::endl;

            // Dispatch a real Luau script through the live in-process pipe
            const char* pipeScript = "print('[VOLTRO 2] Manual Map live execution confirmed!')";
            std::string payload = std::string("{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"") + pipeScript + "\"}";
            uint32_t len = static_cast<uint32_t>(payload.size());
            std::vector<BYTE> packet(sizeof(uint32_t) + len);
            memcpy(packet.data(), &len, sizeof(uint32_t));
            memcpy(packet.data() + sizeof(uint32_t), payload.data(), len);
            DWORD written = 0;
            WriteFile(hPipe, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr);
            std::cout << "[TEST] Script dispatched via VoltroIPC (" << written << " bytes)." << std::endl;
            CloseHandle(hPipe);
        } else {
            DWORD pipeErr = GetLastError();
            std::cout << "[TEST] VoltroIPC not open (Error " << pipeErr << ")." << std::endl;

            // Try Voltro2IPC fallback
            std::cout << "[TEST] Trying Voltro2IPC fallback pipe..." << std::endl;
            HANDLE hFb = CreateFileW(L"\\\\.\\pipe\\Voltro2IPC",
                                     GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hFb != INVALID_HANDLE_VALUE) {
                std::cout << "[TEST] Connected to fallback Voltro2IPC!" << std::endl;
                const char* fbScript = "print('[VOLTRO 2] Fallback pipe execution')";
                std::string payload = std::string("{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"") + fbScript + "\"}";
                uint32_t len = static_cast<uint32_t>(payload.size());
                std::vector<BYTE> packet(sizeof(uint32_t) + len);
                memcpy(packet.data(), &len, sizeof(uint32_t));
                memcpy(packet.data() + sizeof(uint32_t), payload.data(), len);
                DWORD written = 0;
                WriteFile(hFb, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr);
                std::cout << "[TEST] Dispatched fallback payload (" << written << " bytes)." << std::endl;
                CloseHandle(hFb);
            } else {
                std::cout << "[TEST] Voltro2IPC also not open (Error " << GetLastError() << ")." << std::endl;
            }
        }

        std::cout << "[TEST] Injection diagnostic complete." << std::endl;
    } else {
        std::cout << "[TEST] No active Roblox target found. Please launch Roblox and join a game first." << std::endl;
    }

    std::cout << "\n[TEST COMPLETE]" << std::endl;
    FreeLibrary(hMod);
    return 0;
}
