#include "solara_engine.h"
#include "voltro_runtime.h"
#include "roblox_offsets.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace Voltro {
namespace Runtime {

// Static pointer for the hooked hook callback
static SolaraExecutionEngine* g_solaraEngine = nullptr;

// -----------------------------------------------------------------------------
// SolaraExecutionEngine Implementation
// -----------------------------------------------------------------------------

SolaraExecutionEngine& SolaraExecutionEngine::instance() {
    static SolaraExecutionEngine inst;
    g_solaraEngine = &inst;
    return inst;
}

SolaraExecutionEngine::~SolaraExecutionEngine() {
    shutdown();
}

uintptr_t SolaraExecutionEngine::findTaskScheduler() {
    HMODULE hRoblox = GetModuleHandleA(NULL);
    if (!hRoblox) return 0;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hRoblox, &modInfo, sizeof(modInfo))) {
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t size = modInfo.SizeOfImage;

    // Method 1: RVA offset from community repo (Roblox-Offsets-Website, version c5aecda2245e4fae)
    uintptr_t tsPtrAddr = base + RobloxOffsets::Pointer::TaskScheduler;

    __try {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(tsPtrAddr), &mbi, sizeof(mbi)) &&
            (mbi.State == MEM_COMMIT) &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD)) {

            uintptr_t scheduler = *reinterpret_cast<uintptr_t*>(tsPtrAddr);
            if (scheduler) {
                MEMORY_BASIC_INFORMATION mbiSched;
                if (VirtualQuery(reinterpret_cast<LPCVOID>(scheduler), &mbiSched, sizeof(mbiSched)) &&
                    (mbiSched.State == MEM_COMMIT) &&
                    !(mbiSched.Protect & PAGE_NOACCESS)) {
                    return scheduler;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // RVA offset failed, fall through to pattern scan
    }

    // Method 2: Also try the other TaskScheduler pointer offset from offsets.hpp
    uintptr_t tsPtrAddr2 = base + 0x8B5CEE8;
    __try {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(tsPtrAddr2), &mbi, sizeof(mbi)) &&
            (mbi.State == MEM_COMMIT) &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD)) {

            uintptr_t scheduler = *reinterpret_cast<uintptr_t*>(tsPtrAddr2);
            if (scheduler) {
                MEMORY_BASIC_INFORMATION mbiSched;
                if (VirtualQuery(reinterpret_cast<LPCVOID>(scheduler), &mbiSched, sizeof(mbiSched)) &&
                    (mbiSched.State == MEM_COMMIT) &&
                    !(mbiSched.Protect & PAGE_NOACCESS)) {
                    return scheduler;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    // Method 3: Pattern scan fallback
    // 48 8B 0D ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 50
    const BYTE* memory = reinterpret_cast<const BYTE*>(base);
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
                for (size_t i = 0; i + 16 <= blockSize; ++i) {
                    if (block[i] == 0x48 && block[i+1] == 0x8B && block[i+2] == 0x0D &&
                        block[i+7] == 0x48 && block[i+8] == 0x85 && block[i+9] == 0xC9 &&
                        block[i+10] == 0x74 && block[i+12] == 0x48 && block[i+13] == 0x8B) {
                        
                        int32_t relOffset = *reinterpret_cast<const int32_t*>(block + i + 3);
                        uintptr_t schedulerPtrAddr = reinterpret_cast<uintptr_t>(block + i + 7 + relOffset);
                        
                        MEMORY_BASIC_INFORMATION targetMbi;
                        if (VirtualQuery(reinterpret_cast<LPCVOID>(schedulerPtrAddr), &targetMbi, sizeof(targetMbi)) &&
                            (targetMbi.State == MEM_COMMIT) &&
                            !(targetMbi.Protect & PAGE_NOACCESS) &&
                            !(targetMbi.Protect & PAGE_GUARD)) {
                            
                            uintptr_t scheduler = *reinterpret_cast<uintptr_t*>(schedulerPtrAddr);
                            if (scheduler && VirtualQuery(reinterpret_cast<LPCVOID>(scheduler), &targetMbi, sizeof(targetMbi)) &&
                                (targetMbi.State == MEM_COMMIT)) {
                                return scheduler;
                            }
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                // Page access exception handled safely
            }
        }

        current += mbi.RegionSize;
    }

    // Return 0 if not found — never return raw machine code offsets which crash on dereference
    return 0;
}

static void __fastcall HookedJobStep(void* thisPtr, void* stats) {
    if (g_solaraEngine) {
        g_solaraEngine->onJobStep();
    }
}

bool SolaraExecutionEngine::hookSchedulerJob(uintptr_t scheduler) {
    if (!scheduler) return false;

    // Validate scheduler pointer readability via SEH
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

        m_vftable = vftable;
        DWORD oldProtect = 0;
        if (VirtualProtect(&vftable[2], sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            m_originalJobStep = reinterpret_cast<JobStepFn>(vftable[2]);
            vftable[2] = reinterpret_cast<uintptr_t>(HookedJobStep);
            VirtualProtect(&vftable[2], sizeof(uintptr_t), oldProtect, &oldProtect);
            m_hooked = true;
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        m_hooked = false;
        return false;
    }

    return false;
}

bool SolaraExecutionEngine::initialize() {
    uintptr_t scheduler = findTaskScheduler();
    if (!scheduler) {
        // Scheduler not matched yet — Solara runtime stays safe without crashing
        return false;
    }

    return hookSchedulerJob(scheduler);
}

void SolaraExecutionEngine::shutdown() {
    m_running = false;
    m_hooked = false;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_scriptQueue.empty()) {
        m_scriptQueue.pop();
    }
}

bool SolaraExecutionEngine::queueScript(const std::string& script) {
    if (script.empty()) return false;

    // Lazy initialization: hook TaskScheduler on first script execution, not on DLL load
    if (!m_hooked.load()) {
        initialize();
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_scriptQueue.push(script);
    FramedIpcBridge::instance().sendOutput("[VOLTRO ENGINE] Enqueued script (" + std::to_string(script.length()) + " bytes) into Roblox TaskScheduler step cycle.", "info");
    return true;
}

static void SafeElevateCapabilities(lua_State_t L) {
    if (!L) return;
    __try {
        uintptr_t extraSpaceAddr = reinterpret_cast<uintptr_t>(L) - 0x78;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(extraSpaceAddr), &mbi, sizeof(mbi)) &&
            (mbi.State == MEM_COMMIT) &&
            (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) {
            
            auto* extra = reinterpret_cast<RobloxExtraSpace*>(extraSpaceAddr);
            extra->capabilities = CAPABILITIES_ALL; // Elevate all privileges
            extra->identity = 8;                   // Identity 8
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Trapped safely
    }
}

static bool SafeCallOriginalStep(SolaraExecutionEngine::JobStepFn fn, void* targetJob) {
    if (!fn || !targetJob) return false;
    __try {
        fn(targetJob, nullptr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void SolaraExecutionEngine::setupLuauEnvironment(lua_State_t L) {
    SafeElevateCapabilities(L);
}

void SolaraExecutionEngine::onJobStep() {
    std::string scriptToExec;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_scriptQueue.empty()) {
            scriptToExec = m_scriptQueue.front();
            m_scriptQueue.pop();
        }
    }

    if (!scriptToExec.empty()) {
        FramedIpcBridge::instance().sendOutput("[TASK SCHEDULER] Executing on Roblox main thread step...", "ok");

        // Execute via LuaHookEngine which has the resolved luaL_dostring + lua_State
        auto& engine = LuaHookEngine::instance();
        if (engine.isHooked() && engine.getState()) {
            bool success = engine.executeScript(scriptToExec);
            if (success) {
                FramedIpcBridge::instance().sendOutput("[SOLARA ENGINE] Script executed via Lua VM (" + std::to_string(scriptToExec.length()) + " bytes).", "ok");
            } else {
                FramedIpcBridge::instance().sendOutput("[SOLARA ENGINE] Script execution failed — LuaVM not ready or script blocked.", "error");
            }
        } else {
            // Fallback: queue will be picked up next cycle when engine is ready
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_scriptQueue.push(scriptToExec);
            FramedIpcBridge::instance().sendOutput("[SOLARA ENGINE] LuaVM not hooked yet — script re-queued.", "warn");
        }
    }

    // Call original step protected by SEH so Roblox main game loop never crashes
    if (m_originalJobStep && m_targetJob) {
        SafeCallOriginalStep(m_originalJobStep, reinterpret_cast<void*>(m_targetJob));
    }
}

// -----------------------------------------------------------------------------
// FramedIpcBridge Implementation (4-byte length-prefix framing)
// -----------------------------------------------------------------------------

FramedIpcBridge& FramedIpcBridge::instance() {
    static FramedIpcBridge inst;
    return inst;
}

FramedIpcBridge::~FramedIpcBridge() {
    stop();
}

bool FramedIpcBridge::start() {
    stop();
    m_running = true;
    m_workerThread = std::thread(&FramedIpcBridge::communicationLoop, this);
    return true;
}

void FramedIpcBridge::stop() {
    m_running = false;
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }
}

void FramedIpcBridge::setScriptHandler(std::function<void(const std::string&)> handler) {
    m_scriptHandler = handler;
}

bool FramedIpcBridge::readExact(HANDLE pipe, void* buffer, DWORD bytesToRead) {
    DWORD totalRead = 0;
    BYTE* ptr = reinterpret_cast<BYTE*>(buffer);

    while (totalRead < bytesToRead) {
        DWORD read = 0;
        BOOL ok = ReadFile(pipe, ptr + totalRead, bytesToRead - totalRead, &read, nullptr);
        if (!ok || read == 0) {
            return false;
        }
        totalRead += read;
    }
    return true;
}

bool FramedIpcBridge::sendPacket(const std::string& payload) {
    if (m_pipe == INVALID_HANDLE_VALUE) return false;

    uint32_t length = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> packet(sizeof(uint32_t) + length);
    memcpy(packet.data(), &length, sizeof(uint32_t));
    memcpy(packet.data() + sizeof(uint32_t), payload.data(), length);

    DWORD written = 0;
    return WriteFile(m_pipe, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr) != FALSE;
}

bool FramedIpcBridge::sendOutput(const std::string& text, const std::string& level) {
    std::string escaped;
    for (char c : text) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"OUTPUT\",\"text\":\"" + escaped + "\",\"level\":\"" + level + "\"}";
    return sendPacket(payload);
}

bool FramedIpcBridge::sendStatus(bool attached, const std::string& message) {
    std::string payload = "{\"type\":\"ATTACH_STATUS\",\"attached\":" + std::string(attached ? "true" : "false") + ",\"message\":\"" + message + "\"}";
    return sendPacket(payload);
}

void FramedIpcBridge::communicationLoop() {
    const wchar_t* pipeName = L"\\\\.\\pipe\\VoltroIPC";

    while (m_running) {
        m_pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (m_pipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        sendStatus(true, "Voltro Solara-Backend attached via Framed IPC.");
        sendOutput("[IPC] Connected to Voltro Host (Framed Protocol Active).", "ok");

        while (m_running) {
            // Read 4-byte length prefix
            uint32_t payloadLength = 0;
            if (!readExact(m_pipe, &payloadLength, sizeof(uint32_t))) {
                break; // Disconnected
            }

            if (payloadLength == 0 || payloadLength > 100 * 1024 * 1024) { // 100 MB max guard
                break;
            }

            std::string payload(payloadLength, '\0');
            if (!readExact(m_pipe, &payload[0], payloadLength)) {
                break;
            }

            // Route payload
            if (m_scriptHandler) {
                m_scriptHandler(payload);
            } else {
                SolaraExecutionEngine::instance().queueScript(payload);
            }
        }

        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace Runtime
} // namespace Voltro
