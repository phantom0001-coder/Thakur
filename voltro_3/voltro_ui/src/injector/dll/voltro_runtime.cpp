#include "voltro_runtime.h"
#include "roblox_offsets.h"
#include "solara_engine.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace Voltro {
namespace Runtime {

// -----------------------------------------------------------------------------
// LuaHookEngine Implementation
// -----------------------------------------------------------------------------

LuaHookEngine& LuaHookEngine::instance() {
    static LuaHookEngine inst;
    return inst;
}

LuaHookEngine::~LuaHookEngine() {
    stopExecution();
}

static void ParsePatternToRaw(const std::string& pattern, std::vector<BYTE>& bytes, std::vector<BYTE>& mask) {
    bytes.clear();
    mask.clear();
    std::stringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0);
            mask.push_back(0x00);
        } else {
            bytes.push_back(static_cast<BYTE>(std::stoul(token, nullptr, 16)));
            mask.push_back(0xFF);
        }
    }
}

static uintptr_t SafeScanRegion(uintptr_t start, size_t size, const BYTE* patBytes, const BYTE* patMask, size_t patLen) {
    if (!patBytes || !patMask || patLen == 0 || size < patLen) return 0;

    uintptr_t current = start;
    uintptr_t end = start + size;

    while (current < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)))
            break;

        bool readable = (mbi.State == MEM_COMMIT) &&
                        !(mbi.Protect & PAGE_NOACCESS) &&
                        !(mbi.Protect & PAGE_GUARD) &&
                        (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
                                        PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));

        if (readable) {
            uintptr_t blockStart = current;
            uintptr_t blockEnd = current + mbi.RegionSize;
            if (blockEnd > end) blockEnd = end;

            size_t blockAvail = blockEnd - blockStart;
            if (blockAvail >= patLen) {
                uintptr_t result = 0;
                __try {
                    const BYTE* mem = reinterpret_cast<const BYTE*>(blockStart);
                    size_t scanLen = blockAvail - patLen;
                    for (size_t i = 0; i <= scanLen; ++i) {
                        bool match = true;
                        for (size_t j = 0; j < patLen; ++j) {
                            if (patMask[j] == 0xFF && mem[i + j] != patBytes[j]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            result = blockStart + i;
                            break;
                        }
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    result = 0;
                }
                if (result) return result;
            }
        }

        current += mbi.RegionSize;
    }
    return 0;
}

static uintptr_t ResolveRipRelative32(uintptr_t matchAddr, int dispOffset, int instrSize) {
    uintptr_t result = 0;
    __try {
        int32_t rel = *reinterpret_cast<const int32_t*>(matchAddr + dispOffset);
        uintptr_t target = matchAddr + instrSize + rel;

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(target), &mbi, sizeof(mbi)) &&
            (mbi.State == MEM_COMMIT) &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD)) {
            result = target;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = 0;
    }
    return result;
}

static uintptr_t SafeReadPtrUintptr(uintptr_t addr) {
    uintptr_t result = 0;
    __try {
        result = *reinterpret_cast<volatile uintptr_t*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = 0;
    }
    return result;
}

static lua_State_t SafeReadPtrLuaState(uintptr_t addr) {
    lua_State_t result = nullptr;
    __try {
        result = *reinterpret_cast<volatile lua_State_t*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = nullptr;
    }
    return result;
}

static bool ValidateLuaStateCandidate(lua_State_t candidate) {
    if (!candidate) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(candidate), &mbi, sizeof(mbi)))
        return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;

    uintptr_t firstField = SafeReadPtrUintptr(reinterpret_cast<uintptr_t>(candidate));
    if (!firstField) return false;

    MEMORY_BASIC_INFORMATION mbi2;
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(firstField), &mbi2, sizeof(mbi2)))
        return false;
    return (mbi2.State == MEM_COMMIT) && !(mbi2.Protect & PAGE_NOACCESS);
}

// All scan logic in one static function — no __try here, only in helpers above
static bool RunSignatureScan(uintptr_t baseAddr, size_t imageSize, luaL_dostring_t& outDoString, lua_State_t& outState) {
    // --- Find luaL_dostring via pattern ---
    if (!outDoString) {
        const char* dostringPats[] = {
            "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9",
            "48 89 4C 24 ? 55 53 56 57 41 54",
            "48 89 5C 24 ? 48 89 74 24 ?",
            "55 48 8B EC 48 83 EC 20",
            "57 48 83 EC 20 48 8B F9",
        };

        for (const char* pat : dostringPats) {
            std::vector<BYTE> pb, pm;
            ParsePatternToRaw(pat, pb, pm);
            uintptr_t found = SafeScanRegion(baseAddr, imageSize, pb.data(), pm.data(), pb.size());
            if (found) {
                outDoString = reinterpret_cast<luaL_dostring_t>(found);
                break;
            }
        }
    }

    // --- Find lua_State via pattern ---
    if (!outState) {
        const char* statePats[] = {
            "48 8D 05 ? ? ? ? 48 8B D9 48 8B 00",
            "48 8B 05 ? ? ? ? 48 89 44 24",
            "4C 8D 05 ? ? ? ? 49 8B",
            "48 8B 0D ? ? ? ? 48 89",
            "4C 8B 05 ? ? ? ? 48 89",
            "48 8B 15 ? ? ? ? 48 85 D2",
            "48 89 05 ? ? ? ? 48 89",
            "48 8B 35 ? ? ? ? 48 85 F6",
        };

        for (const char* pat : statePats) {
            std::vector<BYTE> pb, pm;
            ParsePatternToRaw(pat, pb, pm);

            uintptr_t matchAddr = SafeScanRegion(baseAddr, imageSize, pb.data(), pm.data(), pb.size());
            if (!matchAddr) continue;

            uintptr_t targetPtr = ResolveRipRelative32(matchAddr, 3, 7);
            if (!targetPtr) continue;

            lua_State_t candidate = SafeReadPtrLuaState(targetPtr);
            if (ValidateLuaStateCandidate(candidate)) {
                outState = candidate;
                break;
            }
        }
    }

    return (outDoString != nullptr && outState != nullptr);
}

bool LuaHookEngine::scanSignatures() {
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) return false;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return false;

    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t imageSize = modInfo.SizeOfImage;

    bool ok = RunSignatureScan(baseAddr, imageSize, m_pfnDoString, m_luaState);
    m_hooked = ok;
    return ok;
}

bool LuaHookEngine::validateScriptSafety(const std::string& script, std::string& outReason) {
    std::string lower = script;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    const std::vector<std::string> blocked = {
        "game:shutdown",
        "game.shutdown",
        "os.execute",
        "io.open",
        "io.popen",
        "debug.sethook",
        "ffi.load",
        "ffi.cdef"
    };

    for (const auto& func : blocked) {
        if (lower.find(func) != std::string::npos) {
            outReason = "Blocked dangerous Lua call: " + func;
            return false;
        }
    }
    return true;
}

// SEH-protected static helpers (cannot use __try in C++ member functions)
static bool ResolveRvaDoString(uintptr_t base, luaL_dostring_t& outDoString) {
    outDoString = nullptr;
    uintptr_t addr = base + RobloxOffsets::Engine::LuaVM_Load;

    __try {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) &&
            (mbi.State == MEM_COMMIT) &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD)) {

            BYTE* code = reinterpret_cast<BYTE*>(addr);
            if (code[0] != 0x00 && code[0] != 0xCC && code[0] != 0xC3) {
                outDoString = reinterpret_cast<luaL_dostring_t>(addr);
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        outDoString = nullptr;
    }
    return outDoString != nullptr;
}

static bool ResolveLuaStateViaTaskScheduler(uintptr_t base, lua_State_t& outState) {
    outState = nullptr;

    // Read the global TaskScheduler pointer
    uintptr_t tsPtrAddr = base + RobloxOffsets::Pointer::TaskScheduler;
    __try {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(tsPtrAddr), &mbi, sizeof(mbi)) ||
            (mbi.State != MEM_COMMIT) ||
            (mbi.Protect & PAGE_NOACCESS) ||
            (mbi.Protect & PAGE_GUARD)) {
            return false;
        }

        uintptr_t scheduler = *reinterpret_cast<volatile uintptr_t*>(tsPtrAddr);
        if (!scheduler) return false;

        // Validate TaskScheduler is readable
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(scheduler), &mbi, sizeof(mbi)) ||
            (mbi.State != MEM_COMMIT) ||
            (mbi.Protect & PAGE_NOACCESS)) {
            return false;
        }

        // Walk the TaskScheduler's job list
        // JobStart and JobEnd offsets from structured offsets (version-9d412f44a6fe4081)
        // These offsets define the range of the jobs vector
        constexpr uintptr_t JOB_START_OFFSET = 0xC8;
        constexpr uintptr_t JOB_END_OFFSET = 0xD0;
        constexpr uintptr_t JOB_VFTABLE_OFFSET = 0x00;
        constexpr uintptr_t JOB_SIZE = 0x48;  // approximate size per job entry

        uintptr_t jobStart = 0, jobEnd = 0;
        __try {
            jobStart = *reinterpret_cast<volatile uintptr_t*>(scheduler + JOB_START_OFFSET);
            jobEnd = *reinterpret_cast<volatile uintptr_t*>(scheduler + JOB_END_OFFSET);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        if (!jobStart || !jobEnd || jobEnd <= jobStart || (jobEnd - jobStart) > 0x100000) {
            return false;
        }

        // Iterate through jobs looking for one with a lua_State-like pointer
        // Each job is a structure; scan for readable pointer fields that look like lua_State
        uintptr_t numJobs = (jobEnd - jobStart) / JOB_SIZE;
        if (numJobs > 1000) numJobs = 1000;  // sanity limit

        for (uintptr_t i = 0; i < numJobs; ++i) {
            uintptr_t jobAddr = jobStart + i * JOB_SIZE;

            MEMORY_BASIC_INFORMATION mbiJob;
            if (!VirtualQuery(reinterpret_cast<LPCVOID>(jobAddr), &mbiJob, sizeof(mbiJob)))
                continue;
            if (mbiJob.State != MEM_COMMIT || (mbiJob.Protect & PAGE_NOACCESS))
                continue;

            __try {
                // Read vftable of this job
                uintptr_t vftable = *reinterpret_cast<volatile uintptr_t*>(jobAddr + JOB_VFTABLE_OFFSET);
                if (!vftable) continue;

                // Scan job structure for a pointer that looks like a lua_State
                // lua_State is a heap allocation, typically aligned, in committed memory
                for (uintptr_t off = 0x08; off < JOB_SIZE; off += sizeof(uintptr_t)) {
                    uintptr_t field = *reinterpret_cast<volatile uintptr_t*>(jobAddr + off);
                    if (!field) continue;

                    MEMORY_BASIC_INFORMATION mbiField;
                    if (!VirtualQuery(reinterpret_cast<LPCVOID>(field), &mbiField, sizeof(mbiField)))
                        continue;
                    if (mbiField.State != MEM_COMMIT || (mbiField.Protect & PAGE_NOACCESS))
                        continue;

                    // Check if this looks like a lua_State:
                    // First field should be a valid pointer (to global_State or similar)
                    uintptr_t inner = *reinterpret_cast<volatile uintptr_t*>(field);
                    if (!inner) continue;

                    MEMORY_BASIC_INFORMATION mbiInner;
                    if (VirtualQuery(reinterpret_cast<LPCVOID>(inner), &mbiInner, sizeof(mbiInner)) &&
                        (mbiInner.State == MEM_COMMIT) &&
                        !(mbiInner.Protect & PAGE_NOACCESS)) {

                        outState = reinterpret_cast<lua_State_t>(field);
                        return true;
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    return false;
}

bool LuaHookEngine::resolveOffsets() {
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) return false;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return false;

    uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    char buf[256];

    // Step 1: RVA offset for luaL_dostring
    sprintf_s(buf, "[OFFSET ENGINE] Trying RVA luaL_dostring at base+%p...", (void*)RobloxOffsets::Engine::LuaVM_Load);
    FramedIpcBridge::instance().sendOutput(buf, "info");

    if (ResolveRvaDoString(base, m_pfnDoString)) {
        sprintf_s(buf, "[OFFSET ENGINE] luaL_dostring = %p (via RVA)", (void*)m_pfnDoString);
        FramedIpcBridge::instance().sendOutput(buf, "ok");
    } else {
        FramedIpcBridge::instance().sendOutput("[OFFSET ENGINE] RVA luaL_dostring failed.", "warn");
    }

    // Step 2: Find lua_State via TaskScheduler RVA
    sprintf_s(buf, "[OFFSET ENGINE] Trying TaskScheduler RVA at base+%p...", (void*)RobloxOffsets::Pointer::TaskScheduler);
    FramedIpcBridge::instance().sendOutput(buf, "info");

    if (ResolveLuaStateViaTaskScheduler(base, m_luaState)) {
        sprintf_s(buf, "[OFFSET ENGINE] lua_State = %p (via TaskScheduler walk)", (void*)m_luaState);
        FramedIpcBridge::instance().sendOutput(buf, "ok");
    } else {
        FramedIpcBridge::instance().sendOutput("[OFFSET ENGINE] TaskScheduler walk failed. Trying pattern scan...", "warn");
    }

    // Step 3: Pattern scan fallback for anything still missing
    if (!m_pfnDoString || !m_luaState) {
        FramedIpcBridge::instance().sendOutput("[OFFSET ENGINE] Running pattern scan for missing pointers...", "info");
        scanSignatures();
    }

    // Final result
    if (m_pfnDoString && m_luaState) {
        sprintf_s(buf, "[OFFSET ENGINE] READY: dostring=%p, lua_State=%p", (void*)m_pfnDoString, (void*)m_luaState);
        FramedIpcBridge::instance().sendOutput(buf, "ok");
        m_hooked = true;
        return true;
    }

    sprintf_s(buf, "[OFFSET ENGINE] FAILED: dostring=%s, lua_State=%s",
        m_pfnDoString ? "OK" : "NULL", m_luaState ? "OK" : "NULL");
    FramedIpcBridge::instance().sendOutput(buf, "error");

    m_pfnDoString = nullptr;
    m_luaState = nullptr;
    m_hooked = false;
    return false;
}

bool LuaHookEngine::initialize() {
    if (IsDebuggerPresent()) {
        FramedIpcBridge::instance().sendOutput("[SECURITY] Debugger detected via IsDebuggerPresent", "warn");
    }

    BOOL remoteDebugger = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebugger);
    if (remoteDebugger) {
        FramedIpcBridge::instance().sendOutput("[SECURITY] Remote debugger detected", "warn");
    }

    return resolveOffsets();
}

static int SafeCallDoString(luaL_dostring_t pfn, lua_State_t L, const char* str, bool& faultOccurred) {
    faultOccurred = false;
    __try {
        return pfn(L, str);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        faultOccurred = true;
        return -1;
    }
}

bool LuaHookEngine::executeScript(const std::string& script) {
    if (script.empty()) {
        return false;
    }

    std::string reason;
    if (!validateScriptSafety(script, reason)) {
        FramedIpcBridge::instance().sendOutput("[SAFETY BLOCKED] " + reason, "error");
        return false;
    }

    m_executing = true;
    m_abortRequested = false;

    FramedIpcBridge::instance().sendOutput("[VOLTRO ENGINE] Executing script payload (" + std::to_string(script.length()) + " bytes)...", "info");

    // Watchdog thread for 10-second timeout
    std::thread watchdog([this]() {
        auto start = std::chrono::steady_clock::now();
        while (m_executing && !m_abortRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= 10) {
                m_abortRequested = true;
                FramedIpcBridge::instance().sendOutput("[TIMEOUT] Script aborted: execution exceeded 10 second timeout limit.", "error");
                break;
            }
        }
    });
    watchdog.detach();

    // Call Roblox Lua VM if resolved
    if (m_pfnDoString && m_luaState) {
        bool fault = false;
        int status = SafeCallDoString(m_pfnDoString, m_luaState, script.c_str(), fault);
        if (fault) {
            FramedIpcBridge::instance().sendOutput("[EXCEPTION] Crash guard intercepted unhandled memory fault during execution.", "error");
        } else if (status == 0) {
            FramedIpcBridge::instance().sendOutput("[EXECUTE SUCCESS] Script executed successfully in Roblox Lua VM.", "ok");
        } else {
            FramedIpcBridge::instance().sendOutput("[LUA ERROR] Execution returned status code: " + std::to_string(status), "error");
        }
    } else {
        // Queue cleanly to Solara TaskScheduler execution pipeline
        SolaraExecutionEngine::instance().queueScript(script);
    }

    m_executing = false;
    return true;
}

void LuaHookEngine::playAttachAnimation() {
    const std::string animScript = R"(
        task.spawn(function()
            local TweenService = game:GetService("TweenService")
            local CoreGui = game:GetService("CoreGui")
            local Players = game:GetService("Players")
            local lp = Players.LocalPlayer

            local parentGui = nil
            pcall(function()
                if CoreGui then parentGui = CoreGui end
            end)
            if not parentGui and lp then
                pcall(function() parentGui = lp:WaitForChild("PlayerGui", 3) end)
            end
            if not parentGui then return end

            local old = parentGui:FindFirstChild("VoltroAttachSplash")
            if old then pcall(function() old:Destroy() end) end

            local sg = Instance.new("ScreenGui")
            sg.Name = "VoltroAttachSplash"
            sg.ResetOnSpawn = false
            sg.ZIndexBehavior = Enum.ZIndexBehavior.Sibling

            local container = Instance.new("Frame")
            container.Name = "Container"
            container.Size = UDim2.new(0, 190, 0, 70)
            container.Position = UDim2.new(0.5, -95, -0.35, 0)
            container.BackgroundColor3 = Color3.fromRGB(7, 9, 12)
            container.BackgroundTransparency = 0.12
            container.BorderSizePixel = 0
            container.Parent = sg

            local corner = Instance.new("UICorner")
            corner.CornerRadius = UDim.new(0, 14)
            corner.Parent = container

            local stroke = Instance.new("UIStroke")
            stroke.Color = Color3.fromRGB(0, 255, 102)
            stroke.Thickness = 2.5
            stroke.Transparency = 0.1
            stroke.Parent = container

            local vLabel = Instance.new("TextLabel")
            vLabel.Name = "VLogo"
            vLabel.Size = UDim2.new(0, 48, 0, 48)
            vLabel.Position = UDim2.new(0, 10, 0.5, -24)
            vLabel.BackgroundTransparency = 1
            vLabel.Text = "V"
            vLabel.TextColor3 = Color3.fromRGB(0, 255, 102)
            vLabel.TextSize = 38
            vLabel.Font = Enum.Font.GothamBold
            vLabel.Parent = container

            local title = Instance.new("TextLabel")
            title.Name = "Title"
            title.Size = UDim2.new(0, 115, 0, 22)
            title.Position = UDim2.new(0, 64, 0, 14)
            title.BackgroundTransparency = 1
            title.Text = "VOLTRO"
            title.TextColor3 = Color3.fromRGB(232, 238, 244)
            title.TextSize = 16
            title.Font = Enum.Font.GothamBold
            title.TextXAlignment = Enum.TextXAlignment.Left
            title.Parent = container

            local status = Instance.new("TextLabel")
            status.Name = "Status"
            status.Size = UDim2.new(0, 115, 0, 18)
            status.Position = UDim2.new(0, 64, 0, 36)
            status.BackgroundTransparency = 1
            status.Text = "ATTACHED"
            status.TextColor3 = Color3.fromRGB(0, 255, 102)
            status.TextSize = 12
            status.Font = Enum.Font.GothamBold
            status.TextXAlignment = Enum.TextXAlignment.Left
            status.Parent = container

            pcall(function() sg.Parent = parentGui end)

            local dropTween = TweenService:Create(
                container,
                TweenInfo.new(0.7, Enum.EasingStyle.Back, Enum.EasingDirection.Out),
                { Position = UDim2.new(0.5, -95, 0.05, 0) }
            )
            dropTween:Play()

            for i = 1, 3 do
                local glow = TweenService:Create(vLabel, TweenInfo.new(0.25, Enum.EasingStyle.Sine, Enum.EasingDirection.InOut), { TextColor3 = Color3.fromRGB(57, 255, 20), TextSize = 42 })
                glow:Play()
                glow.Completed:Wait()
                local unGlow = TweenService:Create(vLabel, TweenInfo.new(0.25, Enum.EasingStyle.Sine, Enum.EasingDirection.InOut), { TextColor3 = Color3.fromRGB(0, 255, 102), TextSize = 38 })
                unGlow:Play()
                unGlow.Completed:Wait()
            end

            task.wait(1.5)

            local exitTween = TweenService:Create(
                container,
                TweenInfo.new(0.55, Enum.EasingStyle.Back, Enum.EasingDirection.In),
                { Position = UDim2.new(0.5, -95, -0.35, 0) }
            )
            exitTween:Play()
            exitTween.Completed:Connect(function()
                pcall(function() sg:Destroy() end)
            end)
        end)
    )";

    executeScript(animScript);
}

void LuaHookEngine::stopExecution() {
    m_abortRequested = true;
    m_executing = false;
}

// -----------------------------------------------------------------------------
// IpcClientBridge Implementation
// -----------------------------------------------------------------------------

IpcClientBridge& IpcClientBridge::instance() {
    static IpcClientBridge inst;
    return inst;
}

IpcClientBridge::~IpcClientBridge() {
    stop();
}

bool IpcClientBridge::start() {
    stop();
    m_running = true;
    m_workerThread = std::thread(&IpcClientBridge::communicationLoop, this);
    return true;
}

void IpcClientBridge::stop() {
    m_running = false;
    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }
}

void IpcClientBridge::communicationLoop() {
    const wchar_t* pipeName = L"\\\\.\\pipe\\VoltroIPC";

    while (m_running) {
        // Attempt connection to Voltro UI Named Pipe
        m_pipe = CreateFileW(
            pipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (m_pipe == INVALID_HANDLE_VALUE) {
            // Server not ready, retry after sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        // Successfully connected to Voltro UI!
        sendStatus(true, "Voltro DLL hooked and connected to Roblox process.");
        sendOutput("[IPC BRIDGE] Connected to Voltro UI host.", "ok");

        // Wait for Roblox Lua VM to initialize before attempting execution
        sendOutput("[IPC BRIDGE] Waiting for Lua VM initialization...", "info");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Initialize Lua engine (resolve offsets + pattern scan)
        bool initOk = LuaHookEngine::instance().initialize();
        if (initOk) {
            sendOutput("[IPC BRIDGE] Lua engine initialized. Playing attach animation.", "ok");
            LuaHookEngine::instance().playAttachAnimation();
        } else {
            sendOutput("[IPC BRIDGE] Lua engine init FAILED. Script execution will use TaskScheduler fallback.", "warn");
        }

        char buffer[8192];
        DWORD bytesRead = 0;

        while (m_running) {
            BOOL success = ReadFile(m_pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!success || bytesRead == 0) {
                break; // Disconnected
            }

            buffer[bytesRead] = '\0';
            std::string message(buffer, bytesRead);

            // Handle newline-delimited messages
            std::stringstream ss(message);
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty()) {
                    handleMessage(line);
                }
            }
        }

        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void IpcClientBridge::handleMessage(const std::string& json) {
    if (json.find("EXECUTE_SCRIPT") != std::string::npos || json.find("EXECUTE_PAYLOAD") != std::string::npos) {
        // Extract script code from JSON
        std::string needle = "\"code\":\"";
        size_t pos = json.find(needle);
        std::string code;
        if (pos != std::string::npos) {
            pos += needle.length();
            size_t endPos = json.rfind("\"");
            if (endPos != std::string::npos && endPos > pos) {
                code = json.substr(pos, endPos - pos);
            }
        }
        if (code.empty()) {
            code = json;
        }

        // Unescape standard JSON string escapes
        std::string unescaped;
        for (size_t i = 0; i < code.length(); ++i) {
            if (code[i] == '\\' && i + 1 < code.length()) {
                char next = code[i + 1];
                if (next == 'n') { unescaped += '\n'; ++i; }
                else if (next == 'r') { unescaped += '\r'; ++i; }
                else if (next == 't') { unescaped += '\t'; ++i; }
                else if (next == '"') { unescaped += '"'; ++i; }
                else if (next == '\\') { unescaped += '\\'; ++i; }
                else { unescaped += next; ++i; }
            } else {
                unescaped += code[i];
            }
        }

        LuaHookEngine::instance().executeScript(unescaped);
    } else if (json.find("STOP") != std::string::npos) {
        LuaHookEngine::instance().stopExecution();
        sendOutput("[STOP] Script execution signaled to halt.", "warn");
    }
}

bool IpcClientBridge::sendOutput(const std::string& text, const std::string& level) {
    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::string escaped;
    for (char c : text) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"OUTPUT\",\"text\":\"" + escaped + "\",\"level\":\"" + level + "\"}\n";
    DWORD written = 0;
    return WriteFile(m_pipe, payload.data(), static_cast<DWORD>(payload.length()), &written, nullptr);
}

bool IpcClientBridge::sendStatus(bool attached, const std::string& message) {
    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    std::string payload = "{\"type\":\"ATTACH_STATUS\",\"attached\":" + std::string(attached ? "true" : "false") + ",\"message\":\"" + message + "\"}\n";
    DWORD written = 0;
    return WriteFile(m_pipe, payload.data(), static_cast<DWORD>(payload.length()), &written, nullptr);
}

} // namespace Runtime
} // namespace Voltro

// -----------------------------------------------------------------------------
// DLL Entry Point
// -----------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Start IPC only — defer TaskScheduler hooking until first script execution
        std::thread([]() {
            Voltro::Runtime::FramedIpcBridge::instance().start();
        }).detach();
        break;

    case DLL_PROCESS_DETACH:
        Voltro::Runtime::FramedIpcBridge::instance().stop();
        Voltro::Runtime::SolaraExecutionEngine::instance().shutdown();
        break;
    }
    return TRUE;
}
