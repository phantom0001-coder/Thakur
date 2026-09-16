#include "thakur_engine.h"
#include "thakur_runtime.h"
#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>


namespace Thakur {
namespace Runtime {

static ThakurExecutionEngine* g_thakurEngine = nullptr;

ThakurExecutionEngine& ThakurExecutionEngine::instance() {
    static ThakurExecutionEngine inst;
    g_thakurEngine = &inst;
    return inst;
}

ThakurExecutionEngine::~ThakurExecutionEngine() {
    shutdown();
}

uintptr_t ThakurExecutionEngine::findTaskScheduler() {
    
    return scanTaskSchedulerWorker();
}

uintptr_t ThakurExecutionEngine::scanTaskSchedulerWorker() {
    HMODULE hRoblox = GetModuleHandleA(NULL);
    if (!hRoblox) return 0;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hRoblox, &modInfo, sizeof(modInfo))) {
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t size = modInfo.SizeOfImage;

    
    
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
                
            }
        }

        current += mbi.RegionSize;
    }

    
    return 0;
}

static void __fastcall HookedJobStep(void* thisPtr, void* stats) {
    if (g_thakurEngine) {
        g_thakurEngine->onJobStep();
    }
}

bool ThakurExecutionEngine::hookSchedulerJob(uintptr_t scheduler) {
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

bool ThakurExecutionEngine::initialize() {
    if (m_hooked) {
        return true;
    }
    
    uintptr_t scheduler = findTaskScheduler();
    if (!scheduler) {
        FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Failed to locate TaskScheduler instance.", "error");
        m_hooked = false;
        return false;
    }
    
    m_scriptContext = scheduler;
    
    if (!hookSchedulerJob(scheduler)) {
        FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Failed to hook TaskScheduler job step.", "error");
        m_hooked = false;
        return false;
    }
    
    if (LuaHookEngine::instance().isHooked()) {
        m_luaState = LuaHookEngine::instance().getState();
        FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Lua state acquired from LuaHookEngine.", "ok");
    }
    
    m_running = true;
    FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Successfully hooked into Roblox TaskScheduler.", "ok");
    return true;
}

void ThakurExecutionEngine::shutdown() {
    m_running = false;
    m_hooked = false;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_scriptQueue.empty()) {
        m_scriptQueue.pop();
    }
}

bool ThakurExecutionEngine::queueScript(const std::string& script) {
    if (script.empty()) return false;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_scriptQueue.push(script);
    FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Enqueued script (" + std::to_string(script.length()) + " bytes) into Roblox TaskScheduler step cycle.", "info");
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
            extra->capabilities = CAPABILITIES_ALL; 
            extra->identity = 8;                   
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }
}

static bool SafeCallOriginalStep(ThakurExecutionEngine::JobStepFn fn, void* targetJob) {
    if (!fn || !targetJob) return false;
    __try {
        fn(targetJob, nullptr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void ThakurExecutionEngine::setupLuauEnvironment(lua_State_t L) {
    SafeElevateCapabilities(L);
}

void ThakurExecutionEngine::onJobStep() {
    if (!m_running) {
        if (m_originalJobStep && m_targetJob) {
            SafeCallOriginalStep(m_originalJobStep, reinterpret_cast<void*>(m_targetJob));
        }
        return;
    }

    std::string scriptToExec;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_scriptQueue.empty()) {
            scriptToExec = m_scriptQueue.front();
            m_scriptQueue.pop();
        }
    }

    if (!scriptToExec.empty() && m_luaState) {
        FramedIpcBridge::instance().sendOutput("[TASK SCHEDULER] Executing on Roblox main thread step...", "info");
        
        __try {
            uintptr_t extraSpaceAddr = reinterpret_cast<uintptr_t>(m_luaState) - 0x78;
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(extraSpaceAddr), &mbi, sizeof(mbi)) &&
                (mbi.State == MEM_COMMIT) &&
                (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) {
                
                auto* extra = reinterpret_cast<RobloxExtraSpace*>(extraSpaceAddr);
                extra->capabilities = CAPABILITIES_ALL;
                extra->identity = 8;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        
        luaL_dostring_t doString = nullptr;
        if (LuaHookEngine::instance().isHooked()) {
            doString = reinterpret_cast<luaL_dostring_t>(LuaHookEngine::instance().getDoStringPtr());
        }
        
        if (doString) {
            bool fault = false;
            int status = SafeCallDoString(doString, m_luaState, scriptToExec.c_str(), fault);
            if (fault) {
                FramedIpcBridge::instance().sendOutput("[EXCEPTION] Crash guard intercepted memory fault during execution.", "error");
            } else if (status == 0) {
                FramedIpcBridge::instance().sendOutput("[THAKUR ENGINE] Executed script block cleanly (" + std::to_string(scriptToExec.length()) + " bytes).", "ok");
            } else {
                FramedIpcBridge::instance().sendOutput("[LUA ERROR] Execution returned status: " + std::to_string(status), "error");
            }
        } else {
            FramedIpcBridge::instance().sendOutput("[WARN] LuaHookEngine not ready - using fallback execution path.", "warn");
        }
    }

    
    if (m_originalJobStep && m_targetJob) {
        SafeCallOriginalStep(m_originalJobStep, reinterpret_cast<void*>(m_targetJob));
    }
}

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
    const wchar_t* pipeName = L"\\\\.\\pipe\\ThakurIPC";

    while (m_running) {
        m_pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (m_pipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        sendStatus(true, "Thakur Thakur-Backend attached via Framed IPC.");
        sendOutput("[IPC] Connected to Thakur Host (Framed Protocol Active).", "ok");

        while (m_running) {
            
            uint32_t payloadLength = 0;
            if (!readExact(m_pipe, &payloadLength, sizeof(uint32_t))) {
                break; 
            }

            if (payloadLength == 0 || payloadLength > 100 * 1024 * 1024) { 
                break;
            }

            std::string payload(payloadLength, '\0');
            if (!readExact(m_pipe, &payload[0], payloadLength)) {
                break;
            }

            
            if (m_scriptHandler) {
                m_scriptHandler(payload);
            } else {
                ThakurExecutionEngine::instance().queueScript(payload);
            }
        }

        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} 
} 
