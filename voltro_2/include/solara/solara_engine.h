#pragma once

#ifndef SOLARA_ENGINE_H
#define SOLARA_ENGINE_H

#include "solara.h"
#include "injector.h"

namespace Solara {
namespace Engine {

// -----------------------------------------------------------------------------
// Solara Execution Engine (TaskScheduler Hooking & Luau Main-Thread Stepper)
// -----------------------------------------------------------------------------

class SolaraEngine {
public:
    static SolaraEngine& instance();

    bool initialize();
    void shutdown();

    bool queueScript(const std::string& script);
    bool isHooked() const { return m_hooked.load(); }

    void onJobStep();

    typedef void(__fastcall* StepFn)(void* thisPtr, void* stats);

private:
    SolaraEngine() = default;
    ~SolaraEngine();

    uintptr_t scanTaskScheduler();
    bool hookTaskSchedulerJob(uintptr_t scheduler);
    void elevateState(lua_State_t L);
    bool executeSafeLuau(const std::string& script);

    std::atomic<bool> m_hooked{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_hasRealScheduler{false};

    uintptr_t m_scheduler = 0;
    uintptr_t m_targetJob = 0;
    StepFn m_originalStep = nullptr;
    lua_State_t m_luaState = nullptr;

    std::queue<std::string> m_scriptQueue;
    std::mutex m_queueMutex;

    // Simulation / Standalone fallback thread when not inside Roblox address space
    std::thread m_fallbackThread;
    void fallbackStepLoop();
};

} // namespace Engine
} // namespace Solara

#endif // SOLARA_ENGINE_H
