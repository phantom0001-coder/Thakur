#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

namespace Voltro {
namespace Runtime {

// -----------------------------------------------------------------------------
// Roblox Luau Types & Identity Flags (Solara Architecture Reference)
// -----------------------------------------------------------------------------
typedef void* lua_State_t;

// Capability bitmask enabling all script capabilities (equivalent to Identity 8)
constexpr uint64_t CAPABILITIES_ALL = 0x3FFFFFFFFFFFFFFFULL;

#pragma pack(push, 1)
struct RobloxExtraSpace {
    uint8_t pad_00[0x18];
    uint64_t capabilities;
    uint8_t pad_20[0x20];
    int identity;
};
#pragma pack(pop)

// -----------------------------------------------------------------------------
// Solara-like Execution Engine: Dynamic TaskScheduler & ScriptContext Hook
// -----------------------------------------------------------------------------
class SolaraExecutionEngine {
public:
    static SolaraExecutionEngine& instance();

    bool initialize();
    void shutdown();

    // Enqueue user script to be stepped synchronously on Roblox main thread
    bool queueScript(const std::string& script);

    // Returns whether the TaskScheduler and ScriptContext hook is established
    bool isHooked() const { return m_hooked.load(); }
    lua_State_t getLuaState() const { return m_luaState; }

    // TaskScheduler step callback
    typedef void(__fastcall* JobStepFn)(void* thisPtr, void* stats);
    void onJobStep();

private:
    SolaraExecutionEngine() = default;
    ~SolaraExecutionEngine();

    uintptr_t findTaskScheduler();
    bool hookSchedulerJob(uintptr_t scheduler);
    void setupLuauEnvironment(lua_State_t L);

    std::atomic<bool> m_hooked{ false };
    std::atomic<bool> m_running{ false };
    lua_State_t m_luaState = nullptr;
    uintptr_t m_scriptContext = 0;
    uintptr_t m_targetJob = 0;

    // Original vftable step function pointer
    JobStepFn m_originalJobStep = nullptr;
    uintptr_t* m_vftable = nullptr;

    std::queue<std::string> m_scriptQueue;
    std::mutex m_queueMutex;
};

// -----------------------------------------------------------------------------
// Framed IPC Protocol (4-Byte Length-Prefixed Packets)
// -----------------------------------------------------------------------------
class FramedIpcBridge {
public:
    static FramedIpcBridge& instance();

    bool start();
    void stop();

    bool sendOutput(const std::string& text, const std::string& level = "print");
    bool sendStatus(bool attached, const std::string& message = "");

    void setScriptHandler(std::function<void(const std::string&)> handler);

private:
    FramedIpcBridge() = default;
    ~FramedIpcBridge();

    void communicationLoop();
    bool sendPacket(const std::string& payload);
    bool readExact(HANDLE pipe, void* buffer, DWORD bytesToRead);

    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_running{ false };
    std::thread m_workerThread;
    std::function<void(const std::string&)> m_scriptHandler;
};

} // namespace Runtime
} // namespace Voltro
