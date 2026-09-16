#pragma once

#ifndef SOLARA_WATCHDOG_H
#define SOLARA_WATCHDOG_H

#include "solara.h"
#include <thread>

namespace Solara {
namespace Supervisor {

class RobloxWatchdog {
public:
    static RobloxWatchdog& instance();

    bool start(DWORD targetPid = 0);
    void stop();

    bool isWatching() const { return m_watching.load(); }
    DWORD getWatchedPid() const { return m_targetPid.load(); }

private:
    RobloxWatchdog() = default;
    ~RobloxWatchdog();

    void monitorLoop();
    DWORD findRobloxPid();

    std::atomic<bool> m_watching{false};
    std::atomic<DWORD> m_targetPid{0};
    std::thread m_workerThread;
};

} 
} 

#endif 
