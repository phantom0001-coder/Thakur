#include "../include/solara/watchdog.h"
#include "../include/solara/solara_ipc.h"
#include "../include/solara/solara_engine.h"
#include <tlhelp32.h>
#include <algorithm>
#include <iostream>

namespace Solara {
namespace Supervisor {

RobloxWatchdog& RobloxWatchdog::instance() {
    static RobloxWatchdog inst;
    return inst;
}

RobloxWatchdog::~RobloxWatchdog() {
    stop();
}

DWORD RobloxWatchdog::findRobloxPid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    DWORD foundPid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name = pe.szExeFile;
            std::transform(name.begin(), name.end(), name.begin(), ::towlower);
            if (name.find(L"robloxplayerbeta") != std::wstring::npos ||
                name.find(L"robloxplayer") != std::wstring::npos) {
                foundPid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return foundPid;
}

void RobloxWatchdog::monitorLoop() {
    while (m_watching.load()) {
        DWORD pid = m_targetPid.load();
        if (pid == 0) {
            pid = findRobloxPid();
            if (pid != 0) {
                m_targetPid.store(pid);
            }
        }

        if (pid == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) {
            
            m_targetPid.store(0);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        
        DWORD waitRes = WaitForSingleObject(hProc, 1000);
        if (waitRes == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            GetExitCodeProcess(hProc, &exitCode);

            std::string msg = "[SOLARA WATCHDOG] Roblox closed (Exit Code: 0x" + std::to_string(exitCode) + ").";
            IPC::SolaraIpcBridge::instance().sendLog("warn", msg);
            IPC::SolaraIpcBridge::instance().sendStatus(false, "Roblox process exited.");

            Engine::SolaraEngine::instance().shutdown();
            m_targetPid.store(0);
        }

        CloseHandle(hProc);
    }
}

bool RobloxWatchdog::start(DWORD targetPid) {
    if (m_watching.exchange(true)) return true;

    m_targetPid.store(targetPid != 0 ? targetPid : GetCurrentProcessId());
    m_workerThread = std::thread(&RobloxWatchdog::monitorLoop, this);
    return true;
}

void RobloxWatchdog::stop() {
    m_watching.store(false);
    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }
}

} 
} 




extern "C" {
    __declspec(dllexport) void StartWatchdog() {
        Solara::Supervisor::RobloxWatchdog::instance().start();
    }

    __declspec(dllexport) void StopWatchdog() {
        Solara::Supervisor::RobloxWatchdog::instance().stop();
    }
}
