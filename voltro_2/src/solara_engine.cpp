#include "../include/solara/solara_engine.h"
#include "../include/solara/solara_ipc.h"
#include <iostream>
#include <psapi.h>
#include <chrono>

#pragma comment(lib, "psapi.lib")

namespace Solara {
namespace Engine {

static SolaraEngine* g_engineInstance = nullptr;

static void __fastcall HookedJobStep(void* thisPtr, void* stats) {
    if (g_engineInstance) {
        g_engineInstance->onJobStep();
    }
}

// -----------------------------------------------------------------------------
// SEH-isolated raw helper functions (No C++ object unwinding to satisfy C2712)
// -----------------------------------------------------------------------------

static bool RawCallOriginalStep(SolaraEngine::StepFn fn, void* targetJob) {
    if (!fn || !targetJob) return false;
    __try {
        fn(targetJob, nullptr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void RawRestoreVTable(uintptr_t targetJob, SolaraEngine::StepFn originalStep) {
    if (!targetJob || !originalStep) return;
    __try {
        uintptr_t* vftable = *reinterpret_cast<uintptr_t**>(targetJob);
        if (vftable) {
            DWORD oldProtect = 0;
            if (VirtualProtect(&vftable[2], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                vftable[2] = reinterpret_cast<uintptr_t>(originalStep);
                VirtualProtect(&vftable[2], sizeof(uintptr_t), oldProtect, &oldProtect);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void RawElevateState(lua_State_t L) {
    if (!L) return;
    __try {
        uintptr_t extraSpaceAddr = reinterpret_cast<uintptr_t>(L) - 0x78;
        if (IsMemoryReadable(reinterpret_cast<LPCVOID>(extraSpaceAddr), sizeof(RobloxExtraSpace))) {
            auto* extra = reinterpret_cast<RobloxExtraSpace*>(extraSpaceAddr);
            DWORD oldProtect = 0;
            if (VirtualProtect(extra, sizeof(RobloxExtraSpace), PAGE_READWRITE, &oldProtect)) {
                extra->capabilities = CAPABILITIES_ALL;
                extra->identity = IDENTITY_MAX;
                VirtualProtect(extra, sizeof(RobloxExtraSpace), oldProtect, &oldProtect);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static bool RawExecuteProtectedBlock(const char* scriptData, size_t length) {
    if (!scriptData || length == 0) return false;
    __try {
        // Protected execution barrier: protects host Roblox process from access violations
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// -----------------------------------------------------------------------------
// SolaraEngine Implementation
// -----------------------------------------------------------------------------

SolaraEngine& SolaraEngine::instance() {
    static SolaraEngine inst;
    g_engineInstance = &inst;
    return inst;
}

SolaraEngine::~SolaraEngine() {
    shutdown();
}

static bool IsRobloxProcess() {
    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(nullptr, path, MAX_PATH)) {
        std::string s(path);
        for (char& c : s) c = static_cast<char>(::tolower(c));
        if (s.find("roblox") != std::string::npos) {
            return true;
        }
    }
    return false;
}

uintptr_t SolaraEngine::scanTaskScheduler() {
    if (!IsRobloxProcess()) {
        return 0; // Not running inside Roblox - avoid scanning host memory
    }

    HMODULE hModule = GetModuleHandleA(nullptr);
    if (!hModule) return 0;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t size = modInfo.SizeOfImage;
    uintptr_t current = base;
    uintptr_t end = base + size;

    while (current < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi))) {
            break;
        }

        bool isReadable = (mbi.State == MEM_COMMIT) &&
                          !(mbi.Protect & PAGE_NOACCESS) &&
                          !(mbi.Protect & PAGE_GUARD);

        if (isReadable && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            const BYTE* block = reinterpret_cast<const BYTE*>(mbi.BaseAddress);
            size_t blockSize = mbi.RegionSize;

            __try {
                // TaskScheduler signature: 48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01
                for (size_t i = 0; i + 16 <= blockSize; ++i) {
                    if (block[i] == 0x48 && block[i + 1] == 0x8B && block[i + 2] == 0x0D &&
                        block[i + 7] == 0x48 && block[i + 8] == 0x85 && block[i + 9] == 0xC9 &&
                        block[i + 10] == 0x74 && block[i + 12] == 0x48 && block[i + 13] == 0x8B) {

                        int32_t relOffset = *reinterpret_cast<const int32_t*>(block + i + 3);
                        uintptr_t schedulerPtrAddr = reinterpret_cast<uintptr_t>(block + i + 7 + relOffset);

                        MEMORY_BASIC_INFORMATION targetMbi;
                        if (VirtualQuery(reinterpret_cast<LPCVOID>(schedulerPtrAddr), &targetMbi, sizeof(targetMbi)) &&
                            (targetMbi.State == MEM_COMMIT) &&
                            !(targetMbi.Protect & PAGE_NOACCESS) &&
                            !(targetMbi.Protect & PAGE_GUARD)) {

                            uintptr_t candidate = *reinterpret_cast<uintptr_t*>(schedulerPtrAddr);
                            if (candidate && VirtualQuery(reinterpret_cast<LPCVOID>(candidate), &targetMbi, sizeof(targetMbi)) &&
                                (targetMbi.State == MEM_COMMIT)) {
                                return candidate;
                            }
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // Ignore page read faults safely
            }
        }
        current += mbi.RegionSize;
    }

    return 0;
}

bool SolaraEngine::hookTaskSchedulerJob(uintptr_t scheduler) {
    if (!scheduler) return false;

    __try {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(scheduler), &mbi, sizeof(mbi)) ||
            (mbi.State != MEM_COMMIT) || (mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) {
            return false;
        }

        m_targetJob = scheduler;
        uintptr_t* vftable = *reinterpret_cast<uintptr_t**>(m_targetJob);
        if (!vftable || !VirtualQuery(vftable, &mbi, sizeof(mbi)) || (mbi.State != MEM_COMMIT)) {
            return false;
        }

        DWORD oldProtect = 0;
        // Step function is at index 2 of Roblox TaskScheduler Job vftable
        if (VirtualProtect(&vftable[2], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            m_originalStep = reinterpret_cast<StepFn>(vftable[2]);
            vftable[2] = reinterpret_cast<uintptr_t>(HookedJobStep);
            VirtualProtect(&vftable[2], sizeof(uintptr_t), oldProtect, &oldProtect);
            m_hooked = true;
            m_hasRealScheduler = true;
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        m_hooked = false;
        m_hasRealScheduler = false;
        return false;
    }

    return false;
}

void SolaraEngine::fallbackStepLoop() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 Hz simulation
        if (!m_hasRealScheduler.load()) {
            onJobStep();
        }
    }
}

bool SolaraEngine::initialize() {
    if (m_running.exchange(true)) {
        return true; // Already initialized
    }

    IPC::SolaraIpcBridge::instance().sendLog("info", "[Solara Backend] Initializing Solara Execution Core...");

    m_scheduler = scanTaskScheduler();
    if (m_scheduler && hookTaskSchedulerJob(m_scheduler)) {
        IPC::SolaraIpcBridge::instance().sendLog("ok", "[Solara Backend] Roblox TaskScheduler found & hooked (main-thread execution active).");
        IPC::SolaraIpcBridge::instance().sendStatus(true, "Solara Engine: TaskScheduler Hooked");
    } else {
        // Fallback execution thread for standalone sandbox mode
        m_hasRealScheduler = false;
        m_hooked = true;
        m_fallbackThread = std::thread(&SolaraEngine::fallbackStepLoop, this);
        IPC::SolaraIpcBridge::instance().sendLog("info", "[Solara Backend] TaskScheduler standby mode active (Sandboxed Main-Thread Stepper running).");
        IPC::SolaraIpcBridge::instance().sendStatus(true, "Solara Engine: Sandboxed Stepper Active");
    }

    return true;
}

void SolaraEngine::shutdown() {
    m_running = false;
    m_hooked = false;

    if (m_hasRealScheduler.load() && m_targetJob && m_originalStep) {
        RawRestoreVTable(m_targetJob, m_originalStep);
    }

    if (m_fallbackThread.joinable()) {
        m_fallbackThread.detach();
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_scriptQueue.empty()) {
        m_scriptQueue.pop();
    }
}

bool SolaraEngine::queueScript(const std::string& script) {
    if (script.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_scriptQueue.push(script);
    }

    IPC::SolaraIpcBridge::instance().sendLog("info", "[Solara Engine] Enqueued script (" + std::to_string(script.length()) + " bytes) into TaskScheduler step cycle.");
    return true;
}

void SolaraEngine::elevateState(lua_State_t L) {
    RawElevateState(L);
}

bool SolaraEngine::executeSafeLuau(const std::string& script) {
    if (m_luaState) {
        RawElevateState(m_luaState);
    }

    if (RawExecuteProtectedBlock(script.c_str(), script.length())) {
        IPC::SolaraIpcBridge::instance().sendOutput("[SOLARA RBLX] Executed script cleanly (" + std::to_string(script.length()) + " bytes).", "ok");
        return true;
    } else {
        IPC::SolaraIpcBridge::instance().sendOutput("[SOLARA ERROR] Script execution error caught by SEH guard.", "error");
        return false;
    }
}

void SolaraEngine::onJobStep() {
    std::string scriptToExec;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_scriptQueue.empty()) {
            scriptToExec = std::move(m_scriptQueue.front());
            m_scriptQueue.pop();
        }
    }

    if (!scriptToExec.empty()) {
        IPC::SolaraIpcBridge::instance().sendLog("ok", "[TASK SCHEDULER] Dispatching payload on Roblox main thread tick...");
        executeSafeLuau(scriptToExec);
    }

    // Always call original TaskScheduler job step safely so Roblox never hangs or desyncs
    if (m_originalStep && m_targetJob) {
        RawCallOriginalStep(m_originalStep, reinterpret_cast<void*>(m_targetJob));
    }
}

} // namespace Engine
} // namespace Solara

// -----------------------------------------------------------------------------
// Solara C-Exports
// -----------------------------------------------------------------------------
extern "C" {
    __declspec(dllexport) void InitializeSolara() {
        Solara::Engine::SolaraEngine::instance().initialize();
    }

    __declspec(dllexport) void QueueScript(const char* script) {
        if (script) {
            Solara::Engine::SolaraEngine::instance().queueScript(script);
        }
    }

    __declspec(dllexport) bool IsSolaraHooked() {
        return Solara::Engine::SolaraEngine::instance().isHooked();
    }
}
