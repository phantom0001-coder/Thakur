#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <cstdint>

namespace Voltro {
namespace Runtime {


typedef void* lua_State_t;
typedef int (*luaL_dostring_t)(lua_State_t L, const char* str);
typedef void (*roblox_print_t)(int type, const char* format, ...);

class LuaHookEngine {
public:
    static LuaHookEngine& instance();

    bool initialize();
    bool executeScript(const std::string& script);
    void playAttachAnimation();
    void stopExecution();

    bool isHooked() const { return m_hooked; }
    lua_State_t getState() const { return m_luaState; }

private:
    LuaHookEngine() = default;
    ~LuaHookEngine();

    bool scanSignatures();
    bool validateScriptSafety(const std::string& script, std::string& outReason);

    uintptr_t scanMemory(uintptr_t start, size_t size, const std::string& pattern);
    void parsePattern(const std::string& pattern, std::vector<BYTE>& bytes, std::vector<BYTE>& mask);

    lua_State_t m_luaState = nullptr;
    luaL_dostring_t m_pfnDoString = nullptr;
    roblox_print_t m_pfnRobloxPrint = nullptr;

    std::atomic<bool> m_hooked{false};
    std::atomic<bool> m_executing{false};
    std::atomic<bool> m_abortRequested{false};
};

class IpcClientBridge {
public:
    static IpcClientBridge& instance();

    bool start();
    void stop();

    bool sendOutput(const std::string& text, const std::string& level = "print");
    bool sendStatus(bool attached, const std::string& message = "");

private:
    IpcClientBridge() = default;
    ~IpcClientBridge();

    void communicationLoop();
    void handleMessage(const std::string& json);

    std::thread m_workerThread;
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_running{false};
};

} 
} 
