#include "voltro_runtime.h"
#include "solara_engine.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace Voltro {
namespace Runtime {





LuaHookEngine& LuaHookEngine::instance() {
    static LuaHookEngine inst;
    return inst;
}

LuaHookEngine::~LuaHookEngine() {
    stopExecution();
}

void LuaHookEngine::parsePattern(const std::string& pattern, std::vector<BYTE>& bytes, std::vector<BYTE>& mask) {
    bytes.clear();
    mask.clear();

    std::stringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0);
            mask.push_back(0x00);
        } else {
            BYTE val = static_cast<BYTE>(std::stoul(token, nullptr, 16));
            bytes.push_back(val);
            mask.push_back(0xFF);
        }
    }
}

uintptr_t LuaHookEngine::scanMemory(uintptr_t start, size_t size, const std::string& pattern) {
    std::vector<BYTE> patternBytes;
    std::vector<BYTE> patternMask;
    parsePattern(pattern, patternBytes, patternMask);

    if (patternBytes.empty() || size < patternBytes.size()) {
        return 0;
    }

    const BYTE* memory = reinterpret_cast<const BYTE*>(start);
    size_t scanLength = size - patternBytes.size();

    for (size_t i = 0; i < scanLength; ++i) {
        bool match = true;
        for (size_t j = 0; j < patternBytes.size(); ++j) {
            if (patternMask[j] == 0xFF && memory[i + j] != patternBytes[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return start + i;
        }
    }
    return 0;
}

bool LuaHookEngine::scanSignatures() {
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
        return false;
    }

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        return false;
    }

    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size_t imageSize = modInfo.SizeOfImage;

    
    const std::vector<std::string> dostringPatterns = {
        "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9",
        "48 89 4C 24 ? 55 53 56 57 41 54",
        "48 89 5C 24 ? 48 89 74 24 ?",
        "55 48 8B EC 48 83 EC 20",
        "57 48 83 EC 20 48 8B F9",
        "48 89 54 24 ? 48 89 4C 24 ? 57 48 83 EC",
        "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78",
    };

    uintptr_t doStringAddr = 0;
    for (const auto& pat : dostringPatterns) {
        doStringAddr = scanMemory(baseAddr, imageSize, pat);
        if (doStringAddr) break;
    }

    if (doStringAddr) {
        m_pfnDoString = reinterpret_cast<luaL_dostring_t>(doStringAddr);
    }

    
    const std::vector<std::string> statePatterns = {
        "48 8D 05 ? ? ? ? 48 8B D9 48 8B 00",
        "48 8B 05 ? ? ? ? 48 89 44 24",
        "4C 8D 05 ? ? ? ? 49 8B",
        "48 8B 0D ? ? ? ? 48 89",
        "4C 8B 05 ? ? ? ? 48 89",
        "48 8B 15 ? ? ? ? 48 85 D2",
    };

    uintptr_t stateAddr = 0;
    for (const auto& pat : statePatterns) {
        stateAddr = scanMemory(baseAddr, imageSize, pat);
        if (stateAddr) {
            
            int32_t relOffset = *reinterpret_cast<const int32_t*>(stateAddr + 3);
            uintptr_t targetPtr = stateAddr + 7 + relOffset;

            
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(targetPtr), &mbi, sizeof(mbi)) &&
                (mbi.State == MEM_COMMIT) &&
                !(mbi.Protect & PAGE_NOACCESS) &&
                !(mbi.Protect & PAGE_GUARD)) {
                m_luaState = *reinterpret_cast<lua_State_t*>(targetPtr);
                if (m_luaState) break;
            }
        }
    }

    
    if (m_pfnDoString && m_luaState) {
        m_hooked = true;
    } else {
        m_hooked = false;
        m_pfnDoString = nullptr;
        m_luaState = nullptr;
    }
    return m_hooked;
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

bool LuaHookEngine::initialize() {
    
    if (IsDebuggerPresent()) {
        IpcClientBridge::instance().sendOutput("[SECURITY] Debugger detected via IsDebuggerPresent", "warn");
    }

    BOOL remoteDebugger = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebugger);
    if (remoteDebugger) {
        IpcClientBridge::instance().sendOutput("[SECURITY] Remote debugger detected", "warn");
    }

    return scanSignatures();
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
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        
        sendStatus(true, "Voltro DLL hooked and connected to Roblox process.");
        sendOutput("[IPC BRIDGE] Connected to Voltro UI host.", "ok");

        
        LuaHookEngine::instance().playAttachAnimation();

        char buffer[8192];
        DWORD bytesRead = 0;

        while (m_running) {
            BOOL success = ReadFile(m_pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!success || bytesRead == 0) {
                break; 
            }

            buffer[bytesRead] = '\0';
            std::string message(buffer, bytesRead);

            
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

} 
} 





BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        
        std::thread([]() {
            
            Voltro::Runtime::SolaraExecutionEngine::instance().initialize();
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
